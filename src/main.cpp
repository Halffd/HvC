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
    io.ControlSend("Code", "Hello from ControlSend");
}

void testRegisterHotkey(IO& io) {
    lo << "Testing RegisterHotkey function...\n";
    io.AssignHotkey({
        MOD_CONTROL, {VK_SPACE,
         "Ctrl+Space"}, []() {
        lo << "Ctrl+Space activated!\n";
    }, true});
}

void testAddHotkey(IO& io) {
    lo << "Testing AddHotkey function...\n";
    io.AddHotkey("a", []() {
        lo << "a activated!\n";
    }, true);
    io.AddHotkey("f2", []() {
        lo << "f2 activated!\n";
    }, true);
    io.AddHotkey("^+x", []() {
        lo << "Ctrl+Shift+x activated!\n";
    }, true);
    io.AddHotkey("+c", []() {
        lo << "Shift+c activated!\n";
    }, false);
    io.AddHotkey("#-", []() {
        IO::MsgBox("Win+- activated!\n");
    }, false);
}

void testHotkeyListen(IO& io) {
   // lo << "Testing HotkeyListen function...\n";
    //std::thread([&io]() {
        io.HotkeyListen(); // Run in a separate thread
    //}).detach();
}
void testHandleKeyAction(IO& io) {
    lo << "Testing HandleKeyAction function...\n";
    io.HandleKeyAction("down", "A");
    io.HandleKeyAction("up", "A");
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


// Main function (should be placed in a separate file, e.g., main.cpp)
int main() {
    WindowManager w;
    w.SetPriority(5);
    lo << "Hello";
    //int r = w.Run("C:\\Windows\\explorer.exe", 1, "Max", "", 1);
    //lo << r;
    //int r2 = w.Run("C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe", 0, "Min", "", 1);
    //lo << r2;
    // int r3 = w.Run("C:\\Program Files\\mpv\\mpv.exe", 0, "", "https://youtu.be/Hvy4xxdP8Dg", 3);
    // lo << r3;
    //lo << w.Run("C:\\Program Files\\mpv\\mpv.exe", 0, "", "https://youtu.be/kHUaEKZQcQo", 0);
    //lo << w.Run("https://www.youtube.com", ProcessMethod::Shell, "", "");
    //lo << w.Run("S:\\Emu\\ryujinx\\Ryujinx.exe", 0, "", "", 0);
    //lo << w.Run("S:\\SteamLibrary\\steamapps\\common\\GarrysMod\\gmod.exe", ProcessMethod::CreateNewWindow, "", "-gl", 5);
    //lo << w.Terminal("@echo off & powershell -command \"Add-Type -AssemblyName PresentationFramework; [System.Windows.MessageBox]::Show('Hello, this is a message box!', 'Message Box Title')\"", false, "Hide", false);
    //lo << w.Terminal("streamlink twitch.tv/robcdee best", false, "Hide", false);
    //lo << w.Terminal("ipconfig", true, "Call", true);
    //lo << w.Terminal("py -m IPython", true, "Max", true, "Powershell");
    //lo << w.Terminal("Write-Host (Get-Date -Format \"dd MMMM yyyy\")", true, "", true, "Powershell");
    Window wm("exe mpv.exe"); // Initialize with an empty identifier

    // Example usage of adding groups
    wm.AddGroup("grp", "exe blender.exe");
    wm.AddGroup("grp", "class Notepad++");
    wm.AddGroup("grp", "exe code.exe");

    // Example usage of FindWindow
    wID hwnd = wm.Find("exe mpv.exe"); // Change this to test other identifiers

    if (hwnd) {
        lo << "Window Title: " << wm.Title(hwnd);
        //wm.Min(hwnd);
        // wm.Max(hwnd);
        // wm.AlwaysOnTop(hwnd, true);
        // wm.Transparency(hwnd, 128); // 0-255, where 255 is fully opaque
        // wm.Close(hwnd);
    }

    //wm.All(); // List all open windows
    IO io;

    std::string keyName = "Space";
    std::string mode = "P";

    // Get key state 
    int state = io.GetState(keyName, mode);
    std::cout << "Key state: " << (state ? "Pressed" : "Released") << std::endl;

    // Run tests
    // testSend(io);
    testControlSend(io);
    testRegisterHotkey(io);
    testAddHotkey(io);
    // testSetTimer(io);
    // testMsgBox(io);
    testHotkeyListen(io);

    bool loop = true;
    while(loop){
        if(io.GetState("Esc")){
            loop = false;
        }
    }
    // Wait for user input to exit
    //lo << "Press Enter to exit...";
    //std::cin.get();

    return 0;
}