// File:  cppSocket.h
// Date:  9/3/2013
// Auth:  K. Loux
// Desc:  Cross-platform wrapper around socket calls.

#ifndef CPP_SOCKET_H_
#define CPP_SOCKET_H_

// pThread headers (must be first!)
#include <pthread.h>

// Standard C++ headers
#include <string>
#include <vector>
#include <map>
#include <iostream>

#ifdef _WIN32
// Windows headers
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <WinSock2.h>
#else
// *nix headers
#include <sys/select.h>
#endif

// Forward declarations
struct sockaddr_in;

class CPPSocket
{
public:
	enum SocketType
	{
		SocketTCPServer,
		SocketTCPClient,
		SocketUDPServer,
		SocketUDPClient,
		SocketICMP
	};

#ifdef WIN32
	typedef unsigned int SocketID;
	typedef char DataType;
#else
	typedef int SocketID;
	typedef unsigned char DataType;
#endif

	CPPSocket(SocketType type = SocketTCPClient, std::ostream &outStream = std::cout);
	~CPPSocket();

	bool Create(const unsigned short &port = 0, const std::string &target = "");
	void Destroy();

	void DropClient(const SocketID& sockId);

	bool SetBlocking(bool blocking);

	int Receive(SocketID& sockId);
	int Receive(struct sockaddr_in *outSenderAddr = NULL);

	// NOTE:  If type == SocketTCPServer, calling method MUST aquire and release mutex when using GetLastMessage
	DataType* GetLastMessage() { clientMessageSize = 0; return rcvBuffer; }

	bool GetLock();
	bool ReleaseLock();
	pthread_mutex_t& GetMutex() { return bufferMutex; }
	
	bool SetOption(const int &level, const int &option, const DataType* value, const int &size);
	bool WaitForSocket(const int &timeout);

	bool Bind(const sockaddr_in &address);
	static sockaddr_in AssembleAddress(const unsigned short &port, const std::string &target = "");

	bool UDPSend(const char *addr, const short &port, const DataType* buffer, const int &bufferSize);// UDP version
	bool TCPSend(const DataType* buffer, const int &bufferSize);// TCP version

	inline bool IsICMP() const { return type == SocketICMP; }
	inline bool IsTCP() const { return type == SocketTCPServer || type == SocketTCPClient; }
	inline bool IsServer() const { return type == SocketTCPServer || type == SocketUDPServer; }

	unsigned int GetClientCount() const;

	SocketID GetFileDescriptor() const { return sock; }

	std::string GetErrorString() const { return GetLastError(); }

#ifndef WIN32
	static const int SOCKET_ERROR = -1;
#endif

	static const unsigned int maxMessageSize;

	static std::vector<std::string> GetLocalIPAddress();
	static std::string GetBestLocalIPAddress(const std::string &destination);

private:
	static const unsigned int maxConnections;
	static const unsigned int tcpListenTimeout;// [sec]

	const SocketType type;
	std::ostream &outStream;

	SocketID sock;
	DataType* rcvBuffer;

	bool Listen();
	bool Connect(const sockaddr_in &address);
	bool EnableAddressReusue();

	static std::string GetTypeString(SocketType type);
	static std::string GetLastError();

	int DoReceive(SocketID sock, struct sockaddr_in *senderAddr = NULL);
	bool TCPServerSend(const DataType* buffer, const int &bufferSize);

	// TCP server methods and members
	friend void *LaunchThread(void *pThisSocket);
	void ListenThreadEntry();
	void HandleClient(SocketID newSock);

	volatile bool continueListening;
	volatile std::map<SocketID, int> clientMessageSize;
	pthread_t listenerThread;
	pthread_mutex_t bufferMutex;
	fd_set clients;
	fd_set readSocks;
	SocketID maxSock;

	static void AddSocketToSet(SocketID socketFD, fd_set &set);
	static void RemoveSocketFromSet(SocketID socketFD, fd_set &set);
};

#endif// CPP_SOCKET_H_
