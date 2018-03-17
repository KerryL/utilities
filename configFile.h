// File:  configFile.h
// Date:  9/27/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Generic (abstract) config file class.

#ifndef CONFIG_FILE_H_
#define CONFIG_FILE_H_

// Local headers
#include "uString.h"

// Standard C++ headers
#include <iostream>
#include <map>
#include <cstring>
#include <cassert>
#include <vector>
#include <errno.h>
#include <typeinfo>
#include <type_traits>
#include <memory>

class ConfigFile
{
public:
	explicit ConfigFile(OStream &outStream = Cout)
		: outStream(outStream) {}
	virtual ~ConfigFile() = default;

	bool ReadConfiguration(const String &fileName);
	template <typename T>
	bool WriteConfiguration(const String &fileName,
		const String &field, const T &value);

protected:
	OStream& outStream;
	
	virtual void BuildConfigItems() = 0;
	virtual void AssignDefaults() = 0;

	virtual bool ConfigIsOK() = 0;
	
	class ConfigItemBase
	{
	public:
		virtual ~ConfigItemBase() {}
		virtual bool AssignValue(const String &/*data*/) { return false; }
		
	protected:
		ConfigItemBase() {}
	};

	template <typename T>
	class ConfigItem : public ConfigItemBase
	{
	public:
		typedef bool (*ReadFunction)(const String &s, T &value);
		
		ConfigItem(T &value, ReadFunction reader) : ConfigItemBase(),
			value(value), reader(reader) {}
		virtual ~ConfigItem() = default;

		virtual bool AssignValue(const String &data);

	private:
		T &value;
		ReadFunction reader;
	};
	
	void AddConfigItem(const String& key, bool& data);
	template <typename T>
	void AddConfigItem(const String& key, std::basic_string<T>& data);
	template <typename T>
	void AddConfigItem(const String& key, std::vector<std::basic_string<T>>& data);
	template <typename T, typename std::enable_if<!std::is_enum<T>::value>::type>
	void AddConfigItem(const String& key, T& data);
	template <typename T>
	void AddConfigItem(const String& key, std::vector<T>& data);
	template <typename T, typename std::enable_if<!std::is_enum<T>::value, int>::type = 0>
	void AddConfigItem(const String& key, T& data);
	template <typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
	void AddConfigItem(const String& key, T& data);
	template <typename T>
	void AddConfigItem(const String &key, T& data,
		typename ConfigItem<T>::ReadFunction reader);

	template <typename T>
	String GetKey(const T& i) const { return (*keyMap.find((void* const)&i)).second; }
	
	static bool BoolReader(const String& s, bool& value);
	static bool StringReader(const String& s, std::string& value);
	static bool StringVectorReader(const String& s, std::vector<std::string>& value);
	static bool StringReader(const String& s, std::wstring& value);
	static bool StringVectorReader(const String& s, std::vector<std::wstring>& value);
	template <typename T>
	static bool GenericReader(const String& s, T& value);
	template <typename T>
	static bool VectorReader(const String& s, std::vector<T>& v);
	template <typename T>
	static bool EnumReader(const String& s, T& value);

private:
	static const String commentCharacter;

	void StripCarriageReturn(String &s) const;
	void SplitFieldFromData(const String &line, String &field, String &data);
	void ProcessConfigItem(const String &field, const String &data);

	typedef std::map<String, std::unique_ptr<ConfigItemBase>> ConfigItemMap;
	ConfigItemMap configItems;
	std::map<void* const, String> keyMap;
};

//==========================================================================
// Class:			ConfigFile
// Function:		AddConfigItem
//
// Description:		Adds the specified field key and data reference to the list.
//
// Input Arguments:
//		key		= const String&
//		data	= T&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
template <typename T, typename std::enable_if<!std::is_enum<T>::value, int>::type>
void ConfigFile::AddConfigItem(const String &key, T& data)
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
//		key		= const String&
//		data	= T& (if enum)
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
template <typename T, typename std::enable_if<std::is_enum<T>::value, int>::type>
void ConfigFile::AddConfigItem(const String& key, T& data)
{
	AddConfigItem(key, data, EnumReader<T>);
}

