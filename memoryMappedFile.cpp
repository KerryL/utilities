// File:  memoryMappedFile.cpp
// Date:  3/7/2019
// Auth:  K. Loux
// Desc:  Cross-platform memory-mapped file object.  Designed to support
//        sequentially reading each line of a file line-by-line.

// Local headers
#include "memoryMappedFile.h"

#ifdef _WIN32
// Windows headers
#include "Memoryapi.h"
#else
#endif// _WIN32

// Standard C++ headers
#include <iostream>

MemoryMappedFile::MemoryMappedFile(const UString::String& fileName)
{
	fileHandle = CreateFile(fileName.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		std::ostringstream ss;
		ss << GetLastError();
		throw std::exception(("Failed to create file handle; error = " + ss.str()).c_str());
	}
	
	mappingHandle = CreateFileMapping(fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if (!mappingHandle)
	{
		std::ostringstream ss;
		ss << GetLastError();
		throw std::exception(("Failed to create mapping; error = " + ss.str()).c_str());
	}

	mappedView = reinterpret_cast<char*>(MapViewOfFile(mappingHandle, FILE_MAP_READ, 0, 0, 0));
	if (!mappedView)
	{
		std::ostringstream ss;
		ss << GetLastError();
		throw std::exception(("Failed to create view; error = " + ss.str()).c_str());
	}

	DWORD highOrderSize;
	auto lowOrderSize(GetFileSize(fileHandle, &highOrderSize));
	maxSize = (static_cast<uint64_t>(highOrderSize) << 32) + lowOrderSize;
}

MemoryMappedFile::~MemoryMappedFile()
{
	if (mappedView)
	{
		if (!UnmapViewOfFile(mappedView))
		{
			// TODO:  Anything here?
		}
	}

	if (mappingHandle)
	{
		if (CloseHandle(mappingHandle) == 0)
		{
			// TODO:  Anything here?
		}
	}
	
	if (fileHandle)
	{
		if (CloseHandle(fileHandle) == 0)
		{
			// TODO:  Anything here?
		}
	}
}

bool MemoryMappedFile::ReadNextLine(std::string& line)
{
	for (uint64_t i = currentOffset; i < maxSize; ++i)
	{
		if (mappedView[i] == '\n')
		{
			line = std::string(mappedView + currentOffset, static_cast<size_t>(i - currentOffset));
			currentOffset = i + 1;
			return true;
		}
	}

	return false;
}
