#include "logger.h"
#include "x11_includes.h"
#include "IO.hpp"

#if defined(WINDOWS)
HHOOK IO::keyboardHook = NULL;
#endif
std::unordered_map<int, HotKey> IO::hotkeys; // Map to store hotkeys by ID
IO::IO()
{
    H::DisplayManager::Initialize();
    Display* display = H::DisplayManager::GetDisplay();
    Window root = H::DisplayManager::GetRootWindow();

#if defined(__linux__)
    if (!display){
        display = XOpenDisplay(nullptr);
        root = XDefaultRootWindow(display);

        if (!display)
        {
            std::cerr << "Failed to open X11 display" << std::endl;
        }
    }
#endif
}

void IO::removeSpecialCharacters(str &keyName)
{
    // Define the characters to remove
    const std::string charsToRemove = "^+!#*&";

    // Remove characters
    keyName.erase(std::remove_if(keyName.begin(), keyName.end(),
                                 [&charsToRemove](char c)
                                 {
                                     return charsToRemove.find(c) != std::string::npos;
                                 }),
                  keyName.end());
}
void IO::HandleKeyEvent(XEvent& event) {
    XKeyEvent* keyEvent = reinterpret_cast<XKeyEvent*>(&event);
    KeySym keysym = XLookupKeysym(keyEvent, 0);

    for (const auto& [id, hotkey] : hotkeys) {
        if (hotkey.enabled && hotkey.key.virtualKey == keysym) {
            if (hotkey.action) hotkey.action();
        }
    }
}

void IO::HandleMouseEvent(XEvent& event) {
    XButtonEvent* buttonEvent = reinterpret_cast<XButtonEvent*>(&event);
    unsigned int button = buttonEvent->button;

    for (const auto& [id, hotkey] : hotkeys) {
        if (hotkey.enabled && IO::StringToButton(hotkey.key.name) == button) {
            if (hotkey.action) hotkey.action();
        }
    }
}

