// File:  cppSocket.h
// Date:  9/3/2013
// Auth:  K. Loux
// Desc:  Cross-platform wrapper around socket calls.

// Standard C++ headers
#include <cstdlib>
#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sstream>
#include <signal.h>

#ifdef _WIN32
#define poll WSAPoll
#else
// *nix headers
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <ifaddrs.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
#endif

// Local headers
#include "cppSocket.h"

using namespace std;

//==========================================================================
// Class:			CPPSocket
// Function:		Constant definitions
//
// Description:		Static constant definitions for the CPPSocket class.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
const unsigned int CPPSocket::maxMessageSize = 4096;
const unsigned int CPPSocket::maxConnections = 5;
const unsigned int CPPSocket::tcpListenTimeout = 5;// [sec]

//==========================================================================
// Class:			CPPSocket
// Function:		CPPSocket
//
// Description:		Constructor for CPPSocket class.
//
// Input Arguments:
//		type	= SocketType
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
CPPSocket::CPPSocket(SocketType type, ostream& outStream) : type(type), outStream(outStream)
{
	clientMessageSize = 0;
	pthread_mutex_init(&bufferMutex, NULL);

	rcvBuffer = new DataType[maxMessageSize];
}

//==========================================================================
// Class:			CPPSocket
// Function:		~CPPSocket
//
// Description:		Destructor for CPPSocket class.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
CPPSocket::~CPPSocket()
{
	Destroy();

	delete [] rcvBuffer;
	rcvBuffer = NULL;

	int errorNumber;
	if ((errorNumber = pthread_mutex_destroy(&bufferMutex)) != 0)
		outStream << "Error destroying mutex (" << errorNumber << ")" << endl;
}

//==========================================================================
// Class:			CPPSocket
// Function:		Create
//
// Description:		Creates a new socket.
//
// Input Arguments:
//		port		= const unsigned short& specifying the local port to which
//					  the socket should be bound
//		target		= [optional, default ""] const string& containing the
//					  IP address to which messages will be sent; useful for
//					  determining which of several NICs to bind to.  If
//					  omitted, will bind to INADDR_ANY.
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true if the socket is created successfully, false otherwise
//
//==========================================================================
bool CPPSocket::Create(const unsigned short &port, const string &target)
{
#ifdef WIN32
	WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 1), &wsaData) != 0)
	{
        outStream << "Failed to find Winsock 2.1 or better" << endl;
        return false;
    }
#endif

	if (IsICMP())
		sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	else if (IsTCP())
		sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	else
		sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sock == INVALID_SOCKET)
	{
		outStream << "  Socket creation Failed:  " << GetLastError() << endl;
		outStream << "  Port: " << port << endl;
		outStream << "  Type: " << GetTypeString(type) << endl;

		return false;
	}

	outStream << "  Created " << GetTypeString(type) << " socket with id " << sock << endl;

	if (type == SocketICMP)
		return true;
	else if (type == SocketTCPClient)
		return Connect(AssembleAddress(port, GetBestLocalIPAddress(target)));

	// TCP Servers and any UDP socket
	return Bind(AssembleAddress(port, GetBestLocalIPAddress(target)));
}

//==========================================================================
// Class:			CPPSocket
// Function:		Destroy
//
// Description:		Closes the socket connection and gets us prepared to
//					(possibly) re-use this object.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void CPPSocket::Destroy()
{
	continueListening = false;
	clientMessageSize = 0;
	int errorNumber;
	if (type == SocketTCPServer)
	{
		if ((errorNumber = pthread_join(listenerThread, NULL)) != 0)
			outStream << "Error destroying mutex (" << errorNumber << ")" << endl;
	}

#ifdef WIN32
	closesocket(sock);
	WSACleanup();
#else
	close(sock);
#endif
	outStream << "  Socket " << sock << " has been destroyed" << endl;
}