//==========================================================================
// Class:			ConfigFile
// Function:		AddConfigItem
//
// Description:		Adds the specified field key and data reference to the list.
//
// Input Arguments:
//		key		= const String&
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
void ConfigFile::AddConfigItem(const String &key, std::vector<T>& data)
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
//		key		= const String&
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
void ConfigFile::AddConfigItem(const String &key, T& data,
	typename ConfigItem<T>::ReadFunction reader)
{
	bool success(configItems.insert(std::make_pair(key, std::make_unique<ConfigItem<T>>(data, reader))).second);
	assert(success && "failed to add item to forward map");// TODO:  Do something better here

	success = keyMap.insert(std::make_pair((void*)&data, key)).second;
	assert(success && "failed to add item to reverse map");// TODO:  Do something better here
}

//==========================================================================
// Class:			ConfigFile
// Function:		AddConfigItem
//
// Description:		Adds the specified field key and data reference to the list.
//
// Input Arguments:
//		key		= const String&
//		data	= std::basic_string<T>&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
template <typename T>
void ConfigFile::AddConfigItem(const String &key, std::basic_string<T>& data)
{
	AddConfigItem(key, data, StringReader);
}

//==========================================================================
// Class:			ConfigFile
// Function:		AddConfigItem
//
// Description:		Adds the specified field key and data reference to the list.
//
// Input Arguments:
//		key		= const String&
//		data	= std::vector<std::basic_string<T>>&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
template <typename T>
void ConfigFile::AddConfigItem(const String &key, std::vector<std::basic_string<T>>& data)
{
	AddConfigItem(key, data, StringVectorReader);
}

//==========================================================================
// Class:			ConfigFile
// Function:		WriteConfiguration
//
// Description:		Writes the specified field to the config file.  Maintains
//					existing whitespace, comments, formatting, etc.
//
// Input Arguments:
//		fileName	= const String&
//		field		= const String&
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
bool ConfigFile::WriteConfiguration(const String &fileName,
	const String &field, const T &value)
{
	IFStream inFile(fileName.c_str(), std::ios::in);
	if (!inFile.is_open() || !inFile.good())
	{
		outStream << "Failed to open '" << fileName << "'" << std::endl;
		return false;
	}

	String tempFileName("tempConfigFile");
	OFStream outFile(tempFileName.c_str(), std::ios::out);
	if (!outFile.is_open() || !outFile.good())
	{
		outStream << "Failed to open '" << tempFileName << "'" << std::endl;
		return false;
	}

	String line, comment, currentField, data;
	size_t inLineComment;

	bool written(false);
	while (std::getline(inFile, line))
	{
		if (!written && !line.empty() &&
			commentCharacter.compare(line.substr(0,1)) != 0)
		{
			inLineComment = line.find(commentCharacter);
			if (inLineComment != String::npos)
			{
				comment = line.substr(inLineComment);
				line = line.substr(0, inLineComment);
			}
			else
				comment.clear();

			SplitFieldFromData(line, currentField, data);
			if (currentField.compare(field) == 0)
			{
				StringStream ss;
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
//		s	= const String&
//
// Output Arguments:
//		value	= const T&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
template <typename T>
bool ConfigFile::GenericReader(const String &s, T &value)
{
	IStringStream ss(s);
	return !(ss >> value).fail();
}

//==========================================================================
// Class:			ConfigFile
// Function:		EnumReader
//
// Description:		Generic string-to-type function for reading config data.
//
// Input Arguments:
//		s	= const String&
//
// Output Arguments:
//		value	= const T&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
template <typename T>
bool ConfigFile::EnumReader(const String &s, T &value)
{
	IStringStream ss(s);
	typename std::underlying_type<T>::type tempValue;
	if ((ss >> tempValue).fail())
		return false;

	value = static_cast<T>(tempValue);
	return true;
}

//==========================================================================
// Class:			ConfigFile
// Function:		VectorReader
//
// Description:		Generic string-to-type function for reading config data and
//					adding each new entry to a vector.
//
// Input Arguments:
//		s	= const String&
//
// Output Arguments:
//		v	= const std::vector<T>&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
template <typename T>
bool ConfigFile::VectorReader(const String &s, std::vector<T> &v)
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
//		dataString	= const String&
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
template <typename T>
bool ConfigFile::ConfigItem<T>::AssignValue(const String &dataString)
{
	return reader(dataString, value);
}

#endif// CONFIG_FILE_H_
