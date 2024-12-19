#include "Window.hpp"
#include "IO.hpp"
#include <iostream>
#include <thread>

IO io;
void HvC(IO io){
    io.Hotkey("RWin", [&io]() {
        io.Send("Win");
    });
    io.Hotkey("w", [&io]() {
        io.Send("up");
    });
    io.Hotkey("a", [&io]() {
        io.Send("left");
    });
    io.Hotkey("s", [&io]() {
        io.Send("down");
    });
    io.Hotkey("d", [&io]() {
        io.Send("right");
    });
    io.Hotkey("\\", [&io]() {
        io.Send("mediaplay");
    });
    io.Hotkey("rctrl", [&io]() {
        io.Send("mediaplay");
    });
    io.Hotkey("numpadadd", [&io]() {
        io.Send("volumeup");
    });
    io.Hotkey("numpadsub", [&io]() {
        io.Send("volumedown");
    });
    io.Hotkey("numpaddiv", [&io]() {
        io.Send("volumemute");
    });

    io.Hotkey("insert", [&io]() {
        H::Window win("A");
        win.Min();
    });
    io.Hotkey("&f9", [&io]() {
        io.Suspend();
    });
    io.Hotkey("!Esc", []() {
        exit(0);
    });
    io.HotkeyListen();
}

// Main function (should be placed in a separate file, e.g., main.cpp)
int main() {
    WindowManager w;
    //w.SetPriority(5);
    lo << "Hello";
    HvC(io);
    
    return 0;
}