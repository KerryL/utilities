// File:  profiler.cpp
// Date:  4/17/2013
// Auth:  K. Loux
// Desc:  Profiler object for analyzing applications.  Note:  This is NOT thread-safe!

// Local headers
#include "profiler.h"

#ifdef PROFILE

uint64_t Profiler::startTime;
Profiler::NameTimeMap Profiler::frequencies;
std::unordered_map<std::thread::id, std::stack<Profiler::FunctionTimePair>> Profiler::entryTimes;

std::mutex Profiler::entryTimesMutex;
std::mutex Profiler::frequenciesMutex;

#endif// PROFILE
