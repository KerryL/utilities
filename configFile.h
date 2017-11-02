// File:  configFile.h
// Date:  9/27/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Generic (abstract) config file class.

#ifndef CONFIG_FILE_H_
#define CONFIG_FILE_H_

// Standard C++ headers
#include <string>
#include <iostream>
#include <map>
#include <sstream>
#include <cstring>
#include <cassert>
#include <fstream>
#include <vector>
#include <errno.h>
#include <typeinfo>
#include <memory>

class ConfigFile
{
public:
	ConfigFile(std::ostream &outStream = std::cout)
		: outStream(outStream) {}
	virtual ~ConfigFile() = default;

	bool ReadConfiguration(const std::string &fileName);
	template <typename T>
	bool WriteConfiguration(const std::string &fileName,
		const std::string &field, const T &value);

protected:
	std::ostream& outStream;
	
	virtual void BuildConfigItems() = 0;
	virtual void AssignDefaults() = 0;

	virtual bool ConfigIsOK() = 0;
	
	class ConfigItemBase
	{
	public:
		virtual ~ConfigItemBase() {}
		virtual bool AssignValue(const std::string &/*data*/) { return false; }
		
	protected:
		ConfigItemBase() {}
	};

	template <typename T>
	class ConfigItem : public ConfigItemBase
	{
	public:
		typedef bool (*ReadFunction)(const std::string &s, T &value);
		
		ConfigItem(T &value, ReadFunction reader) : ConfigItemBase(),
			value(value), reader(reader) {}
		virtual ~ConfigItem() = default;

		virtual bool AssignValue(const std::string &data);

	private:
		T &value;
		ReadFunction reader;
	};
	
	void AddConfigItem(const std::string &key, bool& data);
	void AddConfigItem(const std::string &key, std::string& data);
	void AddConfigItem(const std::string &key, std::vector<std::string>& data);
	template <typename T>
	void AddConfigItem(const std::string &key, T& data);
	template <typename T>
	void AddConfigItem(const std::string &key, std::vector<T>& data);
	template <typename T>
	void AddConfigItem(const std::string &key, T& data,
		typename ConfigItem<T>::ReadFunction reader);

	template <typename T>
	std::string GetKey(const T& i) const { return (*keyMap.find((void* const)&i)).second; }
	
	static bool BoolReader(const std::string &s, bool &value);
	static bool StringReader(const std::string &s, std::string &value);
	static bool StringVectorReader(const std::string &s, std::vector<std::string> &value);
	template <typename T>
	static bool GenericReader(const std::string &s, T &value);
	template <typename T>
	static bool VectorReader(const std::string &s, std::vector<T> &v);

private:
	static const std::string commentCharacter;

	void StripCarriageReturn(std::string &s) const;
	void SplitFieldFromData(const std::string &line, std::string &field, std::string &data);
	void ProcessConfigItem(const std::string &field, const std::string &data);

	typedef std::map<std::string, std::unique_ptr<ConfigItemBase>> ConfigItemMap;
	ConfigItemMap configItems;
	std::map<void* const, std::string> keyMap;
};

//==========================================================================
// Class:			ConfigFile
// Function:		AddConfigItem
//
// Description:		Adds the specified field key and data reference to the list.
//
// Input Arguments:
//		key		= const std::string&
//		data	= T&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
template <typename T>
void ConfigFile::AddConfigItem(const std::string &key, T& data)
{
	AddConfigItem(key, data, GenericReader<T>);
}

//==========================================================================
// Class:			ConfigFile
// Function:		AddConfigItem
//
// Description:		Adds the specified field key and data reference to the list.
//
// Input Arguments:
//		key		= const std::string&
//		data	= std::vector<T>&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
template <typename T>
void ConfigFile::AddConfigItem(const std::string &key, std::vector<T>& data)
{
	AddConfigItem(key, data, VectorReader<T>);
}

