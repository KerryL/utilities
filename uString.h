// File:  uString.h
// Date:  3/6/2017
// Auth:  K. Loux
// Desc:  String definitions for handling unicode/non-unicode builds.

// Standard C++ headers
#include <string>
#include <fstream>
#include <sstream>
#include <regex>
#include <locale>
#include <codecvt>

#ifndef U_STRING_H_
#define U_STRING_H_

#ifdef UNICODE

#ifndef _T
#define _T(x) L##x
#endif

extern std::wostream& Cout;
extern std::wostream& Cerr;
extern std::wistream& Cin;

typedef wchar_t Char;
typedef std::wstring String;
typedef std::wfstream FStream;
typedef std::wifstream IFStream;
typedef std::wofstream OFStream;
typedef std::wstringstream StringStream;
typedef std::wostringstream OStringStream;
typedef std::wistringstream IStringStream;
typedef std::wostream OStream;
typedef std::wistream IStream;
typedef std::wregex RegEx;

#else

#ifndef _T
#define _T(x) x
#endif

extern std::ostream& Cout;
extern std::ostream& Cerr;
extern std::istream& Cin;

typedef char Char;
typedef std::string String;
typedef std::fstream FStream;
typedef std::ifstream IFStream;
typedef std::ofstream OFStream;
typedef std::stringstream StringStream;
typedef std::ostringstream OStringStream;
typedef std::istringstream IStringStream;
typedef std::ostream OStream;
typedef std::istream IStream;
typedef std::regex RegEx;

#endif// UNICODE

namespace UString
{

inline String ToStringType(const std::string& narrow)
{
	String newString(narrow.length(), Char(' '));
	std::copy(narrow.begin(), narrow.end(), newString.begin());
	return newString;
}

template<class S>
std::string ToNarrowString(const typename std::enable_if<std::is_same<String, std::string>::value, S>::type& s)
{
	return s;
}

template<class S>
std::string ToNarrowString(const typename std::enable_if<!std::is_same<String, std::string>::value, S>::type& s)
{
	std::wstring_convert<std::codecvt_utf8<Char>, Char> converter;
	return converter.to_bytes(s);
}

}// UString

#endif// U_STRING_H_
