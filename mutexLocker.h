// File:  mutexLocker.h
// Date:  9/16/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Helper function for pThread mutexes.  Locks a mutex when created,
//        releases when destroyed.  This can be used to ease handling of
//        mutexes in a function with multiple control paths.

#ifndef MUTEX_HELPER_H_
#define MUTEX_HELPER_H_

// Standard C++ headers
#include <mutex>

class MutexLocker
{
public:
	MutexLocker(std::mutex &mutex);
	~MutexLocker();

private:
	std::mutex &mutex;
};

#endif// MUTEX_HELPER_H_