//==========================================================================
// Class:			ConfigFile
// Function:		AddConfigItem
//
// Description:		Adds the specified field key and data reference to the list.
//
// Input Arguments:
//		key		= const std::string&
//		data	= T&
//		reader	= ConfigItem<T>::ReadFunction
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
template <typename T>
void ConfigFile::AddConfigItem(const std::string &key, T& data,
	typename ConfigItem<T>::ReadFunction reader)
{
	bool success = configItems.insert(std::make_pair(key, std::make_unique<ConfigItem<T>>(data, reader))).second;
	assert(success && "failed to add item to forward map");// TODO:  Do something better here

	success = keyMap.insert(std::make_pair((void*)&data, key)).second;
	assert(success && "failed to add item to reverse map");// TODO:  Do something better here
}

//==========================================================================
// Class:			ConfigFile
// Function:		WriteConfiguration
//
// Description:		Writes the specified field to the config file.  Maintains
//					existing whitespace, comments, formatting, etc.
//
// Input Arguments:
//		fileName	= const std::string&
//		field		= const std::string&
//		value		= const T&
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
template <typename T>
bool ConfigFile::WriteConfiguration(const std::string &fileName,
	const std::string &field, const T &value)
{
	std::ifstream inFile(fileName.c_str(), std::ios::in);
	if (!inFile.is_open() || !inFile.good())
	{
		outStream << "Failed to open '" << fileName << "'" << std::endl;
		return false;
	}

	std::string tempFileName("tempConfigFile");
	std::ofstream outFile(tempFileName.c_str(), std::ios::out);
	if (!outFile.is_open() || !outFile.good())
	{
		outStream << "Failed to open '" << tempFileName << "'" << std::endl;
		return false;
	}

	std::string line, comment, currentField, data;
	size_t inLineComment;

	bool written(false);
	while (std::getline(inFile, line))
	{
		if (!written && !line.empty() &&
			commentCharacter.compare(line.substr(0,1)) != 0)
		{
			inLineComment = line.find(commentCharacter);
			if (inLineComment != std::string::npos)
			{
				comment = line.substr(inLineComment);
				line = line.substr(0, inLineComment);
			}
			else
				comment.clear();

			SplitFieldFromData(line, currentField, data);
			if (currentField.compare(field) == 0)
			{
				std::stringstream ss;
				ss << field << " = " << value;
				line = ss.str();
				written = true;
			}
		}

		outFile << line << comment << std::endl;
	}

	if (!written)
		outFile << field << " = " << value << std::endl;

	inFile.close();
	outFile.close();
	if (std::remove(fileName.c_str()) == -1)
	{
		outStream << "Failed to delete '" << fileName
			<< "':  " << std::strerror(errno) << std::endl;
		return false;
	}

	if (std::rename(tempFileName.c_str(), fileName.c_str()) == -1)
	{
		outStream << "Failed to rename '" << tempFileName
			<< "' to '" << fileName << "':  " << std::strerror(errno) << std::endl;
		return false;
	}

	return true;
}

//==========================================================================
// Class:			ConfigFile
// Function:		GenericReader
//
// Description:		Generic string-to-type function for reading config data.
//
// Input Arguments:
//		s	= const std::string&
//
// Output Arguments:
//		value	= const T&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
template <typename T>
bool ConfigFile::GenericReader(const std::string &s, T &value)
{
	std::stringstream ss(s);
	if (typeid(T) == typeid(std::string))
	{
		// TODO:  getline or something here so we don't break on/loose spaces
	}
	// TODO:  Would be nice to recognize vectors and bools here, too, so we don't have to specify readers for these
	
	return !(ss >> value).fail();
}

//==========================================================================
// Class:			ConfigFile
// Function:		VectorReader
//
// Description:		Generic string-to-type function for reading config data and
//					adding each new entry to a vector.
//
// Input Arguments:
//		s	= const std::string&
//
// Output Arguments:
//		v	= const std::vector<T>&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
template <typename T>
bool ConfigFile::VectorReader(const std::string &s, std::vector<T> &v)
{
	T value;
	if (!GenericReader(s, value))
		return false;
		
	v.push_back(value);
	return true;
}

//==========================================================================
// Class:			ConfigItem
// Function:		AssignValue
//
// Description:		Assigns the value of the data string to the appropriate
//					dereferenced pointed, based on the this item's type.
//
// Input Arguments:
//		dataString	= const std::string&
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
template <typename T>
bool ConfigFile::ConfigItem<T>::AssignValue(const std::string &dataString)
{
	return reader(dataString, value);
}

#endif// CONFIG_FILE_H_
