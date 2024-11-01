#include "Window.hpp"
#include "IO.hpp"
#include <iostream>
#include <thread>

void testSend(IO& io) {
    lo << "Testing Send function...\n";
    io.Send("Hello");
    io.Send("{Shift down}A{Shift up}"); // Test sending 'A' with Shift
    io.Send("{Ctrl down}C{Ctrl up}"); // Test sending Ctrl+C
}

void testControlSend(IO& io) {
    lo << "Testing ControlSend function...\n";
    // Assuming you have a valid window title or class
    io.ControlSend("Notepad", "Hello from ControlSend");
}

void testRegisterHotkey(IO& io) {
    lo << "Testing RegisterHotkey function...\n";
    io.RegisterHotkey({MOD_CONTROL, {VK_SPACE, "Ctrl+Space"}, []() {
        lo << "Ctrl+Space activated!\n";
    }});
}

void testAddHotkey(IO& io) {
    lo << "Testing AddHotkey function...\n";
    io.AddHotkey("!x", []() {
        lo << "Alt+x activated!\n";
    }, false);
}

void testHotkeyListen(IO& io) {
    lo << "Testing HotkeyListen function...\n";
    std::thread([&io]() {
        io.HotkeyListen(); // Run in a separate thread
    }).detach();
}

void testSetTimer(IO& io) {
    lo << "Testing SetTimer function...\n";
    io.SetTimer(2000, []() {
        lo << "Timer executed after 2 seconds!\n";
    });
}

void testMsgBox(IO& io) {
    lo << "Testing MsgBox function...\n";
    io.MsgBox("This is a test message box!");
}

void testHandleKeyAction(IO& io) {
    lo << "Testing HandleKeyAction function...\n";
    io.HandleKeyAction("down", "A");
    io.HandleKeyAction("up", "A");
}

// Main function (should be placed in a separate file, e.g., main.cpp)
int main() {
    lo << "Hello";
    lo << 1;
    lo << "a" << 2 << 4 << "b";
    Window wm("exe mpv.exe"); // Initialize with an empty identifier

    // Example usage of adding groups
    wm.AddGroup("grp", "class Notepad++");
    wm.AddGroup("grp", "exe code.exe");

    // Example usage of FindWindow
    HWND hwnd = wm.Find("group grp"); // Change this to test other identifiers

    if (hwnd) {
        lo << "Window Title: " << wm.Title(hwnd);
        wm.Min(hwnd);
        wm.Max(hwnd);
        wm.AlwaysOnTop(hwnd, true);
        wm.Transparency(hwnd, 128); // 0-255, where 255 is fully opaque
        wm.Close(hwnd);
    }

    wm.All(); // List all open windows
    IO io;

    std::string keyName = "Space";
    std::string mode = "P";

    // Get key state 
    int state = io.GetState(keyName, mode);
    std::cout << "Key state: " << (state ? "Pressed" : "Released") << std::endl;


    // Run tests
    testSend(io);
    testControlSend(io);
    testRegisterHotkey(io);
    testAddHotkey(io);
    testHotkeyListen(io);
    testSetTimer(io);
    testMsgBox(io);
    testHandleKeyAction(io);

    // Wait for user input to exit
    lo << "Press Enter to exit...";
    std::cin.get();

    return 0;
}