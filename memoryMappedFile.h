// File:  memoryMappedFile.h
// Date:  3/7/2019
// Auth:  K. Loux
// Desc:  Cross-platform memory-mapped file object.  Designed to support
//        sequentially reading each line of a file line-by-line.

#ifndef MEMORY_MAPPED_FILE_H_
#define MEMORY_MAPPED_FILE_H_

// Local headers
#include "uString.h"

#ifdef _WIN32
// Windows headers
#include <Windows.h>
#else
#endif// _WIN32

class MemoryMappedFile
{
public:
	MemoryMappedFile(const UString::String& fileName);
	~MemoryMappedFile();
	
	bool IsOpenAndGood() const;
	
	bool ReadNextLine(std::string& line);
	
private:
#ifdef _WIN32
	typedef HANDLE FileHandle;
	HANDLE mappingHandle = 0;
#else
	typedef int FileHandle;
#endif// _WIN32
	FileHandle fileHandle = 0;
	char* mappedView = nullptr;

	uint64_t currentOffset = 0;
	uint64_t maxSize;
	
	void HandleError(const std::string& message);
};

#endif// MEMORY_MAPPED_FILE_H_