Key IO::StringToButton(str buttonName) {
    buttonName = ToLower(buttonName);
    
    if (buttonName == "button1") return Button1;
    if (buttonName == "button2") return Button2;
    if (buttonName == "button3") return Button3;
    if (buttonName == "wheelup" || buttonName == "scrollup") return Button4;
    if (buttonName == "wheeldown" || buttonName == "scrolldown") return Button5;
    return 0;
}
Key IO::handleKeyString(const std::string& keystr) {
    std::string type = keystr.substr(0, 2);
    std::string numberStr = keystr.substr(2);
    Key value;

    // Check for hexadecimal (0x or 0X prefix)
    if (numberStr.size() > 1 && numberStr[0] == '0' && 
        (numberStr[1] == 'x' || numberStr[1] == 'X')) {
        value = std::strtol(numberStr.c_str(), nullptr, 16);
    } else if (numberStr.size() > 1 && numberStr[0] == '0') {
        value = std::strtol(numberStr.c_str(), nullptr, 8);
    } else {
        value = std::strtol(numberStr.c_str(), nullptr, 10);
    }
    KeySym keysym = NoSymbol;

    if (type == "vk") {
        // For virtual keys, directly convert the value to a KeySym
        // Assuming value is offset from 'A' as in your original code
        keysym = XK_A + (value - 1);
        std::cout << "Virtual Key: " << value << " -> KeySym: " << keysym << std::endl;
    } 
    else if (type == "ks") {
        // For virtual keys, directly convert the value to a KeySym
        keysym = value;
        std::cout << "Virtual Key: " << value << " -> KeySym: " << keysym << std::endl;
    } 
    else if (type == "sc") {
        // For scan codes, use XkbKeycodeToKeysym
        // Note: scan code to keycode mapping might need system-specific adjustment
        KeyCode keycode = value;
        keysym = XkbKeycodeToKeysym(display, keycode, 0, 0);
        std::cout << "Scan Code: " << value << " -> KeySym: " << keysym << std::endl;
    } 
    else if (type == "kc") {
        // For keycodes, use XKeycodeToKeysym
        KeyCode keycode = value;
        keysym = XKeycodeToKeysym(display, keycode, 0);
        std::cout << "Keycode: " << value << " -> KeySym: " << keysym << std::endl;
    } 
    else {
        std::cout << "Unknown key type: " << keystr << std::endl;
    }

    return keysym;
}
// Helper function to convert string to virtual key code
Key IO::StringToVirtualKey(str keyName)
{
    removeSpecialCharacters(keyName);
#ifdef WINDOWS
    if (keyName.length() == 1)
    {
        return VkKeyScan(keyName[0]);
    }
    keyName = ToLower(keyName);
    // Map string names to virtual key codes
    if (keyName == "esc")
        return VK_ESCAPE;
    if (keyName == "home")
        return VK_HOME;
    if (keyName == "end")
        return VK_END;
    if (keyName == "pgup")
        return VK_PRIOR;
    if (keyName == "pgdn")
        return VK_NEXT;
    if (keyName == "insert")
        return VK_INSERT;
    if (keyName == "delete")
        return VK_DELETE;
    if (keyName == "numpad0")
        return VK_NUMPAD0;
    if (keyName == "numpad1")
        return VK_NUMPAD1;
    if (keyName == "numpad2")
        return VK_NUMPAD2;
    if (keyName == "numpad3")
        return VK_NUMPAD3;
    if (keyName == "numpad4")
        return VK_NUMPAD4;
    if (keyName == "numpad5")
        return VK_NUMPAD5;
    if (keyName == "numpad6")
        return VK_NUMPAD6;
    if (keyName == "numpad7")
        return VK_NUMPAD7;
    if (keyName == "numpad8")
        return VK_NUMPAD8;
    if (keyName == "numpad9")
        return VK_NUMPAD9;
    if (keyName == "up")
        return VK_UP;
    if (keyName == "down")
        return VK_DOWN;
    if (keyName == "left")
        return VK_LEFT;
    if (keyName == "right")
        return VK_RIGHT;
    if (keyName == "f1")
        return VK_F1;
    if (keyName == "f2")
        return VK_F2;
    if (keyName == "f3")
        return VK_F3;
    if (keyName == "f4")
        return VK_F4;
    if (keyName == "f5")
        return VK_F5;
    if (keyName == "f6")
        return VK_F6;
    if (keyName == "f7")
        return VK_F7;
    if (keyName == "f8")
        return VK_F8;
    if (keyName == "f9")
        return VK_F9;
    if (keyName == "f10")
        return VK_F10;
    if (keyName == "f11")
        return VK_F11;
    if (keyName == "f12")
        return VK_F21;
    if (keyName == "f13")
        return VK_F13;
    if (keyName == "f14")
        return VK_F14;
    if (keyName == "f15")
        return VK_F15;
    if (keyName == "f16")
        return VK_F16;
    if (keyName == "f17")
        return VK_F17;
    if (keyName == "f18")
        return VK_F18;
    if (keyName == "f19")
        return VK_F19;
    if (keyName == "f20")
        return VK_F20;
    if (keyName == "f21")
        return VK_F21;
    if (keyName == "f22")
        return VK_F22;
    if (keyName == "f23")
        return VK_F23;
    if (keyName == "f24")
        return VK_F24;
    if (keyName == "enter")
        return VK_RETURN;
    if (keyName == "space")
        return VK_SPACE;
    if (keyName == "lbutton")
        return VK_LBUTTON;
    if (keyName == "rbutton")
        return VK_RBUTTON;
    if (keyName == "apps")
        return VK_APPS;
    if (keyName == "win")
        return 0x5B;
    if (keyName == "lwin")
        return VK_LWIN;
    if (keyName == "rwin")
        return VK_RWIN;
    if (keyName == "ctrl")
        return VK_CONTROL;
    if (keyName == "lctrl")
        return VK_LCONTROL;
    if (keyName == "rctrl")
        return VK_RCONTROL;
    if (keyName == "shift")
        return VK_SHIFT;
    if (keyName == "lshift")
        return VK_LSHIFT;
    if (keyName == "rshift")
        return VK_RSHIFT;
    if (keyName == "alt")
        return VK_MENU;
    if (keyName == "lalt")
        return VK_LMENU;
    if (keyName == "ralt")
        return VK_RMENU;
    if (keyName == "backspace")
        return VK_BACK;
    if (keyName == "tab")
        return VK_TAB;
    if (keyName == "capslock")
        return VK_CAPITAL;
    if (keyName == "numlock")
        return VK_NUMLOCK;
    if (keyName == "scrolllock")
        return VK_SCROLL;
    if (keyName == "pausebreak")
        return VK_PAUSE;
    if (keyName == "printscreen")
        return VK_SNAPSHOT;
    if (keyName == "volumeup")
        return VK_VOLUME_UP;
    if (keyName == "volumedown")
        return VK_VOLUME_DOWN;
    return 0; // Default case for unrecognized keys
#else
    if (keyName.length() == 1)
    {
        return XStringToKeysym(keyName.c_str());
    }
    keyName = ToLower(keyName);

    if (keyName == "esc")
        return XK_Escape;
    if (keyName == "enter")
        return XK_Return;
    if (keyName == "space")
        return XK_space;
    if (keyName == "tab")
        return XK_Tab;
    if (keyName == "ctrl")
        return XK_Control_L;
    if (keyName == "lctrl")
        return XK_Control_L;
    if (keyName == "rctrl")
        return XK_Control_R;
    if (keyName == "shift")
        return XK_Shift_L;
    if (keyName == "lshift")
        return XK_Shift_L;
    if (keyName == "rshift")
        return XK_Shift_R;
    if (keyName == "alt")
        return XK_Alt_L;
    if (keyName == "lalt")
        return XK_Alt_L;
    if (keyName == "ralt")
        return XK_Alt_R;
    if (keyName == "win")
        return XK_Super_L;
    if (keyName == "lwin")
        return XK_Super_L;
    if (keyName == "rwin")
        return XK_Super_R;
    if (keyName == "backspace")
        return XK_BackSpace;
    if (keyName == "delete")
        return XK_Delete;
    if (keyName == "insert")
        return XK_Insert;
    if (keyName == "home")
        return XK_Home;
    if (keyName == "end")
        return XK_End;
    if (keyName == "pgup")
        return XK_Page_Up;
    if (keyName == "pgdn")
        return XK_Page_Down;
    if (keyName == "left")
        return XK_Left;
    if (keyName == "right")
        return XK_Right;
    if (keyName == "up")
        return XK_Up;
    if (keyName == "down")
        return XK_Down;
    if (keyName == "capslock")
        return XK_Caps_Lock;
    if (keyName == "numlock")
        return XK_Num_Lock;
    if (keyName == "scrolllock")
        return XK_Scroll_Lock;
    if (keyName == "pause")
        return XK_Pause;
    if (keyName == "f1")
        return XK_F1;
    if (keyName == "f2")
        return XK_F2;
    if (keyName == "f3")
        return XK_F3;
    if (keyName == "f4")
        return XK_F4;
    if (keyName == "f5")
        return XK_F5;
    if (keyName == "f6")
        return XK_F6;
    if (keyName == "f7")
        return XK_F7;
    if (keyName == "f8")
        return XK_F8;
    if (keyName == "f9")
        return XK_F9;
    if (keyName == "f10")
        return XK_F10;
    if (keyName == "f11")
        return XK_F11;
    if (keyName == "f12")
        return XK_F12;
    if (keyName == "numpad0")
        return XK_KP_0;
    if (keyName == "numpad1")
        return XK_KP_1;
    if (keyName == "numpad2")
        return XK_KP_2;
    if (keyName == "numpad3")
        return XK_KP_3;
    if (keyName == "numpad4")
        return XK_KP_4;
    if (keyName == "numpad5")
        return XK_KP_5;
    if (keyName == "numpad6")
        return XK_KP_6;
    if (keyName == "numpad7")
        return XK_KP_7;
    if (keyName == "numpad8")
        return XK_KP_8;
    if (keyName == "numpad9")
        return XK_KP_9;
    if (keyName == "numpadadd")
        return XK_KP_Add;
    if (keyName == "numpadsub")
        return XK_KP_Subtract;
    if (keyName == "numpadmul")
        return XK_KP_Multiply;
    if (keyName == "numpaddiv")
        return XK_KP_Divide;
    if (keyName == "numpaddec")
        return XK_KP_Decimal;
    if (keyName == "numpadenter")
        return XK_KP_Enter;
    if (keyName == "menu")
        return XK_Menu;
    if (keyName == "printscreen")
        return XK_Print;
    if (keyName == "volumeup")
        return XF86XK_AudioRaiseVolume; // Requires <X11/XF86keysym.h>
    if (keyName == "volumedown")
        return XF86XK_AudioLowerVolume; // Requires <X11/XF86keysym.h>
    if (keyName == "volumemute")
        return XF86XK_AudioMute; // Requires <X11/XF86keysym.h>
    if (keyName == "medianext")
        return XF86XK_AudioNext;
    if (keyName == "mediaprev")
        return XF86XK_AudioPrev;
    if (keyName == "mediaplay")
        return XF86XK_AudioPlay;

    return IO::StringToButton(keyName); // Default for unsupported keys}
#endif
}
void IO::Send(cstr keys)
{
    ProcessKeyCombination(keys);
}

