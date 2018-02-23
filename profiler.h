// File:  profiler.h
// Date:  4/17/2013
// Auth:  K. Loux
// Desc:  Profiler object for analyzing applications.  Note:  This is NOT thread-safe!

/*
INSTRUCTIONS:
To use this profiling tool, uncomment "#define PROFILE" below.  Then,
instrument your code with the following macros:

PROFILER_START - called once, prior to calling any other macros
PROFILER_ENTER - call this to start the timer
PROFILER_EXIT - call this to stop the timer (one EXIT for every ENTER!)
PROFILE_THIS_SCOPE - call this to profile from the call to the end of the scope
PROFILER_PRINT - call once at the end of the program to print the results

Note that code segments are categorized based on function name.  So in order to
differentiate between portions of a function, these portions must be broken out
into separate subroutines.

You can profile a function and also the methods called within the function.

When you want to disable profiling, just undefine PROFILE.
*/

#ifndef PROFILER_H_
#define PROFILER_H_

// Comment out the line below to disable profiling
//#define PROFILE

#ifdef PROFILE

// Standard C++ headers
#include <unordered_map>
#include <stack>
#include <iostream>
#include <iomanip>
#include <string>
#include <cassert>
#include <utility>
#include <sstream>
#include <chrono>
#include <thread>
#include <mutex>

#if defined _WIN32
#define FUNC_NAME __FUNCTION__
#else
#define FUNC_NAME __PRETTY_FUNCTION__
#endif

struct FunctionThread
{
	FunctionThread(const std::string& function, const std::thread::id& threadId) : function(function), threadId(threadId) {}
	std::string function;
	std::thread::id threadId;

	bool operator==(const FunctionThread& other) const
	{
		return function.compare(other.function) == 0 &&
			threadId == other.threadId;
	}
};

// Specialized hash for FunctionThread
namespace std
{
template <>
struct hash<FunctionThread>
{
	size_t operator()(const FunctionThread& key) const
	{
		// Compute individual hash values for first,
		// second and third and combine them using XOR
		// and bit shifting:

		return (hash<std::string>()(key.function)
			^ (hash<std::thread::id>()(key.threadId) << 1));
	}
};
}

class Profiler
{
public:
	static void Start()
	{
		startTime = GetTime();
	}

	static void Enter(const std::string& function)
	{
		std::lock_guard<std::mutex> lock(entryMutex);
		entryTimes[std::this_thread::get_id()].push(std::make_pair(function, GetTime()));
	}

	static void Exit(const std::string& function)
	{
		const auto id(std::this_thread::get_id());

		// Catch cases where the user may have missed a PROFILER_EXIT macro
		assert(entryTimes.find(id) != entryTimes.end());
		assert(!entryTimes[id].empty());
		assert(entryTimes[id].top().first.compare(function) == 0);

		const FunctionThread identifier(function, std::this_thread::get_id());

		{
			std::lock_guard<std::mutex> lock(frequencyMutex);// It appears that the addition of these locks had a significant impact on execution time, and therefore the results may not be reliable...
			frequencies[identifier] = std::make_pair(
				frequencies[identifier].first + GetTime() - entryTimes[id].top().second,
				frequencies[identifier].second + 1);
		}

		std::lock_guard<std::mutex> lock(entryMutex);
		entryTimes[id].pop();
	}

	static void Print()
	{
		Print(std::cout);
	}

