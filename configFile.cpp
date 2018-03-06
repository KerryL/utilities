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
const String ConfigFile::commentCharacter = _T("#");

//==========================================================================
// Class:			ConfigFile
// Function:		ReadConfiguration
//
// Description:		Reads the configuration from file.
//
// Input Arguments:
//		fileName	= const String&
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::ReadConfiguration(const String &fileName)
{
	if (configItems.size() == 0)
		BuildConfigItems();

	AssignDefaults();

	outStream << "Reading configuration from '" << fileName << "'" << std::endl;

	IFStream file(fileName.c_str(), std::ios::in);
	if (!file.is_open() || !file.good())
	{
		outStream << "Unable to open file '" << fileName << "' for input" << std::endl;
		return false;
	}

	String line, field, data;
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
//		key		= const String&
//		data	= bool&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::AddConfigItem(const String &key, bool& data)
{
	AddConfigItem(key, data, BoolReader);
}

//==========================================================================
// Class:			ConfigFile
// Function:		AddConfigItem
//
// Description:		Adds the specified field key and data reference to the list.
//
// Input Arguments:
//		key		= const String&
//		data	= String&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::AddConfigItem(const String &key, String& data)
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
//		data	= std::vector<String>&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::AddConfigItem(const String &key, std::vector<String>& data)
{
	AddConfigItem(key, data, StringVectorReader);
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
//		line	= const String&
//
// Output Arguments:
//		field	= String&
//		data	= String&
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::SplitFieldFromData(const String &line,
	String &field, String &data)
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
//		field	= const String&
//		data	= const String&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::ProcessConfigItem(const String &field, const String &data)
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
//		data	= const String&
//
// Output Arguments:
//		value	= bool&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::BoolReader(const String &data, bool &value)
{
	value = data.compare(_T("1")) == 0 || data.empty();
	return true;
}

//==========================================================================
// Class:			ConfigFile
// Function:		StringReader
//
// Description:		Reads the specified data into another string.
//
// Input Arguments:
//		data	= const String&
//
// Output Arguments:
//		value	= String&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::StringReader(const String &data, String &value)
{
	value = data;
	return true;
}

//==========================================================================
// Class:			ConfigFile
// Function:		StringReader
//
// Description:		Reads the specified data into another string.
//
// Input Arguments:
//		data	= const String&
//
// Output Arguments:
//		value	= String&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::StringVectorReader(const String &data, std::vector<String> &value)
{
	String s;
	if (!StringReader(data, s))
		return false;
	value.push_back(s);
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
//		s	= String&
//
// Output Arguments:
//		s	= String&
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::StripCarriageReturn(String &s) const
{
	if (!s.empty() && *s.rbegin() == '\r')
		s.erase(s.length() - 1);
}