void IO::SendX11Key(const std::string &keyName, bool press)
{
#if defined(__linux__)
    if (!display)
    {
        std::cerr << "X11 display not initialized!" << std::endl;
        return;
    }

    Key keysym = StringToVirtualKey(keyName);
    if (keysym == NoSymbol)
    {
        std::cerr << "Invalid key: " << keyName << std::endl;
        return;
    }

    KeyCode keycode = XKeysymToKeycode(display, keysym);
    if (keycode == 0)
    {
        std::cerr << "Cannot find keycode for " << keyName << std::endl;
        return;
    }

    XTestFakeKeyEvent(display, keycode, press, CurrentTime);
    XFlush(display);
#endif
}
void IO::ProcessKeyCombination(cstr keys)
{
    int modifiers = 0;

#if defined(WINDOWS)
    // Windows-specific implementation
    for (size_t i = 0; i < keys.length(); ++i)
    {
        if (keys[i] == '{')
        {
            size_t end = keys.find('}', i);
            if (end != std::string::npos)
            {
                std::string sequence = keys.substr(i + 1, end - i - 1);
                if (sequence == "Alt down")
                {
                    modifiers |= MOD_ALT;
                    keybd_event(VK_MENU, 0, 0, 0); // Press down Alt
                }
                else if (sequence == "Alt up")
                {
                    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0); // Release Alt
                }
                else if (sequence == "Ctrl down")
                {
                    modifiers |= MOD_CONTROL;
                    keybd_event(VK_CONTROL, 0, 0, 0); // Press down Ctrl
                }
                else if (sequence == "Ctrl up")
                {
                    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0); // Release Ctrl
                }
                else if (sequence == "Shift down")
                {
                    modifiers |= MOD_SHIFT;
                    keybd_event(VK_SHIFT, 0, 0, 0); // Press down Shift
                }
                else if (sequence == "Shift up")
                {
                    keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0); // Release Shift
                }
                else
                {
                    int virtualKey = StringToVirtualKey(sequence);
                    if (virtualKey)
                    {
                        keybd_event(virtualKey, 0, 0, 0);               // Press down
                        keybd_event(virtualKey, 0, KEYEVENTF_KEYUP, 0); // Release
                    }
                }
                i = end; // Move past the closing brace
                continue;
            }
        }

        int virtualKey = StringToVirtualKey(std::string(1, keys[i]));
        if (virtualKey)
        {
            keybd_event(virtualKey, 0, 0, 0);               // Press down
            keybd_event(virtualKey, 0, KEYEVENTF_KEYUP, 0); // Release
        }
    }

    // Release any held modifiers
    if (modifiers & MOD_ALT)
        keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    if (modifiers & MOD_CONTROL)
        keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    if (modifiers & MOD_SHIFT)
        keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
