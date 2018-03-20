// File:  mutexUtilities.cpp
// Date:  3/20/2018
// Auth:  K. Loux
// Copy:  (c) Copyright 2018
// Desc:  Helper functions for working with mutexes.

// Local headers
#include "mutexUtilities.h"

namespace MutexUtilities
{

AccessUpgrader::AccessUpgrader(std::shared_lock<std::shared_mutex>& sharedLock) : sharedLock(sharedLock)
{
	sharedLock.unlock();
	sharedLock.mutex()->lock();// Exclusive
}

AccessUpgrader::~AccessUpgrader()
{
	sharedLock.mutex()->unlock();
	sharedLock.lock();
}

}// MutexUtilities