//==========================================================================
// Class:			CPPSocket
// Function:		AssembleAddress
//
// Description:		Assembles the specified address into a socket address structure.
//
// Input Arguments:
//		port	= const unsigned short&
//		target	= const std::string&
//
// Output Arguments:
//		None
//
// Return Value:
//		sockaddr_in pointing to the specified address and port
//
//==========================================================================
sockaddr_in CPPSocket::AssembleAddress(const unsigned short &port, const std::string &target)
{
	sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	if (target.empty())
		address.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		address.sin_addr.s_addr = inet_addr(target.c_str());
	address.sin_port = htons(port);

	return address;
}

//==========================================================================
// Class:			CPPSocket
// Function:		Bind
//
// Description:		Bind this socket to the specified port.
//
// Input Arguments:
//		address		= const sockaddr_in&
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true if the socket is bound successfully, false otherwise
//
//==========================================================================
bool CPPSocket::Bind(const sockaddr_in &address)
{
	if (type == SocketTCPServer)
		EnableAddressReusue();

	if (bind(sock, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
	{
		outStream << "  Bind to " << inet_ntoa(address.sin_addr) << ":"
			<< ntohs(address.sin_port) << " failed:  " << GetLastError() << endl;
		return false;
	}

	outStream << "  Socket " << sock << " successfully bound to " <<
		inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port) << endl;

	if (type == SocketTCPServer)
		return Listen();

	return true;
}

//==========================================================================
// Class:			CPPSocket
// Function:		SetOption
//
// Description:		Sets socket options.
//
// Input Arguments:
//		level	= const int&
//		option	= const int&
//		value	= const DataType*
//		size	= const int&
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool CPPSocket::SetOption(const int &level, const int &option, const DataType* value, const int &size)
{
	if (setsockopt(sock, level, option, value, size) == SOCKET_ERROR)
	{
		outStream << "Failed to set option:  " << GetLastError() << endl;
        return false;
    }
	
	return true;
}

//==========================================================================
// Class:			friend of CPPSocket
// Function:		LaunchThread
//
// Description:		Listener thread entry point (launches member function).
//
// Input Arguments:
//		pThisSocket =	void* (really a pointer to a CPPSocket)
//
// Output Arguments:
//		None
//
// Return Value:
//		void*
//
//==========================================================================
void *LaunchThread(void *pThisSocket)
{
	static_cast<CPPSocket*>(pThisSocket)->ListenThreadEntry();
	return NULL;
}

//==========================================================================
// Class:			CPPSocket
// Function:		Listen
//
// Description:		Spawns a new thread and puts the socket in a listen state
//					(in the new thread).
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true if successful, false otherwise
//
//==========================================================================
bool CPPSocket::Listen()
{
	continueListening = true;

	// TCP severs crash if writing to a broken pipe, if we don't explicitly ignore the error
#ifndef WIN32
	signal(SIGPIPE, SIG_IGN);
#endif

	if (listen(sock, maxConnections) == SOCKET_ERROR)
	{
		outStream << "  Listen on socket ID " << sock << " failed:  " << GetLastError() << endl;
		return false;
	}

	outStream << "  Socket " << sock << " listening" << endl;

	if (pthread_create(&listenerThread, NULL, &LaunchThread, (void*)this) == 0)
	{
		//outStream << "  Spawned listening thread with ID " << listenerThread << endl;// TODO:  Not sure why MSCV complains about this... Linux is OK
		return true;
	}

	return false;
}

//==========================================================================
// Class:			CPPSocket
// Function:		WaitForSocket
//
// Description:		Implements poll() on read for the socket.
//
// Input Arguments:
//		timeout	= const int& [msec]
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true if socket is ready for read, false if timeout occured
//
//==========================================================================
bool CPPSocket::WaitForSocket(const int &timeout)
{
	struct pollfd fds;
	fds.fd = sock;
	fds.events = POLLIN;
	int retVal = poll(&fds, 1, timeout);
	if (retVal == SOCKET_ERROR)
		outStream << "poll failed:  " << GetLastError() << std::endl;
	return retVal > 0;
}

//==========================================================================
// Class:			CPPSocket
// Function:		AddSocketToSet
//
// Description:		Calls FD_SET with appropriate #pragmas for clean compile.
//
// Input Arguments:
//		socketFD	= SocketID
//		set			= fd_set&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void CPPSocket::AddSocketToSet(SocketID socketFD, fd_set &set)
{
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable:4127)
#endif
	FD_SET(socketFD, &set);
#ifdef _MSC_VER
#pragma warning (pop)
#endif
}

