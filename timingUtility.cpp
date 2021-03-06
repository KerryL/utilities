// File:  timingUtility.cpp
// Date:  10/14/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Timing utilities including elapsed time methods and loop timer.

// Local headers
#include "timingUtility.h"

// Standard C++ headers
#include <errno.h>
#include <cstring>
#include <cassert>
#include <iomanip>
#include <sstream>
#include <thread>

// OS headers
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

//==========================================================================
// Class:			TimingUtility
// Function:		TimingUtility
//
// Description:		Constructor for TimingUtility class.
//
// Input Arguments:
//		newTimeStep			= const double& [sec]
//		warningThreshold	= const double& [%]
//		outStream			= UString::OStream&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
TimingUtility::TimingUtility(const double& newTimeStep, const double& warningThreshold,
	UString::OStream &outStream) : warningThreshold(warningThreshold), outStream(outStream)
{
	SetLoopTime(newTimeStep);
}

//==========================================================================
// Class:			TimingUtility
// Function:		SetLoopTime
//
// Description:		Sets the time step for the loop timer.
//
// Input Arguments:
//		newTimeStep	= const double& [sec]
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void TimingUtility::SetLoopTime(const double& newTimeStep)
{
	assert(newTimeStep > 0.0);
	timeStep = std::chrono::duration_cast<std::chrono::nanoseconds>(FractionalSeconds(newTimeStep));

	if (stepIndices.find(timeStep) == stepIndices.end())
	{
		currentIndex = counts.size();
		stepIndices[timeStep] = currentIndex;

		counts.push_back(0);
		minFrameTimes.push_back(timeStep * 100);// Something much larger than timeStep
		maxFrameTimes.push_back(std::chrono::seconds(0));
		averageFrameTimes.push_back(std::chrono::seconds(0));
		minBusyTimes.push_back(minFrameTimes.front());
		maxBusyTimes.push_back(std::chrono::seconds(0));
		averageBusyTimes.push_back(std::chrono::seconds(0));
	}
	else
		currentIndex = stepIndices[timeStep];

	assert(currentIndex < counts.size());
	assert(currentIndex < minFrameTimes.size());
	assert(currentIndex < maxFrameTimes.size());
	assert(currentIndex < averageFrameTimes.size());
	assert(currentIndex < minBusyTimes.size());
	assert(currentIndex < maxBusyTimes.size());
	assert(currentIndex < averageBusyTimes.size());
}

//==========================================================================
// Class:			TimingUtility
// Function:		TimeLoop
//
// Description:		Performs timing and sleeping to maintain specified time
//					step.  Must be called exactly once per loop and AT
//					THE TOP OF THE LOOP!.  If not placed at the top of the loop,
//					the first and second passes through the loop both occur
//					prior to the first sleep.
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
void TimingUtility::TimeLoop()
{
	const Clock::time_point now(Clock::now());

	if (counts[0] > 0)
		elapsed = now - lastLoopTime;

	const Clock::duration sleepTime(timeStep - elapsed);
	if (elapsed < timeStep)
	{
		std::this_thread::sleep_for(sleepTime);
		lastLoopTime = now + sleepTime;
	}
	else// Reset timer to now instead of trying to achieve a time that has already passed
		lastLoopTime = now;

	if (elapsed > timeStep * warningThreshold)
		outStream << "Warning:  Elapsed time is greater than time step ("
			<< FractionalSeconds(elapsed).count() << " > " << FractionalSeconds(timeStep).count() << ")" << std::endl;

	UpdateTimingStatistics();
	counts[currentIndex]++;
}

//==========================================================================
// Class:			TimingUtility
// Function:		UpdateTimingStatistics
//
// Description:		Updates timing statistics for the loop.
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
void TimingUtility::UpdateTimingStatistics()
{
	const Clock::time_point now(Clock::now());	
	const Clock::duration totalElapsed(now - lastUpdate);// including sleep
	lastUpdate = now;

	if (counts[0] == 0)
		return;

	if (totalElapsed < minFrameTimes[currentIndex])
		minFrameTimes[currentIndex] = totalElapsed;
	if (totalElapsed > maxFrameTimes[currentIndex])
		maxFrameTimes[currentIndex] = totalElapsed;

	if (elapsed < minBusyTimes[currentIndex])
		minBusyTimes[currentIndex] = elapsed;
	if (elapsed > maxBusyTimes[currentIndex])
		maxBusyTimes[currentIndex] = elapsed;

	// To compute averages, we need at least one good data point
	if (counts[currentIndex] == 0)
		return;

	averageFrameTimes[currentIndex] = std::chrono::duration_cast<Clock::duration>((averageFrameTimes[currentIndex] * (counts[currentIndex] - 1)
		+ totalElapsed) / static_cast<double>(counts[currentIndex]));
	averageBusyTimes[currentIndex] = std::chrono::duration_cast<Clock::duration>((averageBusyTimes[currentIndex] * (counts[currentIndex] - 1)
		+ elapsed) / static_cast<double>(counts[currentIndex]));
}

