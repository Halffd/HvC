#include "logger.h"
#include "x11_includes.h"
#include "IO.hpp"
#include "core/DisplayManager.hpp"
#include "../window/WindowManager.hpp"
#include <algorithm>
#include <iostream>
#include <thread>

namespace H {

#if defined(WINDOWS)
HHOOK IO::keyboardHook = NULL;
#endif
std::unordered_map<int, HotKey> IO::hotkeys; // Map to store hotkeys by ID
bool IO::hotkeyEnabled = true;
int IO::hotkeyCount = 0;

IO::IO()
{
    std::cout << "IO constructor called" << std::endl;
    DisplayManager::Initialize();
    display = DisplayManager::GetDisplay();
    if (!display) {
        std::cerr << "Failed to get X11 display" << std::endl;
    }
    InitKeyMap();
}

IO::~IO() {
    std::cout << "IO destructor called" << std::endl;
    if (timerRunning && timerThread.joinable()) {
        timerRunning = false;
        timerThread.join();
    }
    
    // Don't close the display here, it's managed by DisplayManager
    display = nullptr;
}

void IO::InitKeyMap() {
    // Basic implementation of key map
    std::cout << "Initializing key map" << std::endl;
    // Add key mappings here as needed
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

// Static method to handle key events
void IO::HandleKeyEvent(XEvent& event) {
    XKeyEvent* keyEvent = reinterpret_cast<XKeyEvent*>(&event);
    KeySym keysym = XLookupKeysym(keyEvent, 0);

    for (const auto& [id, hotkey] : hotkeys) {
        if (hotkey.enabled && hotkey.key == keysym) {
            if (hotkey.callback) hotkey.callback();
        }
    }
}

// Static method to handle mouse events
void IO::HandleMouseEvent(XEvent& event) {
    XButtonEvent* buttonEvent = reinterpret_cast<XButtonEvent*>(&event);
    unsigned int button = buttonEvent->button;

    for (const auto& [id, hotkey] : hotkeys) {
        if (hotkey.enabled && StringToButton(hotkey.alias) == button) {
            if (hotkey.callback) hotkey.callback();
        }
    }
}

// Static method to handle key strings
Key IO::handleKeyString(const std::string& keystr) {
    // Use DisplayManager to get the display
    Display* display = DisplayManager::GetDisplay();
    if (!display) {
        std::cerr << "No display available for key handling" << std::endl;
        return 0;
    }
    
    KeySym keysym = 0;
    
    if (keystr.length() == 1) {
        // Single character key
        keysym = XStringToKeysym(keystr.c_str());
    } else {
        // Named key
        keysym = XStringToKeysym(keystr.c_str());
        if (keysym == NoSymbol) {
            // Try with "XK_" prefix
            std::string xkName = "XK_" + keystr;
            keysym = XStringToKeysym(xkName.c_str());
        }
    }
    
    if (keysym == NoSymbol) {
        // Try as a scan code
        int value = std::stoi(keystr);
        KeyCode keycode = value;
        keysym = XkbKeycodeToKeysym(display, keycode, 0, 0);
    }
    
    return keysym;
}

// Method to send keys
void IO::Send(const std::string& keys) {
    std::cout << "Sending keys: " << keys << std::endl;
    ProcessKeyCombination(keys);
}

// Method to process key combinations
void IO::ProcessKeyCombination(const std::string& keys) {
    std::cout << "Processing key combination: " << keys << std::endl;
}

// Method to suspend hotkeys
bool IO::Suspend(int id) {
    std::cout << "Suspending hotkey ID: " << id << std::endl;
    auto it = hotkeys.find(id);
    if (it != hotkeys.end()) {
        it->second.enabled = false;
        return true;
    }
    return false;
}

// Method to resume hotkeys
bool IO::Resume(int id) {
    std::cout << "Resuming hotkey ID: " << id << std::endl;
    auto it = hotkeys.find(id);
    if (it != hotkeys.end()) {
        it->second.enabled = true;
        return true;
    }
    return false;
}

// Add a hotkey with given key, modifiers, and callback function
bool IO::Hotkey(const std::string& hotkeyStr, std::function<void()> action, int id) {
    std::cout << "Setting up hotkey: " << hotkeyStr << std::endl;
    if (id == 0) {
        id = ++hotkeyCount;
    }
    
    // Parse the hotkey string to get key and modifiers
    std::string keyName;
    int modifiers = 0;
    
    // Get key and modifiers from hotkey string
    if (hotkeyStr.find("+") != std::string::npos) {
        // Multiple keys with modifiers
        modifiers = ParseModifiers(hotkeyStr);
        size_t pos = hotkeyStr.find_last_of('+');
        keyName = hotkeyStr.substr(pos+1);
    } else if (hotkeyStr.find("^") == 0) {
        // Control key
        modifiers = ControlMask;
        keyName = hotkeyStr.substr(1);
    } else if (hotkeyStr.find("!") == 0) {
        // Alt key
        modifiers = Mod1Mask;
        keyName = hotkeyStr.substr(1);
    } else if (hotkeyStr.find("#") == 0) {
        // Windows/Super key
        modifiers = Mod4Mask;
        keyName = hotkeyStr.substr(1);
    } else if (hotkeyStr.find("+") == 0) {
        // Shift key
        modifiers = ShiftMask;
        keyName = hotkeyStr.substr(1);
    } else {
        // Just a key, no modifiers
        keyName = hotkeyStr;
    }
    
    // Convert key name to virtual key code
    Key key = StringToVirtualKey(keyName);
    if (key == 0) {
        std::cerr << "Invalid key name: " << keyName << std::endl;
        return false;
    }
    
    // Create hotkey structure
    HotKey hotkey;
    hotkey.alias = hotkeyStr;
    hotkey.key = key;
    hotkey.modifiers = modifiers;
    hotkey.callback = action;
    hotkey.enabled = true;
    
    // Register the hotkey
    hotkeys[id] = hotkey;
    
    return true;
}

// Method to control send
void IO::ControlSend(const std::string& control, const std::string& keys) {
    std::cout << "Control send: " << control << " keys: " << keys << std::endl;
    // Use WindowManager to find the window
    wID hwnd = WindowManager::FindByTitle(control);
    if (!hwnd) {
        std::cerr << "Window not found: " << control << std::endl;
        return;
    }
    
    // Implementation to send keys to the window
}

// Method to get mouse position
int IO::GetMouse() {
    // No need to get root window here
    // Window root = DisplayManager::GetRootWindow();
    return 0;
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

Key IO::StringToButton(str buttonName) {
    buttonName = ToLower(buttonName);
    
    if (buttonName == "button1") return Button1;
    if (buttonName == "button2") return Button2;
    if (buttonName == "button3") return Button3;
    if (buttonName == "wheelup" || buttonName == "scrollup") return Button4;
    if (buttonName == "wheeldown" || buttonName == "scrolldown") return Button5;
    return 0;
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

// Set a timer for a specified function 
void IO::SetTimer(int milliseconds, const std::function<void()> &func)
{
    std::cout << "Setting timer for " << milliseconds << " ms" << std::endl;
    std::thread([=]()
                {
            std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
            func(); })
        .detach();
}

// Display a message box
void IO::MsgBox(const std::string& message) {
    // Stub implementation for message box
    std::cout << "Message Box: " << message << std::endl;
}

// Assign a hotkey to a specific ID
void IO::AssignHotkey(HotKey hotkey, int id) {
    // Generate a unique ID if not provided
    if (id == 0) {
        id = ++hotkeyCount;
    }
    
    // Register the hotkey
    hotkeys[id] = hotkey;
    
    // Platform-specific registration
#ifdef __linux__
    Display* display = DisplayManager::GetDisplay();
    if (!display) return;
    
    Window root = DisplayManager::GetRootWindow();
    
    KeyCode keycode = XKeysymToKeycode(display, hotkey.key);
    if (keycode == 0) {
        std::cerr << "Invalid key code for hotkey: " << hotkey.alias << std::endl;
        return;
    }
    
    Bool owner_events = True;
    
    if (XGrabKey(display, keycode, hotkey.modifiers, root, owner_events,
                GrabModeAsync, GrabModeAsync) != Success) {
        std::cerr << "Failed to grab key: " << hotkey.alias << std::endl;
    }
#endif
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
                        if (hotkey.callback)
                        {
                            std::cout << "Action from non-blocking callback for " << hotkey.alias << "\n";
                            hotkey.callback(); // Call the associated action
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

void IO::PressKey(const std::string& keyName, bool press) {
    std::cout << "Pressing key: " << keyName << " (press: " << press << ")" << std::endl;
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

bool IO::AddHotkey(const std::string& alias, Key key, int modifiers, std::function<void()> callback) {
    // Stub implementation for AddHotkey
    std::cout << "Adding hotkey: " << alias << std::endl;
    HotKey hotkey;
    hotkey.alias = alias;
    hotkey.key = key;
    hotkey.modifiers = modifiers;
    hotkey.callback = callback;
    
    hotkeyCount++;
    hotkeys[hotkeyCount] = hotkey;
    return true;
}

} // namespace H