//==========================================================================
// Class:			CPPSocket
// Function:		RemoveSocketFromSet
//
// Description:		Calls FD_SET with appropriate #pragmas for clean compile.
//
// Input Arguments:
//		socketFD	= SocketID
//		set			= fd_set&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void CPPSocket::RemoveSocketFromSet(SocketID socketFD, fd_set &set)
{
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable:4127)
#endif
	FD_CLR(socketFD, &set);
#ifdef _MSC_VER
#pragma warning (pop)
#endif
}

//==========================================================================
// Class:			CPPSocket
// Function:		ListenThreadEntry
//
// Description:		Listener thread entry point.  Listening, accepting
//					connections and receiving data happens in this thread,
//					but access to the data and sends are handled in the main
//					thread.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void CPPSocket::ListenThreadEntry()
{
	FD_ZERO(&clients);
	AddSocketToSet(sock, clients);
	maxSock = sock;

	struct timeval timeout;
	timeout.tv_sec = tcpListenTimeout;
	timeout.tv_usec = 0;

	SocketID s;
	while (continueListening)
	{
		readSocks = clients;
		if (select(maxSock + 1, &readSocks, NULL, NULL, &timeout) == SOCKET_ERROR)
		{
			outStream << "  Failed to select sockets:  " << GetLastError() << std::endl;
			continue;
		}

		for (s = 0; s <= maxSock; s++)
		{
			if (FD_ISSET(s, &readSocks))
			{
				// Socket ID s is ready
				if (s == sock)// New connection
				{
					SocketID newSock;
					struct sockaddr_in clientAddress;
					unsigned int size = sizeof(struct sockaddr_in);
#ifdef WIN32
					newSock = accept(sock, (struct sockaddr*)&clientAddress, (int*)&size);
#else
					newSock = accept(sock, (struct sockaddr*)&clientAddress, &size);
#endif
					if (newSock == SOCKET_ERROR)
					{
						outStream << "  Failed to accept connection:  " << GetLastError() << std::endl;
						continue;
					}

					/*cout << "  connection from: " << inet_ntoa(clientAddress.sin_addr
						<< ":" << ntohs(clientAddress.sin_port) << endl;//*/

					AddSocketToSet(newSock, clients);
					if (newSock > maxSock)
						maxSock = newSock;
				}
				else
					HandleClient(s);
			}
		}
	}
}

//==========================================================================
// Class:			CPPSocket
// Function:		HandleClient
//
// Description:		Handles incomming requests from client sockets.
//
// Input Arguments:
//		newSock	= SocketID
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void CPPSocket::HandleClient(SocketID newSock)
{
	int errorNumber;
	if ((errorNumber = pthread_mutex_lock(&bufferMutex)) != 0)
		outStream << "  Error locking mutex (" << errorNumber << ")" << endl;
	clientMessageSize = DoReceive(newSock);
	if ((errorNumber = pthread_mutex_unlock(&bufferMutex)) != 0)
		outStream << "  Error unlocking mutex (" << errorNumber << ")" << endl;

	// On disconnect
	if (clientMessageSize <= 0)
	{
		outStream << "  Client " << newSock << " disconnected" << std::endl;
		RemoveSocketFromSet(newSock, clients);

#ifdef WIN32
		closesocket(newSock);
#else
		close(newSock);
#endif
	}
}

//==========================================================================
// Class:			CPPSocket
// Function:		Connect
//
// Description:		Connect to the specified server.
//
// Input Arguments:
//		address		= const sockaddr_in&
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true if the connection is successful, false otherwise
//
//==========================================================================
bool CPPSocket::Connect(const sockaddr_in &address)
{
	if (connect(sock, (struct sockaddr*)&address, sizeof(address)) < 0)
	{
		outStream << "  Connect to " << ntohs(address.sin_port) << " failed:  " << GetLastError() << endl;
		return false;
	}

	outStream << "  Socket " << sock << " on port " << ntohs(address.sin_port) << " successfully connected" << endl;

	return true;
}