//==========================================================================
// Class:			TimingUtility
// Function:		GetTimingStatistics
//
// Description:		Returns a string containing the timing statistics for the
//					loop.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		UString::String
//
//==========================================================================
UString::String TimingUtility::GetTimingStatistics() const
{
	std::vector<double> timeAtStep;
	double totalTime(0.0);// [sec]
	auto it(stepIndices.begin());
	for (unsigned int i = 0; i < counts.size(); i++)
	{
		timeAtStep.push_back(FractionalSeconds(it->first).count() * counts[it->second]);
		totalTime += timeAtStep[i];
		it++;
	}

	const unsigned int titleColumnWidth(24), dataColumnWidth(12);

	UString::OStringStream ss;
	it = stepIndices.begin();
	for (unsigned int i = 0; i < counts.size(); i++)
	{
		ss << "Time step = " << FractionalSeconds(it->first).count() <<
			" sec (" << timeAtStep[i] / totalTime * 100.0 << "% of total loop time)" << std::endl;
		ss << MakeColumn("", titleColumnWidth)
			<< MakeColumn("Min", dataColumnWidth)
			<< MakeColumn("Max", dataColumnWidth)
			<< MakeColumn("Avg", dataColumnWidth) << std::endl;
		ss << MakeColumn("", titleColumnWidth + 3 * dataColumnWidth, '-') << std::endl;
		ss << MakeColumn("Frame Duration (sec)", titleColumnWidth)
			<< MakeColumn(minFrameTimes[it->second], dataColumnWidth)
			<< MakeColumn(maxFrameTimes[it->second], dataColumnWidth)
			<< MakeColumn(averageFrameTimes[it->second], dataColumnWidth)
			<< std::endl;
		ss << MakeColumn("Busy Period    (sec)", titleColumnWidth)
			<< MakeColumn(minBusyTimes[it->second], dataColumnWidth)
			<< MakeColumn(maxBusyTimes[it->second], dataColumnWidth)
			<< MakeColumn(averageBusyTimes[it->second], dataColumnWidth)
			<< std::endl;
		ss << std::endl;
		it++;
	}

	return ss.str();
}

//==========================================================================
// Class:			TimingUtility
// Function:		MakeColumn
//
// Description:		Formats the argument into a fixed-width right-padded string.
//
// Input Arguments:
//		value		= double
//		columnWidth	= unsigned int
//
// Output Arguments:
//		None
//
// Return Value:
//		UString::String
//
//==========================================================================
UString::String TimingUtility::MakeColumn(double value, unsigned int columnWidth) const
{
	UString::OStringStream ss;
	ss.precision(6);
	ss << std::fixed << value;
	return MakeColumn(ss.str(), columnWidth);
}

//==========================================================================
// Class:			TimingUtility
// Function:		MakeColumn
//
// Description:		Formats the argument into a fixed-width right-padded string.
//
// Input Arguments:
//		s			= UString::String
//		columnWidth	= unsigned int
//		pad			= char
//
// Output Arguments:
//		None
//
// Return Value:
//		UString::String
//
//==========================================================================
UString::String TimingUtility::MakeColumn(UString::String s, unsigned int columnWidth, char pad) const
{
	if (s.length() < columnWidth)
		s.append(std::string(columnWidth - s.length(), pad));

	return s;
}

//==========================================================================
// Class:			TimingUtility
// Function:		MakeColumn
//
// Description:		Formats the argument into a fixed-width right-padded string.
//
// Input Arguments:
//		value		= Clock::duration
//		columnWidth	= unsigned int
//		pad			= char
//
// Output Arguments:
//		None
//
// Return Value:
//		UString::String
//
//==========================================================================
UString::String TimingUtility::MakeColumn(Clock::duration value, unsigned int columnWidth) const
{
	return MakeColumn(FractionalSeconds(value).count(), columnWidth);
}

//==========================================================================
// Class:			TimingUtility
// Function:		SleepUntil
//
// Description:		Sleeps until the specified time.
//
// Input Arguments:
//		targetTime	= const Clock::time_point&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void TimingUtility::SleepUntil(const Clock::time_point& targetTime)
{
	const Clock::duration sleepTime(targetTime - Clock::now());
	assert(sleepTime.count() > 0);
	std::this_thread::sleep_for(sleepTime);
}
