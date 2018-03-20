// File:  mutexUtilities.h
// Date:  3/20/2018
// Auth:  K. Loux
// Copy:  (c) Copyright 2018
// Desc:  Helper functions for working with mutexes.

#ifndef MUTEX_UTILITIES_H_
#define MUTEX_UTILITIES_H_

// Standard C++ headers
#include <shared_mutex>
#include <mutex>
#include <condition_variable>

namespace MutexUtilities
{

class AccessUpgrader
{
public:
	AccessUpgrader(std::shared_lock<std::shared_mutex>& sharedLock);
	~AccessUpgrader();

private:
	std::shared_lock<std::shared_mutex>& sharedLock;
};

// This class is helpful when a multi-threaded process may have two threads
// performing the same action unnecessarily.  When a thread begins an action,
// we note this in a list, so subsequent callers can check to see if they
// should begin the same action or not.
class AccessManager
{
public:
	AccessManager();

	bool TryAccess(const String& key);
	void WaitOn(const String& key);

	class AccessHelper
	{
	public:
		AccessHelper(const String& key, AccessManager& manager) : key(key), manager(manager) {}
		~AccessHelper() { manager.Notify(key); }

	private:
		const String key;
		AccessManager& manager;
	};

private:
	std::vector<String> list;
	std::shared_mutex listMutex;
	std::condition_variable accessFinishedCondition;

	void Notify(const String& key);
};

}// MutexUtilities

#endif// MUTEX_UTILITIES_H_
