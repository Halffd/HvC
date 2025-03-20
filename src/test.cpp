#include "window/Window.hpp"
#include "core/IO.hpp"
#include <iostream>

void testSend(H::IO& io) {
    std::cout << "Testing Send function...\n";
    io.Send("Hello");
    io.Send("{Shift down}A{Shift up}"); // Test sending 'A' with Shift
    io.Send("{Ctrl down}C{Ctrl up}"); // Test sending Ctrl+C
}

void testControlSend(H::IO& io) {
    std::cout << "Testing ControlSend function...\n";
    // io.ControlSend("Notepad", "Hello");
    // io.ControlSend("Calculator", "1+2=");
}

void testRegisterHotkey(H::IO& io) {
    std::cout << "Testing RegisterHotkey function...\n";
    
    io.Hotkey("f1", []() {
        std::cout << "F1 pressed\n";
    });
    
    io.Hotkey("ctrl+a", []() {
        std::cout << "Ctrl+A pressed\n";
    });
    
    io.Hotkey("alt+shift+x", []() {
        std::cout << "Alt+Shift+X pressed\n";
    });
    
    io.Hotkey("win+r", []() {
        std::cout << "Win+R pressed\n";
    });
}

void testHotkey(H::IO& io) {
    std::cout << "Testing Hotkey function...\n";
    
    // Test basic key
    io.Hotkey("f1", []() {
        std::cout << "F1 pressed\n";
    });
    
    // Test with modifiers
    io.Hotkey("ctrl+a", []() {
        std::cout << "Ctrl+A pressed\n";
    });
    
    io.Hotkey("alt+shift+x", []() {
        std::cout << "Alt+Shift+X pressed\n";
    });
    
    io.Hotkey("win+r", []() {
        std::cout << "Win+R pressed\n";
    });
    
    // Test with special characters
    io.Hotkey("ctrl+!", []() {
        std::cout << "Ctrl+! pressed\n";
    });
    
    io.Hotkey("alt+@", []() {
        std::cout << "Alt+@ pressed\n";
    });
    
    // Test with numpad keys
    io.Hotkey("numpad0", []() {
        std::cout << "Numpad 0 pressed\n";
    });
}

void testHotkeyListen(H::IO& io) {
    std::cout << "Testing HotkeyListen function...\n";
    io.HotkeyListen();
    // Sleep to allow hotkeys to be detected
    // std::this_thread::sleep_for(std::chrono::seconds(10));
}

void testHandleKeyAction(H::IO& io) {
    std::cout << "Testing HandleKeyAction function...\n";
    // io.HandleKeyAction("press", "a");
    // io.HandleKeyAction("release", "a");
}

void testSetTimer(H::IO& io) {
    std::cout << "Testing SetTimer function...\n";
    // io.SetTimer(1000, []() {
    //     std::cout << "Timer fired\n";
    // });
    // std::this_thread::sleep_for(std::chrono::seconds(2));
}

void testMsgBox(H::IO& io) {
    std::cout << "Testing MsgBox function...\n";
    // io.MsgBox("This is a test message");
}

void linux_test(H::WindowManager w){
    std::cout << "Linux test\n";
    
    // Test window operations
    H::Window win("A");
    std::cout << "Active window: " << win.id << "\n";
    
    // Test window properties
    std::cout << "Window title: " << win.Title() << "\n";
    
    // Test window actions
    // win.Min();
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // win.Max();
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // win.Activate();
}

void windowsTest(H::WindowManager w){
    std::cout << "Windows test\n";
    
    // Test window operations
    H::Window win("Notepad");
    std::cout << "Notepad window: " << win.id << "\n";
    
    // Test window properties
    std::cout << "Window title: " << win.Title() << "\n";
    
    // Test window actions
    // win.Min();
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // win.Max();
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // win.Activate();
}

void setupAHKHotkeys(H::IO& io) {
    io.Hotkey("!Up", []{ H::WindowManager::MoveWindow(1); });
    io.Hotkey("!Down", []{ H::WindowManager::MoveWindow(2); });
    io.Hotkey("!Left", []{ H::WindowManager::MoveWindow(3); });
    io.Hotkey("!Right", []{ H::WindowManager::MoveWindow(4); });
    
    io.Hotkey("^!Up", []{ H::WindowManager::ResizeWindow(1); });
    io.Hotkey("^!Down", []{ H::WindowManager::ResizeWindow(2); });
    io.Hotkey("^!Left", []{ H::WindowManager::ResizeWindow(3); });
    io.Hotkey("^!Right", []{ H::WindowManager::ResizeWindow(4); });
    
    io.Hotkey("#Left", []{ H::WindowManager::SnapWindow(1); });
    io.Hotkey("#Right", []{ H::WindowManager::SnapWindow(2); });
    
    io.Hotkey("^#Left", []{ H::WindowManager::ManageVirtualDesktops(1); });
    io.Hotkey("^#Right", []{ H::WindowManager::ManageVirtualDesktops(2); });
    
    io.Hotkey("^Space", []{ H::WindowManager::ToggleAlwaysOnTop(); });
}

void test(H::IO& io){
    std::cout << "Running tests...\n";
    
    // Test IO functions
    testSend(io);
    testControlSend(io);
    testRegisterHotkey(io);
    testHotkey(io);
    
    // Test window management
    H::WindowManager wm;
    
    #ifdef _WIN32
    windowsTest(wm);
    #else
    linux_test(wm);
    #endif
    
    // Test hotkey listening (this will block)
    testHotkeyListen(io);
}

int test_main(int argc, char* argv[]) {
    H::IO io;
    test(io);
    return 0;
}