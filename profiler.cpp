// File:  profiler.cpp
// Date:  4/17/2013
// Auth:  K. Loux
// Desc:  Profiler object for analyzing applications.  Note:  This is NOT thread-safe!

// Local headers
#include "profiler.h"

#ifdef PROFILE

unsigned long long Profiler::startTime;
Profiler::NameTimeMap Profiler::frequencies;
std::stack<Profiler::FunctionTimePair> Profiler::entryTimes;

#endif// PROFILE