	static void Print(std::ostream& outStream)
	{
		std::lock_guard<std::mutex> entryTimesLock(entryMutex);
		auto etIt(entryTimes.cbegin());
		for (; etIt != entryTimes.end(); ++etIt)
		{
			if (!etIt->second.empty())
				outStream << "Warning:  Profiler stack is not empty for thread " << etIt->first << std::endl;
		}

		// TODO:  Add up time spent in methods on different threads?  Better to show it separately?

		std::lock_guard<std::mutex> frequencyLock(frequencyMutex);
		unsigned int maxFunctionNameLength(0);
		auto it(frequencies.cbegin());
		for (; it != frequencies.end(); ++it)
		{
			if (ExtractShortName(it->first.function).size() > maxFunctionNameLength)
				maxFunctionNameLength = ExtractShortName(it->first.function).size();
		}
		maxFunctionNameLength += 10;

		double totalTime = (double)(GetTime() - startTime);

		std::string percentColumnHeader("Percent Time    ");
		std::string callColumnHeader("Calls");
		outStream << '\n' << RightPadString("Function", maxFunctionNameLength)
			<< percentColumnHeader << callColumnHeader << '\n';
		outStream << RightPadString("", maxFunctionNameLength
			+ percentColumnHeader.size() + callColumnHeader.size(), '-') << '\n';

		for (it = frequencies.begin(); it != frequencies.end(); ++it)
			outStream << RightPadString(ExtractShortName(it->first.function), maxFunctionNameLength)
			<< RightPadString(ToString((double)it->second.first / totalTime * 100.0) + "%", percentColumnHeader.size())
			<< it->second.second << '\n';

		outStream << '\n' << std::endl;
	}

private:
	typedef std::unordered_map<FunctionThread, std::pair<unsigned long long, unsigned long>> NameTimeMap;
	typedef std::pair<std::string, unsigned long long> FunctionTimePair;

	static unsigned long long GetTime()
	{
/*#ifdef _WIN32
		// Below from: http://www.strchr.com/performance_measurements_with_rdtsc
		__asm
		{
			XOR eax, eax
			CPUID
			RDTSC
		}
#else
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		// To avoid overflow, let's only care about the number of seconds
		// since approx 2012
		return ((unsigned long long)ts.tv_sec - 1324512000ULL) * 1000000000ULL + (unsigned long long)ts.tv_nsec;
#endif*/
		return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	static NameTimeMap frequencies;
	static std::unordered_map<std::thread::id, std::stack<FunctionTimePair>> entryTimes;
	static unsigned long long startTime;

	static std::mutex entryMutex;
	static std::mutex frequencyMutex;

	static std::string RightPadString(const std::string &s, const unsigned int &l, const char c = ' ')
	{
		assert(l > s.size());
		std::string padded(s);
		padded.insert(padded.end(), l - padded.size(), c);
		return padded;
	}

	static std::string ToString(const double &d)
	{
		std::stringstream s;
		s << d;
		return s.str();
	}

	static std::string ExtractShortName(const std::string &s)
	{
#ifdef _WIN32
		return s;
#else
		// Remove the return type for calls that have one
		// (everything up to and including the first space)
		size_t start = s.find(' ');
		if (start == std::string::npos)
			start = 0;
		else
			start++;

		// Remove the argument list
		size_t end = s.find('(');
		if (end == std::string::npos)
			return s.substr(start);

		return s.substr(start, end - start);
#endif
	}
};

class ProfilerHelper
{
public:
	ProfilerHelper(const char* function) : function(function)
	{
		Profiler::Enter(function);
	}

	~ProfilerHelper()
	{
		Profiler::Exit(function);
	}

private:
	const char* function;
};

#define PROFILER_START Profiler::Start();
#define PROFILER_ENTER Profiler::Enter(FUNC_NAME);
#define PROFILER_EXIT Profiler::Exit(FUNC_NAME);
#define PROFILE_THIS_SCOPE ProfilerHelper profilerHelper(FUNC_NAME);
#define PROFILE_NAMED_SCOPE(x) ProfilerHelper namedHelper(x);
#define PROFILER_PRINT Profiler::Print();
#define PROFILER_PRINT_TO(x) Profiler::Print(x);

#else

#define PROFILER_START
#define PROFILER_ENTER
#define PROFILER_EXIT
#define PROFILE_THIS_SCOPE
#define PROFILE_NAMED_SCOPE(x)
#define PROFILER_PRINT
#define PROFILER_PRINT_TO(x)

#endif// PROFILE

#endif// PROFILER_H_