//==========================================================================
// Class:			CPPSocket
// Function:		EnableAddressReusue
//
// Description:		Sets the socket options to enable re-use of the address.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true if successful, false otherwise
//
//==========================================================================
bool CPPSocket::EnableAddressReusue()
{
	int one(1);
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (DataType*)&one, sizeof(int)) == SOCKET_ERROR)
	{
		outStream << "  Set socket options failed for socket " << sock << ":  " << GetLastError() << endl;
		return false;
	}

	return true;
}

//==========================================================================
// Class:			CPPSocket
// Function:		SetBlocking
//
// Description:		Sets the blocking mode as specified.
//
// Input Arguments:
//		blocking	= bool specifying whether socket operations should block or not
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool CPPSocket::SetBlocking(bool blocking)
{
#ifdef WIN32
	unsigned long mode = blocking ? 0 : 1;// 1 = Non-Blocking, 0 = Blocking
	return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
	int flags = fcntl(sock, F_GETFL, 0);
	if (flags < 0)
		return false;

	flags = blocking ? (flags &~ O_NONBLOCK) : (flags | O_NONBLOCK);
	return fcntl(sock, F_SETFL, flags) == 0;
#endif
}

//==========================================================================
// Class:			CPPSocket
// Function:		Receive
//
// Description:		Receives messages from the socket.
//
// Input Arguments:
//		outSenderAddr	= struct sockaddr_in* (optional)
//
// Output Arguments:
//		None
//
// Return Value:
//		int specifying bytes received, or SOCKET_ERROR on error
//
//==========================================================================
int CPPSocket::Receive(struct sockaddr_in *outSenderAddr)
{
	if (type == SocketTCPServer)
		return clientMessageSize;

	struct sockaddr_in quietSenderAddr, *useSenderAddr;
	if (outSenderAddr)
		useSenderAddr = outSenderAddr;
	else
		useSenderAddr = &quietSenderAddr;

	int bytesrcv;
	bytesrcv = DoReceive(sock, useSenderAddr);
	if (bytesrcv == SOCKET_ERROR)
	{
		//outStream << "  Error receiving message on socket " << sock << ": " << GetLastError() << endl;
		return SOCKET_ERROR;
	}
	else if (bytesrcv == 0)
	{
		outStream << "  Received empty packet from "
			<< inet_ntoa(useSenderAddr->sin_addr) << ":" << ntohs(useSenderAddr->sin_port) << endl;
		return SOCKET_ERROR;
	}

	// Helpful for debugging, but generally we don't want it in our logs
	/*outStream << "  Received " << bytesrcv << " bytes from "
		<< inet_ntoa(useSenderAddr->sin_addr) << ":" << ntohs(useSenderAddr->sin_port) << endl;//*/

	return bytesrcv;
}

//==========================================================================
// Class:			CPPSocket
// Function:		DoReceive
//
// Description:		Receives messages from the specified socket.  Does not
//					include any error handling.  This class is geared towards
//					small messages (receivable in a single call to recv()).
//					It is possible to handle larger messages, but the calling
//					methods will have to do some work to properly reconstruct
//					the message.
//
// Input Arguments:
//		sock		= int
//		senderAddr	= struct sockaddr_in*
//
// Output Arguments:
//		None
//
// Return Value:
//		int specifying bytes received, or SOCKET_ERROR on error
//
//==========================================================================
#ifdef _WIN32
__pragma(warning(push));
__pragma(warning(disable:4458));
#endif
int CPPSocket::DoReceive(SocketID sock, struct sockaddr_in *senderAddr)
{
	if (senderAddr)
	{
#ifdef WIN32
		int addrSize = sizeof(*senderAddr);
#else
		socklen_t addrSize = sizeof(*senderAddr);
#endif
		return recvfrom(sock, rcvBuffer, maxMessageSize, 0, (struct sockaddr*)senderAddr, &addrSize);
	}
	else
	{
		return recv(sock, rcvBuffer, maxMessageSize, 0);
	}
}
#ifdef _WIN32
__pragma(warning(pop));
#endif

