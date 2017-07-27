// File:  mutexLocker.cpp
// Date:  9/16/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Helper function for pThread mutexes.  Locks a mutex when created,
//        releases when destroyed.  This can be used to ease handling of
//        mutexes in a function with multiple control paths.

// Standard C++ headers
#include <iostream>

// Local headers
#include "mutexLocker.h"

//==========================================================================
// Class:			MutexLocker
// Function:		MutexLocker
//
// Description:		Constructor for MutexLocker class.
//
// Input Arguments:
//		mutex	= std::mutex& to lock (must have been previously initialized)
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
MutexLocker::MutexLocker(std::mutex &mutex) : mutex(mutex)
{
	mutex.lock();
}

//==========================================================================
// Class:			MutexLocker
// Function:		~MutexLocker
//
// Description:		Destructor for MutexLocker class.
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
MutexLocker::~MutexLocker()
{
	mutex.unlock();
}