#else
    // Linux (X11)-specific implementation
    for (size_t i = 0; i < keys.length(); ++i)
    {
        if (keys[i] == '{')
        {
            size_t end = keys.find('}', i);
            if (end != std::string::npos)
            {
                std::string sequence = keys.substr(i + 1, end - i - 1);
                if (sequence == "Alt down")
                {
                    modifiers |= Mod1Mask;
                    SendX11Key("LAlt", true);
                }
                else if (sequence == "Alt up")
                {
                    modifiers &= ~Mod1Mask;
                    SendX11Key("LAlt", false);
                }
                else if (sequence == "Ctrl down")
                {
                    modifiers |= ControlMask;
                    SendX11Key("LCtrl", true);
                }
                else if (sequence == "Ctrl up")
                {
                    modifiers &= ~ControlMask;
                    SendX11Key("LCtrl", false);
                }
                else if (sequence == "Shift down")
                {
                    modifiers |= ShiftMask;
                    SendX11Key("LShift", true);
                }
                else if (sequence == "Shift up")
                {
                    modifiers &= ~ShiftMask;
                    SendX11Key("LShift", false);
                }
                else
                {
                    SendX11Key(sequence, true);
                    SendX11Key(sequence, false);
                }
                i = end; // Move past the closing brace
                continue;
            }
        }

        std::string keyName(1, keys[i]);
        SendX11Key(keyName, true);
        SendX11Key(keyName, false);
    }

    // Release any held modifiers
    if (modifiers & Mod1Mask)
        SendX11Key("LAlt", false);
    if (modifiers & ControlMask)
        SendX11Key("LCtrl", false);
    if (modifiers & ShiftMask)
        SendX11Key("LShift", false);
