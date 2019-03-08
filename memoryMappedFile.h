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
#include "Windows.h"
#else
#endif// _WIN32

class MemoryMappedFile
{
public:
	MemoryMappedFile(const UString::String& fileName);
	~MemoryMappedFile();
	
	inline bool IsOpenAndGood() const { return fileHandle != INVALID_HANDLE_VALUE && mappingHandle && mappedView; }
	
	bool ReadNextLine(std::string& line);
	
private:
	HANDLE fileHandle = 0;
	HANDLE mappingHandle = 0;
	char* mappedView = nullptr;

	uint64_t currentOffset = 0;
	uint64_t maxSize;
};

#endif// MEMORY_MAPPED_FILE_H_
