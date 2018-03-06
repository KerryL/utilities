// File:  uString.cpp
// Date:  3/6/2017
// Auth:  K. Loux
// Desc:  String definitions for handling unicode/non-unicode builds.

// Local headers
#include "uString.h"

// Standard C++ headers
#include <iostream>

#ifdef UNICODE

std::wostream& Cout(std::wcout);
std::wostream& Cerr(std::wcerr);
std::wistream& Cin(std::wcin);

#else

std::ostream& Cout(std::cout);
std::ostream& Cerr(std::cerr);
std::istream& Cin(std::cin);

#endif// UNICODE