#endif
}

bool IO::Suspend(int status)
{
    if (status == -1)
    {
        hotkeyEnabled = !hotkeyEnabled; // Toggle state
    }
    else
    {
        hotkeyEnabled = (status != 0); // Set state based on status
    }

    if (hotkeyEnabled)
    {
        // Hotkeys are enabled
        std::cout << "Hotkeys enabled." << std::endl;
        // Optionally re-register hotkeys if previously unregistered
        for (auto &[id, hotkey] : hotkeys)
        {
            if (!hotkey.suspend)
            {
                #ifdef WINDOWS
                if (hotkey.blockInput)
                {
                    AssignHotkey(hotkey, id);
                }
                else
                {
                    hotkey.enabled = true;
                }
                #else
                    AssignHotkey(hotkey, id);
                #endif
            }
            else
            {
                hotkey.enabled = true;
            }
        }
    }
    else
    {
        // Hotkeys are disabled
        std::cout << "Hotkeys disabled." << std::endl;
        // Unregister hotkeys
        for (auto &[id, hotkey] : hotkeys)
        {
            if (!hotkey.suspend)
            {
                if (hotkey.blockInput)
                {
#ifdef WINDOWS
                    UnregisterHotKey(0, id);
#else
                    XUngrabKey(display, hotkey.key.virtualKey, hotkey.modifiers, root);
#endif
                }
                hotkey.enabled = false;
            }
        }
    }
    return hotkeyEnabled;
}
void IO::Hotkey(cstr hotkeyStr, std::function<void()> action, int id)
{
    str keyStr = hotkeyStr;
    str keys = hotkeyStr;
    int modifiers = ParseModifiers(keyStr);
    bool block = true;
    bool isSuspend = false;
    if (hotkeyStr.find("*") != str::npos)
    {
        block = false;
        keys.erase(keys.find("*"), 1);
    }
    if (keys.find("&") != str::npos)
    {
        isSuspend = true;
        keys.erase(keys.find("&"), 1);
    }
    removeSpecialCharacters(keyStr);
    Key virtualKey = StringToVirtualKey(keyStr);
    HotKey hotkey = {
        modifiers,
        {
            virtualKey,
            keys
        },
        action,
        block,
        isSuspend,
        true};
    if (virtualKey)
    {
#ifdef WINDOWS
        if (action)
        {
            // Assign hotkey if action is provided
            AssignHotkey(hotkey, id);
        }
        else
        {
            // Action is nullptr, find and unregister by key name
            for (auto it = hotkeys.begin(); it != hotkeys.end(); ++it)
            {
                if (it->second.key.name == hotkeyStr)
                {
                    if (it->second.blockInput)
                    {
                        UnregisterHotKey(0, it->first); // Unregister the hotkey
                    }
                    it->second.enabled = false;
                    // hotkeys.erase(it); // Remove from the map
                    std::cout << "Hotkey unregistered: " << hotkeyStr << std::endl;
                    return; // Exit after unregistering
                }
            }
            std::cerr << "Hotkey not found for unregistration: " << hotkeyStr << std::endl;
        }
#else
        if (id == -1)
            id = ++hotkeyCount;
        AssignHotkey(hotkey, id);
        hotkeys[id] = hotkey;
#endif
    }
    else
    {
        std::cerr << "Invalid key name: " << keyStr << std::endl;
    }
}
void IO::ControlSend(cstr control, cstr keys)
{
    wID hwnd = WindowManager::Find(control);
    if (hwnd)
    {
        for (char key : keys)
        {
            int virtualKey = StringToVirtualKey(str(1, key));
            if (virtualKey)
            {
#ifdef WINDOWS
                SendMessage(hwnd, WM_KEYDOWN, virtualKey, 0);
                SendMessage(hwnd, WM_KEYUP, virtualKey, 0);
#endif
            }
        }
    }
    else
    {
        std::cerr << "Window not found!";
    }
}

