// File:  timingUtility.h
// Date:  10/14/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Timing utilities including elapsed time methods and loop timer.

#ifndef TIMING_UTILITY_H_
#define TIMING_UTILITY_H_

// Standard C/C++ headers
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <chrono>
#include <cstdint>

class TimingUtility
{
public:
	typedef std::chrono::steady_clock Clock;
	typedef std::chrono::duration<double> FractionalSeconds;

	TimingUtility(const double& newTimeStep, const double& warningThreshold = 1.01, std::ostream &outStream = std::cout);

	void SetLoopTime(const double& newTimeStep);
	void TimeLoop();
	double GetTimeStep() const { return FractionalSeconds(timeStep).count(); }// [sec]

	std::string GetTimingStatistics() const;

	static void SleepUntil(const Clock::time_point& targetTime);

private:
	const double warningThreshold;
	std::ostream &outStream;

	Clock::duration timeStep, elapsed = std::chrono::seconds(0);
	Clock::time_point lastLoopTime;

	// For timing statistics
	void UpdateTimingStatistics();
	std::string MakeColumn(double value, unsigned int columnWidth) const;
	std::string MakeColumn(Clock::duration value, unsigned int columnWidth) const;
	std::string MakeColumn(std::string s, unsigned int columnWidth, char pad = ' ') const;

	unsigned int currentIndex;
	std::map<Clock::duration, unsigned int> stepIndices;
	std::vector<unsigned long> counts;
	std::vector<Clock::duration> minFrameTimes, maxFrameTimes, averageFrameTimes;// [sec]
	std::vector<Clock::duration> minBusyTimes, maxBusyTimes, averageBusyTimes;// [sec]
	Clock::time_point lastUpdate;
};

#endif// TIMING_UTILITY_H_