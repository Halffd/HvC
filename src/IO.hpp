#ifndef IO_HPP
#define IO_HPP

#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <thread>
#include <chrono>
#include "WindowManager.hpp"

struct HotKey {
    int modifiers; // Modifier mask (Ctrl, Alt, Shift, etc.)
    struct {
        Key virtualKey;
        std::string name;
    } key;
    std::function<void()> action; // Action to perform on activation
    bool blockInput;
    bool suspend;
    bool enabled;
};

class IO {
public:
    static std::unordered_map<int, HotKey> hotkeys;
    IO();
    void Send(const std::string& keys);
    void ControlSend(const std::string& control, const std::string& keys);
    void AssignHotkey(HotKey hotkey, int id = -1);
    void Hotkey(const std::string& hotkeyStr, std::function<void()> action = nullptr, int id = -1);
    bool Suspend(int status = -1);
    void HotkeyListen();
    void SetTimer(int milliseconds, const std::function<void()>& func);
    static void MsgBox(const std::string& message);
    void HandleKeyAction(const std::string& action, const std::string& keyName);
    int GetState(const std::string& keyName, const std::string& mode = "T");
    static Key StringToVirtualKey(str keyName);
    static void removeSpecialCharacters(std::string& keyName);

private:
    int GetKeyboard();
    int GetMouse();
    void HandleMouseEvent(XEvent& event);
    void HandleKeyEvent(XEvent& event);
    unsigned int StringToButton(const str& buttonName);

    void ProcessKeyCombination(const std::string& keys);
    int ParseModifiers(std::string str);
    void SendX11Key(const std::string& keyName, bool press);

    int hotkeyCount = 0;
    bool hotkeyEnabled = true;

#ifdef WINDOWS
    LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
#endif
};

#endif // IO_HPP
