// File:  mutexUtilities.cpp
// Date:  3/20/2018
// Auth:  K. Loux
// Copy:  (c) Copyright 2018
// Desc:  Helper functions for working with mutexes.

// Local headers
#include "mutexUtilities.h"

// Standard C++ headers
#include <algorithm>

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

bool AccessManager::TryAccess(const String& key)
{
	std::shared_lock<std::shared_mutex> lock(listMutex);
	auto it(std::find(list.begin(), list.end(), key));
	if (it == list.end())
	{
		AccessUpgrader exclusiveLock(lock);
		if (std::find(list.begin(), list.end(), key) == list.end())
		{
			list.push_back(key);
			return true;
		}
	}

	return false;
}

void AccessManager::WaitOn(const String& key)
{
	std::unique_lock<std::shared_mutex> lock(listMutex);
	accessFinishedCondition.wait(lock, [key, this]()
	{
		return std::find(list.begin(), list.end(), key) == list.end();
	});
}

void AccessManager::Notify(const String& key)
{
	std::lock_guard<std::shared_mutex> lock(listMutex);
	list.erase(std::find(list.begin(), list.end(), key));
	accessFinishedCondition.notify_all();
}

}// MutexUtilities
