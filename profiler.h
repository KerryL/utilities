// File:  profiler.h
// Date:  4/17/2013
// Auth:  K. Loux
// Desc:  Profiler object for analyzing applications.  Note:  This is NOT thread-safe!

#ifndef _PROFILER_H_
#define _PROFILER_H_

// Comment out the line below to disable profiling
//#define PROFILE

#ifdef PROFILE

// Standard C++ headers
#include <unordered_map>
#include <stack>
#include <iostream>
#include <iomanip>
#include <string>
#include <assert.h>
#include <utility>
#include <sstream>

#if defined _WIN32
#define FUNC_NAME __FUNCTION__
#else
#define FUNC_NAME __PRETTY_FUNCTION__
#endif

class Profiler
{
public:
	static void Start(void)
	{
		startTime = GetTime();
	}

	static void Enter(const char* function)
	{
		entryTimes.push(FunctionTimePair(function, GetTime()));
	}

	static void Exit(const char* function)
	{
		// Catch cases where the user may have missed a PROFILER_EXIT macro
		assert(entryTimes.top().first.compare(function) == 0);
		frequencies[function] = std::pair<unsigned long long, unsigned long>(
				frequencies[function].first + GetTime() - entryTimes.top().second,
				frequencies[function].second + 1);
		entryTimes.pop();
	}

	static void Print(void)
	{
		std::cout << std::endl << std::endl;
		if (!entryTimes.empty())
			std::cout << "Warning:  Profiler stack is not empty!" << std::endl;

		unsigned int maxFunctionNameLength(0);
		NameTimeMap::iterator it;
		for (it = frequencies.begin(); it != frequencies.end(); ++it)
		{
			if (ExtractShortName(it->first).size() > maxFunctionNameLength)
				maxFunctionNameLength = ExtractShortName(it->first).size();
		}
		maxFunctionNameLength += 10;

		double totalTime = (double)(GetTime() - startTime);

		std::string percentColumnHeader("Percent Time    ");
		std::string callColumnHeader("Calls");
		std::cout << RightPadString("Function", maxFunctionNameLength)
			<< percentColumnHeader << callColumnHeader << std::endl;
		std::cout << RightPadString("", maxFunctionNameLength
			+ percentColumnHeader.size() + callColumnHeader.size(), '-') << std::endl;

		for (it = frequencies.begin(); it != frequencies.end(); ++it)
			std::cout << RightPadString(ExtractShortName(it->first), maxFunctionNameLength)
			<< RightPadString(ToString((double)it->second.first / totalTime * 100.0) + "%", percentColumnHeader.size())
			<< it->second.second << std::endl;

		std::cout << std::endl << std::endl;
	}

private:
	typedef std::unordered_map<std::string, std::pair<unsigned long long, unsigned long> > NameTimeMap;
	typedef std::pair<std::string, unsigned long long> FunctionTimePair;

	static unsigned long long GetTime(void)
	{
#ifdef _WIN32
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
#endif
	}

	static NameTimeMap frequencies;
	static std::stack<FunctionTimePair> entryTimes;
	static unsigned long long startTime;

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
#define PROFILER_PRINT Profiler::Print();
#define PROFILE_THIS_SCOPE ProfilerHelper profilerHelper(FUNC_NAME);

#else

#define PROFILER_START
#define PROFILER_ENTER
#define PROFILER_EXIT
#define PROFILER_PRINT
#define PROFILE_THIS_SCOPE

#endif// PROFILE

#endif// _PROFILER_H_