void IO::SetTimer(int milliseconds, const std::function<void()> &func)
{
    std::thread([=]()
                {
            std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
            func(); })
        .detach();
}

void IO::MsgBox(cstr message)
{
#if defined(WINDOWS)
    MessageBoxA(NULL, message.c_str(), "Message", MB_OK);
#else
    std::cout << "MsgBox: " << message << std::endl;
#endif
}
void IO::AssignHotkey(HotKey hotkey, int id)
{
    if (id == -1)
    {
        id = ++hotkeyCount; // Auto increment if id is not provided
    }
    if (hotkey.blockInput)
    {
#ifdef WINDOWS
        if (RegisterHotKey(0, id, hotkey.modifiers, hotkey.key.virtualKey))
        {
            lo << "Hotkey registered: " << hotkey.key.name;
        }
        else
        {
            std::cerr << "Failed to register hotkey: " << GetLastError();
            return;
        }
#else
    if (!display) {
        std::cerr << "X11 display not initialized for hotkey assignment!" << std::endl;
        return;
    }

    if (id == -1) id = ++hotkeyCount;
    hotkeys[id] = hotkey;

    KeyCode keycode = XKeysymToKeycode(display, hotkey.key.virtualKey);
    if (keycode == 0) {
        std::cerr << "Invalid key for X11 hotkey assignment!" << std::endl;
        return;
    }

    // Grab the key with the appropriate owner_events based on blockInput
    Bool owner_events = hotkey.blockInput ? False : True;

    if (XGrabKey(display, keycode, hotkey.modifiers, root, owner_events,
                    GrabModeAsync, GrabModeAsync) != Success) {
        std::cerr << "Failed to grab key: " << hotkey.key.name << std::endl;
    }
    // If mouse buttons are included, grab them too
    str kn = ToLower(hotkey.key.name);
    if (kn == "button1" || kn == "button2" || kn == "button3" ||
        kn == "scrollup" || kn == "scrolldown" || kn == "wheeldown" || kn == "wheelup") {
        Key button = IO::StringToButton(kn);
        if (button != 0) {
            XGrabButton(display, button, hotkey.modifiers, DefaultRootWindow(display), True,
                        ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
        }
    }
#endif
    }
    hotkey.enabled = true;
    if (hotkeys.find(id) == hotkeys.end())
    {
        hotkeys[id] = hotkey;
    }
}

#ifdef WINDOWS
LRESULT CALLBACK IO::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN)
    {
        KBDLLHOOKSTRUCT *pKeyboard = (KBDLLHOOKSTRUCT *)lParam;

        // Detect the state of modifier keys
        bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool altPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;
        bool winPressed = (GetKeyState(VK_LWIN) & 0x8000) != 0 || (GetKeyState(VK_RWIN) & 0x8000) != 0;
        for (const auto &[id, hotkey] : hotkeys)
        {
            if (!hotkey.blockInput && hotkey.enabled)
            {
                // Check if the virtual key matches and modifiers are valid
                if (pKeyboard->vkCode == static_cast<DWORD>(hotkey.key.virtualKey))
                {
                    // Check if the required modifiers are pressed
                    bool modifiersMatch =
                        ((hotkey.modifiers & MOD_SHIFT) ? shiftPressed : true) &&
                        ((hotkey.modifiers & MOD_CONTROL) ? ctrlPressed : true) &&
                        ((hotkey.modifiers & MOD_ALT) ? altPressed : true) &&
                        ((hotkey.modifiers & MOD_WIN) ? winPressed : true); // Check Windows key

                    if (modifiersMatch)
                    {
                        if (hotkey.action)
                        {
                            std::cout << "Action from non-blocking callback for " << hotkey.key.name << "\n";
                            hotkey.action(); // Call the associated action
                        }
                        break; // Exit after the first match
                    }
                }
            }
        }
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}
#endif
int IO::GetMouse() {
    if (!display) {
        display = XOpenDisplay(nullptr);
        if (!display) {
            std::cerr << "Unable to open X display!" << std::endl;
            return EXIT_FAILURE;
        }
    }

    Window window = XCreateSimpleWindow(display, root, 10, 10, 200, 200, 1, 0, 0);
    XSelectInput(display, window, ButtonPressMask | PointerMotionMask);
    XMapWindow(display, window);

    if (XGrabPointer(display, window, True, ButtonPressMask | PointerMotionMask,
                     GrabModeAsync, GrabModeAsync, None, None, CurrentTime) != GrabSuccess) {
        std::cerr << "Unable to grab pointer!" << std::endl;
        return EXIT_FAILURE;
    }

    XEvent event;
    while (true) {
        XNextEvent(display, &event);
        if (event.type == ButtonPress) {
            std::cout << "Mouse button pressed: " << event.xbutton.button << std::endl;
        } else if (event.type == MotionNotify) {
            std::cout << "Mouse moved: (" << event.xmotion.x << ", " << event.xmotion.y << ")" << std::endl;
        }
    }

    XUngrabPointer(display, CurrentTime);
    return 0;
}
int IO::GetKeyboard() {
    if (!display) {
        display = XOpenDisplay(nullptr);
        if (!display) {
            std::cerr << "Unable to open X display!" << std::endl;
            return EXIT_FAILURE;
        }
    }

    Window window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, 0, 0);
    if (XGrabKeyboard(display, window, True, GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess) {
        std::cerr << "Unable to grab keyboard!" << std::endl;
        XDestroyWindow(display, window);
        return EXIT_FAILURE;
    }

    XEvent event;
    while (true) {
        XNextEvent(display, &event);
        if (event.type == KeyPress) {
            std::cout << "Key pressed: " << event.xkey.keycode << std::endl;
        }
    }

    XUngrabKeyboard(display, CurrentTime);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}
