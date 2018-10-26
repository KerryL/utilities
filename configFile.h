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
	explicit ConfigFile(UString::OStream &outStream = Cout)
		: outStream(outStream) {}
	virtual ~ConfigFile() = default;

	bool ReadConfiguration(const UString::String &fileName);
	template <typename T>
	bool WriteConfiguration(const UString::String &fileName,
		const UString::String &field, const T &value);

protected:
	UString::OStream& outStream;
	
	virtual void BuildConfigItems() = 0;
	virtual void AssignDefaults() = 0;

	virtual bool ConfigIsOK() = 0;
	
	class ConfigItemBase
	{
	public:
		virtual ~ConfigItemBase() {}
		virtual bool AssignValue(const UString::String &/*data*/) { return false; }
		
	protected:
		ConfigItemBase() {}
	};

	template <typename T>
	class ConfigItem : public ConfigItemBase
	{
	public:
		typedef bool (*ReadFunction)(const UString::String &s, T &value);
		
		ConfigItem(T &value, ReadFunction reader) : ConfigItemBase(),
			value(value), reader(reader) {}
		virtual ~ConfigItem() = default;

		virtual bool AssignValue(const UString::String &data);

	private:
		T &value;
		ReadFunction reader;
	};
	
	void AddConfigItem(const UString::String& key, bool& data);
	template <typename T>
	void AddConfigItem(const UString::String& key, std::basic_string<T>& data);
	template <typename T>
	void AddConfigItem(const UString::String& key, std::vector<std::basic_string<T>>& data);
	template <typename T, typename std::enable_if<!std::is_enum<T>::value>::type>
	void AddConfigItem(const UString::String& key, T& data);
	template <typename T>
	void AddConfigItem(const UString::String& key, std::vector<T>& data);
	template <typename T, typename std::enable_if<!std::is_enum<T>::value, int>::type = 0>
	void AddConfigItem(const UString::String& key, T& data);
	template <typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
	void AddConfigItem(const UString::String& key, T& data);
	template <typename T>
	void AddConfigItem(const UString::String &key, T& data,
		typename ConfigItem<T>::ReadFunction reader);

	template <typename T>
	UString::String GetKey(const T& i) const { return (*keyMap.find((void* const)&i)).second; }
	
	static bool BoolReader(const UString::String& s, bool& value);
	static bool StringReader(const UString::String& s, std::string& value);
	static bool StringVectorReader(const UString::String& s, std::vector<std::string>& value);
	static bool StringReader(const UString::String& s, std::wstring& value);
	static bool StringVectorReader(const UString::String& s, std::vector<std::wstring>& value);
	template <typename T>
	static bool GenericReader(const UString::String& s, T& value);
	template <typename T>
	static bool VectorReader(const UString::String& s, std::vector<T>& v);
	template <typename T>
	static bool EnumReader(const UString::String& s, T& value);

private:
	static const UString::String commentCharacter;

	void StripCarriageReturn(UString::String &s) const;
	void SplitFieldFromData(const UString::String &line, UString::String &field, UString::String &data);
	void ProcessConfigItem(const UString::String &field, const UString::String &data);

	typedef std::map<UString::String, std::unique_ptr<ConfigItemBase>> ConfigItemMap;
	ConfigItemMap configItems;
	std::map<void* const, UString::String> keyMap;
};

//==========================================================================
// Class:			ConfigFile
// Function:		AddConfigItem
//
// Description:		Adds the specified field key and data reference to the list.
//
// Input Arguments:
//		key		= const UString::String&
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
void ConfigFile::AddConfigItem(const UString::String &key, T& data)
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
//		key		= const UString::String&
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
void ConfigFile::AddConfigItem(const UString::String& key, T& data)
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
//		key		= const UString::String&
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
void ConfigFile::AddConfigItem(const UString::String &key, std::vector<T>& data)
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
//		key		= const UString::String&
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
void ConfigFile::AddConfigItem(const UString::String &key, T& data,
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
//		key		= const UString::String&
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
void ConfigFile::AddConfigItem(const UString::String &key, std::basic_string<T>& data)
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
//		key		= const UString::String&
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
void ConfigFile::AddConfigItem(const UString::String &key, std::vector<std::basic_string<T>>& data)
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
//		fileName	= const UString::String&
//		field		= const UString::String&
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
bool ConfigFile::WriteConfiguration(const UString::String &fileName,
	const UString::String &field, const T &value)
{
	UString::IFStream inFile(fileName.c_str(), std::ios::in);
	if (!inFile.is_open() || !inFile.good())
	{
		outStream << "Failed to open '" << fileName << "'" << std::endl;
		return false;
	}

	UString::String tempFileName("tempConfigFile");
	UString::OFStream outFile(tempFileName.c_str(), std::ios::out);
	if (!outFile.is_open() || !outFile.good())
	{
		outStream << "Failed to open '" << tempFileName << "'" << std::endl;
		return false;
	}

	UString::String line, comment, currentField, data;
	size_t inLineComment;

	bool written(false);
	while (std::getline(inFile, line))
	{
		if (!written && !line.empty() &&
			commentCharacter.compare(line.substr(0,1)) != 0)
		{
			inLineComment = line.find(commentCharacter);
			if (inLineComment != UString::String::npos)
			{
				comment = line.substr(inLineComment);
				line = line.substr(0, inLineComment);
			}
			else
				comment.clear();

			SplitFieldFromData(line, currentField, data);
			if (currentField.compare(field) == 0)
			{
				UString::StringStream ss;
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
	if (std::remove(UString::ToNarrowString(fileName).c_str()) == -1)
	{
		outStream << "Failed to delete '" << fileName
			<< "':  " << std::strerror(errno) << std::endl;
		return false;
	}

	if (std::rename(UString::ToNarrowString(tempFileName).c_str(),
		UString::ToNarrowString(fileName).c_str()) == -1)
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
// Description:		Generic UString::-to-type function for reading config data.
//
// Input Arguments:
//		s	= const UString::String&
//
// Output Arguments:
//		value	= const T&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
template <typename T>
bool ConfigFile::GenericReader(const UString::String &s, T &value)
{
	UString::IStringStream ss(s);
	return !(ss >> value).fail();
}

//==========================================================================
// Class:			ConfigFile
// Function:		EnumReader
//
// Description:		Generic UString::-to-type function for reading config data.
//
// Input Arguments:
//		s	= const UString::String&
//
// Output Arguments:
//		value	= const T&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
template <typename T>
bool ConfigFile::EnumReader(const UString::String &s, T &value)
{
	UString::IStringStream ss(s);
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
// Description:		Generic UString::-to-type function for reading config data and
//					adding each new entry to a vector.
//
// Input Arguments:
//		s	= const UString::String&
//
// Output Arguments:
//		v	= const std::vector<T>&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
template <typename T>
bool ConfigFile::VectorReader(const UString::String &s, std::vector<T> &v)
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
// Description:		Assigns the value of the data UString::String to the appropriate
//					dereferenced pointed, based on the this item's type.
//
// Input Arguments:
//		dataString	= const UString::String&
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
template <typename T>
bool ConfigFile::ConfigItem<T>::AssignValue(const UString::String &dataString)
{
	return reader(dataString, value);
}

#endif// CONFIG_FILE_H_