//==========================================================================
// Class:			CPPSocket
// Function:		UDPSend
//
// Description:		Sends a message to the specified address and port (UDP).
//
// Input Arguments:
//		addr		= const char* containing the destination IP for the message
//		port		= const short& specifying the destination port for the message
//		buffer		= const DataType* pointing to the message body contents
//		bufferSize	= const int& specifying the size of the message
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool CPPSocket::UDPSend(const char *addr, const short &port,
	const DataType* buffer, const int &bufferSize)
{
	assert(!IsTCP());

	struct sockaddr_in targetAddress = AssembleAddress(port, addr);

	int bytesSent = sendto(sock, buffer, bufferSize, 0,
		(struct sockaddr*)&targetAddress, sizeof(targetAddress));

	if (bytesSent == SOCKET_ERROR)
	{
		outStream << "  Error sending UDP message to "
			<< inet_ntoa(targetAddress.sin_addr) << ":"
			<< ntohs(targetAddress.sin_port) << ":  "
			<< GetLastError() << endl;
		return false;
	}

	if (bytesSent != bufferSize)
	{
		outStream << "  Wrong number of bytes sent (UDP) to "
			<< inet_ntoa(targetAddress.sin_addr) << ":"
			<< ntohs(targetAddress.sin_port) << endl;
		return false;
	}

	return true;
}

//==========================================================================
// Class:			CPPSocket
// Function:		TCPSend
//
// Description:		Sends a message to the connected server (TCP).
//
// Input Arguments:
//		buffer		= const DataType* pointing to the message body contents
//		bufferSize	= const int& specifying the size of the message
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool CPPSocket::TCPSend(const DataType* buffer, const int &bufferSize)
{
	assert(IsTCP());

	if (IsServer())
		return TCPServerSend(buffer, bufferSize);

	int bytesSent = send(sock, buffer, bufferSize, 0);

	if (bytesSent == SOCKET_ERROR)
	{
		outStream << "  Error sending TCP message: " << GetLastError() << endl;
		return false;
	}

	if (bytesSent != bufferSize)
	{
		outStream << "  Wrong number of bytes sent (TCP)" << endl;
		return false;
	}

	return true;
}

//==========================================================================
// Class:			CPPSocket
// Function:		TCPServerSend
//
// Description:		Sends a message to all of the connected clients (TCP).
//
// Input Arguments:
//		buffer		= const DataType* pointing to the message body contents
//		bufferSize	= const int& specifying the size of the message
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool CPPSocket::TCPServerSend(const DataType* buffer, const int &bufferSize)
{
	int bytesSent;
	SocketID s;
	bool success(true), calledSend(false);

	for (s = 0; s <= maxSock; s++)
	{
		if (!FD_ISSET(s, &clients) || s == sock)
			continue;

		bytesSent = send(s, buffer, bufferSize, 0);
		calledSend = true;

		if (bytesSent == SOCKET_ERROR)
		{
			outStream << "  Error sending TCP message on socket " << s << ": " << GetLastError() << endl;
			success = false;
		}
		else if (bytesSent != bufferSize)
		{
			outStream << "  Wrong number of bytes sent (TCP) on socket "<< s << endl;
			success = false;
		}
	}

	return success && calledSend;
}

