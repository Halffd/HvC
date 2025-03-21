#pragma once

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xrandr.h>
#include <X11/XF86keysym.h>
#endif

// Common typedefs to avoid redefinition errors
#ifndef Key_typedef
#define Key_typedef
typedef unsigned long Key;
#endif
