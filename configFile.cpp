// File:  configFile.cpp
// Date:  9/27/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Generic (abstract) config file class.

// Local headers
#include "configFile.h"

// Standard C++ headers
#include <algorithm>

//==========================================================================
// Class:			ConfigFile
// Function:		Constant definitions
//
// Description:		Constant definitions for ConfigFile class.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
const UString::String ConfigFile::commentCharacter = _T("#");

//==========================================================================
// Class:			ConfigFile
// Function:		ReadConfiguration
//
// Description:		Reads the configuration from file.
//
// Input Arguments:
//		fileName	= const UString::String&
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::ReadConfiguration(const UString::String &fileName)
{
	if (configItems.size() == 0)
		BuildConfigItems();

	AssignDefaults();

	outStream << "Reading configuration from '" << fileName << "'" << std::endl;

	UString::IFStream file(UString::ToNarrowString(fileName).c_str());
	if (!file.is_open() || !file.good())
	{
		outStream << "Unable to open file '" << fileName << "' for input" << std::endl;
		return false;
	}

	UString::String line, field, data;
	size_t inLineComment;

	while (std::getline(file, line))
	{
		StripCarriageReturn(line);
		if (line.empty() ||
			commentCharacter.compare(line.substr(0,1)) == 0)
			continue;

		inLineComment = line.find(commentCharacter);
		if (inLineComment != std::string::npos)
			line = line.substr(0, inLineComment);

		SplitFieldFromData(line, field, data);
		ProcessConfigItem(field, data);
	}

	file.close();

	return ConfigIsOK();
}

//==========================================================================
// Class:			ConfigFile
// Function:		AddConfigItem
//
// Description:		Adds the specified field key and data reference to the list.
//
// Input Arguments:
//		key		= const UString::String&
//		data	= bool&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::AddConfigItem(const UString::String &key, bool& data)
{
	AddConfigItem(key, data, BoolReader);
}

//==========================================================================
// Class:			ConfigFile
// Function:		StringReader
//
// Description:		Reads the specified data into another string.
//
// Input Arguments:
//		data	= const UString::String&
//
// Output Arguments:
//		value	= std::string&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::StringReader(const UString::String &data, std::string &value)
{
	value = UString::ToNarrowString(data);
	return true;
}

//==========================================================================
// Class:			ConfigFile
// Function:		StringReader
//
// Description:		Reads the specified data into another string.
//
// Input Arguments:
//		data	= const UString::String&
//
// Output Arguments:
//		value	= std::wstring&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::StringReader(const UString::String &data, std::wstring &value)
{
	value = UString::ToWideString(data);
	return true;
}

//==========================================================================
// Class:			ConfigFile
// Function:		StringVectorReader
//
// Description:		Reads the specified data into a string vector.
//
// Input Arguments:
//		data	= const UString::String&
//
// Output Arguments:
//		value	= std::vector<std::string>&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::StringVectorReader(const UString::String &data, std::vector<std::string> &value)
{
	UString::String s;
	if (!StringReader(data, s))
		return false;
	value.push_back(UString::ToNarrowString(s));
	return true;
}

//==========================================================================
// Class:			ConfigFile
// Function:		StringVectorReader
//
// Description:		Reads the specified data into s string vector.
//
// Input Arguments:
//		data	= const UString::String&
//
// Output Arguments:
//		value	= std::vector<std::wstring>&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::StringVectorReader(const UString::String &data, std::vector<std::wstring> &value)
{
	UString::String s;
	if (!StringReader(data, s))
		return false;
	value.push_back(UString::ToWideString(s));
	return true;
}

//==========================================================================
// Class:			ConfigFile
// Function:		SplitFieldFromData
//
// Description:		Splits the current line into a field portion and a data
//					portion.  Split occurs at first space or first equal sign
//					(whichever comes first - keys may not contain equal signs
//					or spaces).
//
// Input Arguments:
//		line	= const UString::String&
//
// Output Arguments:
//		field	= UString::String&
//		data	= UString::String&
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::SplitFieldFromData(const UString::String &line,
	UString::String &field, UString::String &data)
{
	// Break out the line into a field and the data (data may
	// contain spaces or equal sign!)
	size_t spaceLoc = line.find_first_of(_T(" "));
	size_t equalLoc = line.find_first_of(_T("="));
	field = line.substr(0, std::min(spaceLoc, equalLoc));

	if (spaceLoc == std::string::npos &&
		equalLoc == std::string::npos)
	{
		// Special case where no data portion was provided
		data.clear();
		return;
	}
	else if (spaceLoc == std::string::npos)
		spaceLoc = equalLoc;
	else if (equalLoc == std::string::npos)
		equalLoc = spaceLoc;

	size_t startData = std::max(spaceLoc, equalLoc) + 1;
	spaceLoc = line.find_first_not_of(_T(" "), startData);
	equalLoc = line.find_first_not_of(_T("="), startData);

	if (spaceLoc == std::string::npos)
		spaceLoc = equalLoc;
	else if (equalLoc == std::string::npos)
		equalLoc = spaceLoc;

	startData = std::max(spaceLoc, equalLoc);
	data = line.substr(startData, line.length() - startData);
}

//==========================================================================
// Class:			ConfigFile
// Function:		ProcessConfigItem
//
// Description:		Processes the specified configuration item.
//
// Input Arguments:
//		field	= const UString::String&
//		data	= const UString::String&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::ProcessConfigItem(const UString::String &field, const UString::String &data)
{
	if (configItems.count(field) > 0)
		(*configItems.find(field)).second->AssignValue(data);
	else
		outStream << "Unknown config field: " << field << std::endl;
}

//==========================================================================
// Class:			ConfigFile
// Function:		BoolReader
//
// Description:		Reads the specified data into boolean form.  Interprets true
//					values when data is "1" or empty (boolean fields should be
//					named such that it is apparent that no data value will be
//					interpreted as true).
//
// Input Arguments:
//		data	= const UString::String&
//
// Output Arguments:
//		value	= bool&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::BoolReader(const UString::String &data, bool &value)
{
	value = data.compare(_T("1")) == 0 || data.empty();
	return true;
}

//==========================================================================
// Class:			ConfigFile
// Function:		StripCarriageReturn
//
// Description:		Removes the '\r' from the end of the string, in case
//					we're reading Windows-generated files on a Linux system.
//
// Input Arguments:
//		s	= UString::String&
//
// Output Arguments:
//		s	= UString::String&
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::StripCarriageReturn(UString::String &s) const
{
	if (!s.empty() && *s.rbegin() == '\r')
		s.erase(s.length() - 1);
}