//==========================================================================
// Class:			CPPSocket
// Function:		GetLocalIPAddress
//
// Description:		Retrieves a list of all local IP addresses.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		vector<string> containing local network interface addresses
//
//==========================================================================
vector<string> CPPSocket::GetLocalIPAddress()
{
	vector<string> ips;
#ifdef _WIN32
	char host[80];

	// Get the host name
	if (gethostname(host, sizeof(host)) == SOCKET_ERROR)
	{
		/*outStream*/cerr << "  Error getting host name: " << GetLastError() << endl;
		return ips;
	}

	struct hostent *hostEntity = gethostbyname(host);

	if (hostEntity == 0)
	{
		/*outStream*/cerr << "  Bad host lookup!" << endl;
		return ips;
	}

	struct in_addr addr;

	// Return ALL addresses
	unsigned int i;
	for (i = 0; hostEntity->h_addr_list[i] != 0; ++i)
	{
		memcpy(&addr, hostEntity->h_addr_list[i], sizeof(struct in_addr));
		ips.push_back(inet_ntoa(addr));
	}
#else
	struct ifaddrs* addresses;
	if (getifaddrs(&addresses) == SOCKET_ERROR)
	{
		/*outStream*/cerr << "  Error getting local addresses: " << GetLastError() << endl;
		return ips;
	}

	struct ifaddrs* ifa = addresses;
	while (ifa->ifa_next)
	{
		ips.push_back(inet_ntoa(((struct sockaddr_in*)(ifa->ifa_addr))->sin_addr));
		ifa = ifa->ifa_next;
	}

	freeifaddrs(addresses);
#endif

	return ips;
}

//==========================================================================
// Class:			CPPSocket
// Function:		GetBestLocalIPAddress
//
// Description:		Retrieves the most likely local IP address given the destination
//					address.
//
// Input Arguments:
//		destination	= const std::string&
//
// Output Arguments:
//		None
//
// Return Value:
//		std::string containing the best local IP
//
//==========================================================================
std::string CPPSocket::GetBestLocalIPAddress(const string &destination)
{
	if (destination.empty())
		return "";

	unsigned int i;
	vector<string> ips(GetLocalIPAddress());
	string compareString(destination.substr(0, destination.find_last_of('.')));
	for (i = 0; i < ips.size(); i++)
	{
		// Use the first address that matches beginning of destination
		if (ips[i].substr(0, compareString.size()).compare(compareString) == 0)
			return ips[i];
	}

	return "";
}

//==========================================================================
// Class:			CPPSocket
// Function:		GetTypeString
//
// Description:		Returns a string describing the socket type.
//
// Input Arguments:
//		type	= SocketType
//
// Output Arguments:
//		None
//
// Return Value:
//		string containing the type description
//
//==========================================================================
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable:4715)
#endif
std::string CPPSocket::GetTypeString(SocketType type)
{
	if (type == SocketTCPServer)
		return "TCP Server";
	else if (type == SocketTCPClient)
		return "TCP Client";
	else if (type == SocketUDPServer)
		return "UDP Server";
	else if (type == SocketUDPClient)
		return "UPD Client";
	else if (type == SocketICMP)
		return "IMCP";
	else
		assert(false);
}
#ifdef _MSC_VER
#pragma warning (pop)
#endif

//==========================================================================
// Class:			CPPSocket
// Function:		GetLastError
//
// Description:		Returns a string describing the last error received.  Broken
//					out into a separate function for portability.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		string containing the error description
//
//==========================================================================
std::string CPPSocket::GetLastError()
{
	stringstream errorString;
#ifdef WIN32
	errorString << "(" << WSAGetLastError() << ")";
#else
	errorString << "(" << errno << ") " << strerror(errno);
#endif
	return errorString.str();
}

//==========================================================================
// Class:			CPPSocket
// Function:		GetLock
//
// Description:		Aquires a lock on the buffer mutex.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true if lock aquired, false otherwise
//
//==========================================================================
bool CPPSocket::GetLock()
{
	return pthread_mutex_trylock(&bufferMutex) == 0;
}

//==========================================================================
// Class:			CPPSocket
// Function:		ReleaseLock
//
// Description:		Releases the lock on the buffer mutex.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true if lock released, false otherwise
//
//==========================================================================
bool CPPSocket::ReleaseLock()
{
	return pthread_mutex_unlock(&bufferMutex) == 0;
}

//==========================================================================
// Class:			CPPSocket
// Function:		GetClientCount
//
// Description:		Returns the number of connected TCP clients.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		unsigned int
//
//==========================================================================
unsigned int CPPSocket::GetClientCount() const
{
	assert(type == SocketTCPServer);

	SocketID s;
	unsigned int count(0);
	for (s = 0; s <= maxSock; s++)
	{
		if (!FD_ISSET(s, &clients) || s == sock)
			continue;
		count++;
	}

	return count;
}
