// File:  configFile.cpp
// Date:  9/27/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Generic (abstract) config file class.

// Standard C++ headers
#include <algorithm>

// Local headers
#include "configFile.h"

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
const std::string ConfigFile::commentCharacter	= "#";

//==========================================================================
// Class:			ConfigFile
// Function:		ReadConfiguration
//
// Description:		Reads the configuration from file.
//
// Input Arguments:
//		fileName	= const std::string&
//
// Output Arguments:
//		None
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::ReadConfiguration(const std::string &fileName)
{
	if (configItems.size() == 0)
		BuildConfigItems();

	AssignDefaults();

	outStream << "Reading configuration from '" << fileName << "'" << std::endl;

	std::ifstream file(fileName.c_str(), std::ios::in);
	if (!file.is_open() || !file.good())
	{
		outStream << "Unable to open file '" << fileName << "' for input" << std::endl;
		return false;
	}

	std::string line, field, data;
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
//		key		= const std::string&
//		data	= bool&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::AddConfigItem(const std::string &key, bool& data)
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
//		key		= const std::string&
//		data	= std::string&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::AddConfigItem(const std::string &key, std::string& data)
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
//		key		= const std::string&
//		data	= std::vector<std::string>&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::AddConfigItem(const std::string &key, std::vector<std::string>& data)
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
//		line	= const std::string&
//
// Output Arguments:
//		field	= std::string&
//		data	= std::string&
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::SplitFieldFromData(const std::string &line,
	std::string &field, std::string &data)
{
	// Break out the line into a field and the data (data may
	// contain spaces or equal sign!)
	size_t spaceLoc = line.find_first_of(" ");
	size_t equalLoc = line.find_first_of("=");
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
	spaceLoc = line.find_first_not_of(" ", startData);
	equalLoc = line.find_first_not_of("=", startData);

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
//		field	= const std::string&
//		data	= const std::string&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::ProcessConfigItem(const std::string &field, const std::string &data)
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
//		data	= const std::string&
//
// Output Arguments:
//		value	= bool&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::BoolReader(const std::string &data, bool &value)
{
	value = data.compare("1") == 0 || data.empty();
	return true;
}

//==========================================================================
// Class:			ConfigFile
// Function:		StringReader
//
// Description:		Reads the specified data into another string.
//
// Input Arguments:
//		data	= const std::string&
//
// Output Arguments:
//		value	= std::string&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::StringReader(const std::string &data, std::string &value)
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
//		data	= const std::string&
//
// Output Arguments:
//		value	= std::string&
//
// Return Value:
//		bool, true for success, false otherwise
//
//==========================================================================
bool ConfigFile::StringVectorReader(const std::string &data, std::vector<std::string> &value)
{
	std::string s;
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
//		s	= std::string&
//
// Output Arguments:
//		s	= std::string&
//
// Return Value:
//		None
//
//==========================================================================
void ConfigFile::StripCarriageReturn(std::string &s) const
{
	if (!s.empty() && *s.rbegin() == '\r')
		s.erase(s.length() - 1);
}