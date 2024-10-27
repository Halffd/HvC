#ifndef TYPES_H
#define TYPES_H
#include <string>
#include <map>
#include <windows.h>
#include "Printer.h"

using wID = HWND;
using str = std::string;
using cstr = const str&;
using group = std::map<str, str>;
using pID = DWORD;  
#endif // TYPES_H