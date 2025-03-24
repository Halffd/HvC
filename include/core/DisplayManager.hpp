#pragma once

#ifdef __linux__
#include <X11/Xlib.h>
#endif

namespace H {
    class DisplayManager {
    public:
        static bool Initialize();
        static void Close();
        
        #ifdef __linux__
        static Display* GetDisplay();
        static Window GetRootWindow();
        #endif
        
    private:
        #ifdef __linux__
        static Display* display;
        static Window rootWindow;
        #endif
    };
} 