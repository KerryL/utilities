// File:  memoryMappedFile.cpp
// Date:  3/7/2019
// Auth:  K. Loux
// Desc:  Cross-platform memory-mapped file object.  Designed to support
//        sequentially reading each line of a file line-by-line.

// Local headers
#include "memoryMappedFile.h"

#ifdef _WIN32
// Windows headers
#include <Memoryapi.h>
#else
// Linux headers
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#endif// _WIN32

// Standard C++ headers
#include <iostream>

MemoryMappedFile::MemoryMappedFile(const UString::String& fileName)
{
#ifdef _WIN32
	fileHandle = CreateFile(fileName.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (fileHandle == INVALID_HANDLE_VALUE)
		HandleError("Failed to create file handle");
	
	mappingHandle = CreateFileMapping(fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if (!mappingHandle)
		HandleError("Failed to create mapping handle");

	mappedView = reinterpret_cast<char*>(MapViewOfFile(mappingHandle, FILE_MAP_READ, 0, 0, 0));
	if (!mappedView)
		HandleError("Failed to create view");

	DWORD highOrderSize;
	auto lowOrderSize(GetFileSize(fileHandle, &highOrderSize));
	maxSize = (static_cast<uint64_t>(highOrderSize) << 32) + lowOrderSize;
	
#else

	fileHandle = open(UString::ToNarrowString(fileName).c_str(), O_RDONLY);
	if (fileHandle == -1)
		HandleError("Failed to create file handle");

	struct stat statInfo;
	if (fstat(fileHandle, &statInfo) == -1)
		HandleError("Failed to get file size");
	maxSize = statInfo.st_size;
	
	mappedView = static_cast<char*>(mmap(nullptr, maxSize, PROT_READ, MAP_PRIVATE, fileHandle, 0));
	if (mappedView == MAP_FAILED)
		HandleError("Failed to create mapping handle");
#endif// _WIN32
}

MemoryMappedFile::~MemoryMappedFile()
{
#ifdef _WIN32
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
#else
	if (mappedView)
		munmap(mappedView, maxSize);
		
	if (fileHandle)
		close(fileHandle);
#endif
}

bool MemoryMappedFile::IsOpenAndGood() const
{
#ifdef _WIN32
	return fileHandle != INVALID_HANDLE_VALUE && mappingHandle && mappedView;
#else
	return fileHandle != -1 && mappedView;
#endif
}

void MemoryMappedFile::HandleError(const std::string& message)
{
	std::ostringstream ss;
#ifdef _WIN32
	ss << GetLastError();
#else
	ss << strerror(errno);
#endif
	throw std::runtime_error((message + "; error = " + ss.str()).c_str());
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
