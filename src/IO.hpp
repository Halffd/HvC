#ifndef IO_HPP
#define IO_HPP

#include <iostream>
#include <string>
#include <thread>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include "WindowManager.hpp"
// Structure to hold hotkey information
struct Hotkey {
    int modifiers;
    struct {
        int virtualKey;
        std::string name;
    } key;
    std::function<void()> action; // Action to perform on hotkey activation
    bool blockInput; // Whether to block input when the hotkey is activated
};


// IO class declaration
class IO {
public:
    wID ioWindow;
    static HHOOK keyboardHook;
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    IO();
    void Send(cstr keys);
    void ControlSend(cstr control, cstr keys);
    void AssignHotkey(Hotkey hotkey, int id = -1);
    void AddHotkey(cstr hotkeyStr, std::function<void()> action, bool blockInput = false, int id = -1);
    void HotkeyListen();
    void SetTimer(int milliseconds, const std::function<void()>& func);
    static void MsgBox(cstr message);
    void HandleKeyAction(cstr action, cstr keyName);
    static int GetState(const std::string& keyName, const std::string& mode = "T");
    static int StringToVirtualKey(str keyName);
    static void removeSpecialCharacters(str& keyName);
private:
    int ParseModifiers(str str);
    void ProcessKeyCombination(cstr keys);
    
    int hotkeyCount = 0; // Incrementing identifier for hotkeys
    std::unordered_map<int, Hotkey> hotkeys; // Map to store hotkeys by ID
};

#endif // IO_HPP