void IO::HotkeyListen() {
#if defined(WINDOWS)
    MSG msg;
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (!keyboardHook) {
        std::cerr << "Failed to install hook: " << GetLastError() << std::endl;
        return;
    }
    while (GetMessage(&msg, 0, 0, 0)) {
        if (msg.message == WM_HOTKEY && hotkeys.count(msg.wParam)) {
            HotKey hotkey = hotkeys[msg.wParam];
            if (hotkey.action) {
                std::cout << "Action triggered for: " << hotkey.key.name << "\n";
                hotkey.action();
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(keyboardHook);
#elif defined(__linux__)
    
    if (!display) {
        std::cerr << "X11 display not initialized for hotkey listening!" << std::endl;
        return;
    }

    XEvent event;
    Display* dis = display;
    
    // Event loop to listen for hotkeys
    while (true) {
        XNextEvent(dis, &event);

        if (event.type == KeyPress) {
            XKeyEvent* keyEvent = reinterpret_cast<XKeyEvent*>(&event);
            KeySym keysym = XLookupKeysym(keyEvent, 0);

            // Iterate through all hotkeys and trigger matching actions
            for (const auto& [id, hotkey] : hotkeys) {
                if (hotkey.enabled && hotkey.key.virtualKey == keysym) {
                    if (hotkey.action) {
                        hotkey.action(); // Trigger hotkey action
                    }

                    // Break the loop if input blocking is enabled
                    if (hotkey.blockInput) {
                        lo << "Blocked input for hotkey: " << hotkey.key.name;
                        break;
                    }
                }
            }
        }
    }

    // Ungrab all keys when exiting
    for (const auto& [id, hotkey] : hotkeys) {
        if (!hotkey.enabled) continue;

        KeyCode keycode = XKeysymToKeycode(dis, hotkey.key.virtualKey);
        if (keycode != 0) {
            XUngrabKey(dis, keycode, hotkey.modifiers, root);
        }
    }

    XCloseDisplay(dis);
#endif
}

int IO::ParseModifiers(str str)
{
    int modifiers = 0;
#ifdef WINDOWS
    if (str.find("+") != str::npos)
    {
        modifiers |= MOD_SHIFT;
        str.erase(str.find("+"), 1);
    }
    if (str.find("^") != str::npos)
    {
        modifiers |= MOD_CONTROL;
        str.erase(str.find("^"), 1);
    }
    if (str.find("!") != str::npos)
    {
        modifiers |= MOD_ALT;
        str.erase(str.find("!"), 1);
    }
    if (str.find("#") != str::npos)
    {
        modifiers |= MOD_WIN;
        str.erase(str.find("#"), 1);
    }
#else
    if (str.find("+") != std::string::npos) {
        modifiers |= ShiftMask;
        str.erase(str.find("+"), 1);
    }
    if (str.find("^") != std::string::npos) {
        modifiers |= ControlMask;
        str.erase(str.find("^"), 1);
    }
    if (str.find("!") != std::string::npos) {
        modifiers |= Mod1Mask;
        str.erase(str.find("!"), 1);
    }
    if (str.find("#") != std::string::npos) {
        modifiers |= Mod4Mask; // For Meta/Windows
        str.erase(str.find("#"), 1);
    }
#endif
    return modifiers;
}
void IO::HandleKeyAction(cstr action, cstr keyName)
{
#if defined(WINDOWS)
    int virtualKey = StringToVirtualKey(keyName);
    if (virtualKey)
    {
        if (action == "down")
        {
            keybd_event(virtualKey, 0, KEYEVENTF_EXTENDEDKEY, 0);
        }
        else if (action == "up")
        {
            keybd_event(virtualKey, 0, KEYEVENTF_KEYUP, 0);
        }
    }
#elif defined(WAYLAND)
    // Implementation depends on Wayland compositor capabilities.
    // Typically requires using libinput or the compositor's client API.
    std::cerr << "Wayland key simulation not yet implemented." << std::endl;
#else
    bool press = action == "down";
    Display *display = XOpenDisplay(nullptr);
    if (!display)
    {
        std::cerr << "Cannot open X11 display" << std::endl;
        return;
    }

    KeySym keysym = XStringToKeysym(keyName.c_str());
    if (keysym == NoSymbol)
    {
        std::cerr << "Invalid key: " << keyName << std::endl;
        return;
    }

    KeyCode keycode = XKeysymToKeycode(display, keysym);
    if (keycode == 0)
    {
        std::cerr << "Cannot find keycode for " << keyName << std::endl;
        return;
    }

    XTestFakeKeyEvent(display, keycode, press, CurrentTime);
    XFlush(display);
#endif
}
int IO::GetState(cstr keyName, cstr mode) {
#if defined(WINDOWS)
    int virtualKey = StringToVirtualKey(keyName);
    if (mode.empty() || mode == "P") {
        return (GetAsyncKeyState(virtualKey) & 0x8000) ? 1 : 0;
    } else if (mode == "T") {
        return (GetKeyState(virtualKey) & 0x1) ? 1 : 0;
    }
#elif defined(__linux__)
    if (!display) {
        display = XOpenDisplay(nullptr);
        if (!display) {
            std::cerr << "Unable to open X11 display!" << std::endl;
            return 0;
        }
    }

    char keys[32];
    XQueryKeymap(display, keys);

    KeySym keysym = XStringToKeysym(keyName.c_str());
    if (keysym == NoSymbol) {
        std::cerr << "Invalid key: " << keyName << std::endl;
        return 0;
    }

    KeyCode keycode = XKeysymToKeycode(display, keysym);
    if (keycode == 0) {
        std::cerr << "Invalid keycode for key: " << keyName << std::endl;
        return 0;
    }

    if (mode.empty() || mode == "P") {
        return (keys[keycode / 8] & (1 << (keycode % 8))) ? 1 : 0;
    } else if (mode == "T") {
        if (keyName == "CapsLock" || keyName == "NumLock") {
            XKeyboardState state;
            XGetKeyboardControl(display, &state);

            if (keyName == "CapsLock") {
                return (state.led_mask & 0x1) ? 1 : 0;
            } else if (keyName == "NumLock") {
                return (state.led_mask & 0x2) ? 1 : 0;
            }
        }
        return 0; // No toggle state for other keys
    }
#endif
    return 0;
}

void IO::PressKey(const std::string& keyName, bool press) {
#ifdef __linux__
    auto* display = H::DisplayManager::GetDisplay();
    if(!display) {
        std::cerr << "No X11 display available for key press\n";
        return;
    }
    
    KeyCode keycode = XKeysymToKeycode(display, StringToVirtualKey(keyName));
    if(keycode == 0) {
        std::cerr << "Invalid keycode for: " << keyName << "\n";
        return;
    }
    
    XTestFakeKeyEvent(display, keycode, press, CurrentTime);
    XFlush(display);
#endif
}
