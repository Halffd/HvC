#include "Window.hpp"
#include "IO.hpp"

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
    #ifdef WINDOWS
    io.AssignHotkey({
        MOD_CONTROL, {VK_SPACE,
         "Ctrl+Space"}, []() {
        lo << "Ctrl+Space activated!\n";
    }, true, true, true});
    #else
    io.AssignHotkey({
        ControlMask, {0x0020,
         "Ctrl+Space"}, []() {
        lo << "Ctrl+Space activated!\n";
    }, true, true, true});
    #endif
}

void testHotkey(IO& io) {
    lo << "Testing Hotkey function...\n";
    io.Hotkey("a", []() {
        lo << "a activated!\n";
    });
    io.Hotkey("f2", [&io]() {
        lo << "f2 activated!\n";
        io.Hotkey("-");
        lo << "- deactivated !\n";
    });
    io.Hotkey("^+x", [&io]() {
        lo << "Ctrl+Shift+x activated!\n";
        io.Hotkey("a");
        lo << "a deactivated !\n";
    });
    io.Hotkey("&+c", [&io]() {
        lo << "Shift+c activated!\n";
        io.Suspend();
    });
    io.Hotkey("*#-", []() {
        IO::MsgBox("Win+- activated!\n");
    });
    io.Hotkey("*-", []() {
        IO::MsgBox("- activated!\n");
    });
    
    io.Hotkey("!Esc", []() {
        exit(0);
    });
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
void linux_test(WindowManager w){
    int r = w.Run("/usr/games/lutris", 1, "Max", "", 0);
    lo << r;
    int r2 = w.Run("/usr/bin/kate", 0, "Min", "", 0);
    lo << r2;
    int r3 = w.Run("/usr/bin/mpv https://youtu.be/Hvy4xxdP8Dg", 0, "", "", 3);
    lo << r3;
    lo << w.Terminal("streamlink twitch.tv/shylily best", false, "Hide", false);
    lo << w.Terminal("ping 8.8.8.8", true, "Call", true);
    lo << w.Terminal("py -m IPython", true, "Max", false);
    lo << w.Run("/home/all/scripts/lux.sh 0.4", ProcessMethod::Shell, "", "");
    lo << w.Run("/home/all/scripts/lux.sh -0.1", ProcessMethod::Shell, "", "");
}
void windowsTest(WindowManager w){
    int r = w.Run("C:\\Windows\\explorer.exe", 1, "Max", "", 1);
    lo << r;
    int r2 = w.Run("C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe", 0, "Min", "", 1);
    lo << r2;
    int r3 = w.Run("C:\\Program Files\\mpv\\mpv.exe", 0, "", "https://youtu.be/Hvy4xxdP8Dg", 3);
    lo << r3;
    lo << w.Run("C:\\Program Files\\mpv\\mpv.exe", 0, "", "https://youtu.be/kHUaEKZQcQo", 0);
    lo << w.Run("mpv https://www.youtube.com", ProcessMethod::Shell, "", "");
    lo << w.Run("S:\\Emu\\ryujinx\\Ryujinx.exe", 0, "", "", 0);
    lo << w.Run("S:\\SteamLibrary\\steamapps\\common\\GarrysMod\\gmod.exe", ProcessMethod::CreateNewWindow, "", "-gl", 5);
    lo << w.Terminal("@echo off & powershell -command \"Add-Type -AssemblyName PresentationFramework; [System.Windows.MessageBox]::Show('Hello, this is a message box!', 'Message Box Title')\"", false, "Hide", false);
    lo << w.Terminal("Write-Host (Get-Date -Format \"dd MMMM yyyy\")", true, "", true, "Powershell");
}
void test(IO io){
    //linux_test(w);

    //bool loop = true;
    //while(loop){
    //    if(io.GetState("Esc")){
    //        loop = false;
    //    }
    //}
    // Wait for user input to exit
    //lo << "Press Enter to exit...";
    //std::cin.get();

    H::Window wm("exe mpv"); // Initialize with an empty identifier

    // Example usage of adding groups
    wm.AddGroup("grp", "exe blender");
    wm.AddGroup("grp", "class Notepad++");
    wm.AddGroup("grp", "exe code");

    // Example usage of FindWindow
    wID winId = wm.Find("exe mpv"); // Change this to test other identifiers
    std::cout << winId;
    if (winId) {
        lo << "Window Title: " << wm.Title(winId);
        wm.Min(winId);
        wm.Max(winId);
        wm.AlwaysOnTop(winId, true);
        wm.Transparency(winId, 128); // 0-255, where 255 is fully opaque
        wm.Pos(winId);
        wm.Close(winId);
    }
    wm.All(); // List all open windows

    std::string keyName = "Space";
    std::string mode = "P";

    // Get key state 
    int state = io.GetState(keyName, mode);
    std::cout << "Key state: " << (state ? "Pressed" : "Released") << std::endl;

    // Run tests
    //testSend(io);
    testControlSend(io);
    testRegisterHotkey(io);
    testHotkey(io);
    //testSetTimer(io);
    //testMsgBox(io);
    testHotkeyListen(io);
}