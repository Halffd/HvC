#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"
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
    
    // Start hotkey monitoring thread for X11
#ifdef __linux__
    if (display) {
        // Set up X11 error handler
        XSetErrorHandler([](Display*, XErrorEvent*) -> int { return 0; });
        
        // Start hotkey monitoring in a separate thread
        timerRunning = true;
        timerThread = std::thread([this]() {
            this->MonitorHotkeys();
        });
    }
#endif
}

IO::~IO() {
    std::cout << "IO destructor called" << std::endl;
    if (timerRunning && timerThread.joinable()) {
        timerRunning = false;
        timerThread.join();
    }
    
    // Ungrab all hotkeys before closing
#ifdef __linux__
    if (display) {
        Window root = DefaultRootWindow(display);
        for (const auto& [id, hotkey] : hotkeys) {
            if (hotkey.key != 0) {
                KeyCode keycode = XKeysymToKeycode(display, hotkey.key);
                if (keycode != 0) {
                    UngrabKeyWithVariants(keycode, hotkey.modifiers, root);
                }
            }
        }
    }
#endif
    
    // Don't close the display here, it's managed by DisplayManager
    display = nullptr;
}

// Helper method to grab a key with all modifier variants (NumLock, CapsLock)
void IO::GrabKeyWithVariants(KeyCode keycode, unsigned int modifiers, Window root, bool exclusive) {
    if (!display) return;
    
    // First, determine the mask for NumLock
    unsigned int numlockmask = 0;
    XModifierKeymap* modmap = XGetModifierMapping(display);
    if (modmap && modmap->max_keypermod > 0) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < modmap->max_keypermod; j++) {
                KeyCode kc = modmap->modifiermap[i * modmap->max_keypermod + j];
                if (kc == XKeysymToKeycode(display, XK_Num_Lock)) {
                    numlockmask = (1 << i);
                    break;
                }
            }
        }
        XFreeModifiermap(modmap);
    }
    
    // Define the modifier variants to handle
    unsigned int modVariants[] = {
        0,                      // Base modifiers
        LockMask,               // CapsLock
        numlockmask,            // NumLock
        LockMask | numlockmask  // Both CapsLock and NumLock
    };
    
    // Grab with all modifier variants
    for (unsigned int variant : modVariants) {
        // Skip if this specific variant isn't applicable
        // (when numlockmask is 0 and variant involves numlockmask)
        if ((variant & numlockmask) && numlockmask == 0)
            continue;
        
        unsigned int finalMods = modifiers | variant;
        
        // Ungrab first in case it's already grabbed
        XUngrabKey(display, keycode, finalMods, root);
        
        if (exclusive) {
            // Exclusive grab - like AHK's normal behavior
            Status status = XGrabKey(display, keycode, finalMods, root, True, 
                                    GrabModeAsync, GrabModeAsync);
            
            if (status != Success) {
                std::cerr << "Failed to grab key (code: " << (int)keycode 
                        << ") with modifiers: " << finalMods << std::endl;
            }
        } else {
            // Non-exclusive - like AHK's ~ prefix
            // Just ensure we're listening for key events on the root window
            XSelectInput(display, root, KeyPressMask | KeyReleaseMask);
        }
    }
    
    // Ensure changes are flushed to the X server immediately
    XSync(display, False);
}

// Helper method to ungrab a key with all modifier variants
void IO::UngrabKeyWithVariants(KeyCode keycode, unsigned int modifiers, Window root) {
    if (!display) return;
    
    // Determine the mask for NumLock
    unsigned int numlockmask = 0;
    XModifierKeymap* modmap = XGetModifierMapping(display);
    if (modmap && modmap->max_keypermod > 0) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < modmap->max_keypermod; j++) {
                KeyCode kc = modmap->modifiermap[i * modmap->max_keypermod + j];
                if (kc == XKeysymToKeycode(display, XK_Num_Lock)) {
                    numlockmask = (1 << i);
                    break;
                }
            }
        }
        XFreeModifiermap(modmap);
    }
    
    // Ungrab with all modifier variants
    XUngrabKey(display, keycode, modifiers, root);
    XUngrabKey(display, keycode, modifiers | LockMask, root);
    
    if (numlockmask) {
        XUngrabKey(display, keycode, modifiers | numlockmask, root);
        XUngrabKey(display, keycode, modifiers | LockMask | numlockmask, root);
    }
    
    // Ensure changes are flushed to the X server immediately
    XSync(display, False);
}

// X11 hotkey monitoring thread
void IO::MonitorHotkeys() {
#ifdef __linux__
    if (!display) return;
    
    std::cout << "Starting hotkey monitoring thread" << std::endl;
    XEvent event;
    Window root = DefaultRootWindow(display);
    
    // Select events on the root window
    XSelectInput(display, root, KeyPressMask);
    
    // Helper function to check if a keysym is a modifier
    auto IsModifierKey = [](KeySym ks) -> bool {
        return ks == XK_Shift_L || ks == XK_Shift_R ||
               ks == XK_Control_L || ks == XK_Control_R ||
               ks == XK_Alt_L || ks == XK_Alt_R ||
               ks == XK_Meta_L || ks == XK_Meta_R ||
               ks == XK_Super_L || ks == XK_Super_R ||
               ks == XK_Hyper_L || ks == XK_Hyper_R ||
               ks == XK_Caps_Lock || ks == XK_Shift_Lock ||
               ks == XK_Num_Lock || ks == XK_Scroll_Lock;
    };

    while (timerRunning) {
        // Check for pending events
        if (XPending(display) > 0) {
            XNextEvent(display, &event);
            
            if (event.type == KeyPress) {
                XKeyEvent* keyEvent = &event.xkey;
                KeySym keysym = XLookupKeysym(keyEvent, 0);

                // --- FIX: Ignore events for modifier keys themselves ---
                if (IsModifierKey(keysym)) {
                    continue; 
                }
                // ------------------------------------------------------
                
                std::cout << "KeyPress event detected: " << XKeysymToString(keysym) 
                          << " (keycode: " << keyEvent->keycode << ")"
                          << " with state: " << keyEvent->state << std::endl;
                
                // --- FIX: Clean the modifier state mask --- 
                // Include standard modifiers + LockMask (Caps Lock)
                // Exclude Mod2Mask (NumLock) and Mod3Mask (ScrollLock) typically
                unsigned int relevantModifiers = ShiftMask | LockMask | ControlMask | Mod1Mask | Mod4Mask | Mod5Mask;
                unsigned int cleanedState = keyEvent->state & relevantModifiers;
                // ------------------------------------------
                
                // Find and execute matching hotkeys
                for (const auto& [id, hotkey] : hotkeys) {
                    if (hotkey.enabled) {
                        // KeySym was stored in hotkey.key during registration
                        if (hotkey.key == keysym && 
                            cleanedState == hotkey.modifiers) { // Compare base key AND cleaned modifier state
                            std::cout << "Hotkey matched: " << hotkey.alias << " (state: " << cleanedState << " vs expected: " << hotkey.modifiers << ")" << std::endl;
                            // Execute the callback in a separate thread to avoid blocking
                            if (hotkey.callback) {
                                std::thread([callback = hotkey.callback]() {
                                    callback();
                                }).detach();
                            }
                            // Consider breaking if only one hotkey should match per event
                             break; 
                        }
                    }
                }
            }
        }
        
        // Sleep to avoid high CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout << "Hotkey monitoring thread stopped" << std::endl;
#endif
}

void IO::InitKeyMap() {
    // Basic implementation of key map
    std::cout << "Initializing key map" << std::endl;
    
    // Initialize key mappings for common keys
#ifdef __linux__
    keyMap["esc"] = XK_Escape;
    keyMap["enter"] = XK_Return;
    keyMap["space"] = XK_space;
    keyMap["tab"] = XK_Tab;
    keyMap["backspace"] = XK_BackSpace;
    keyMap["ctrl"] = XK_Control_L;
    keyMap["alt"] = XK_Alt_L;
    keyMap["shift"] = XK_Shift_L;
    keyMap["win"] = XK_Super_L;
    keyMap["lwin"] = XK_Super_L; // Add alias for Left Win
    keyMap["rwin"] = XK_Super_R; // Add Right Win
    keyMap["up"] = XK_Up;
    keyMap["down"] = XK_Down;
    keyMap["left"] = XK_Left;
    keyMap["right"] = XK_Right;
    keyMap["delete"] = XK_Delete;
    keyMap["insert"] = XK_Insert;
    keyMap["home"] = XK_Home;
    keyMap["end"] = XK_End;
    keyMap["pageup"] = XK_Page_Up;
    keyMap["pagedown"] = XK_Page_Down;
    keyMap["printscreen"] = XK_Print;
    keyMap["scrolllock"] = XK_Scroll_Lock;
    keyMap["pause"] = XK_Pause;
    keyMap["capslock"] = XK_Caps_Lock;
    keyMap["numlock"] = XK_Num_Lock;
    keyMap["menu"] = XK_Menu; // Add Menu key
    
    // Numpad keys
    keyMap["kp_0"] = XK_KP_0;
    keyMap["kp_1"] = XK_KP_1;
    keyMap["kp_2"] = XK_KP_2;
    keyMap["kp_3"] = XK_KP_3;
    keyMap["kp_4"] = XK_KP_4;
    keyMap["kp_5"] = XK_KP_5;
    keyMap["kp_6"] = XK_KP_6;
    keyMap["kp_7"] = XK_KP_7;
    keyMap["kp_8"] = XK_KP_8;
    keyMap["kp_9"] = XK_KP_9;
    keyMap["kp_insert"] = XK_KP_Insert; // KP 0
    keyMap["kp_end"] = XK_KP_End;       // KP 1
    keyMap["kp_down"] = XK_KP_Down;     // KP 2
    keyMap["kp_pagedown"] = XK_KP_Page_Down; // KP 3
    keyMap["kp_left"] = XK_KP_Left;     // KP 4
    keyMap["kp_begin"] = XK_KP_Begin;   // KP 5
    keyMap["kp_right"] = XK_KP_Right;    // KP 6
    keyMap["kp_home"] = XK_KP_Home;     // KP 7
    keyMap["kp_up"] = XK_KP_Up;       // KP 8
    keyMap["kp_pageup"] = XK_KP_Page_Up;   // KP 9
    keyMap["kp_delete"] = XK_KP_Delete;   // KP Decimal
    keyMap["kp_decimal"] = XK_KP_Decimal;
    keyMap["kp_add"] = XK_KP_Add;
    keyMap["kp_subtract"] = XK_KP_Subtract;
    keyMap["kp_multiply"] = XK_KP_Multiply;
    keyMap["kp_divide"] = XK_KP_Divide;
    keyMap["kp_enter"] = XK_KP_Enter;
    
    // Function keys
    keyMap["f1"] = XK_F1;
    keyMap["f2"] = XK_F2;
    keyMap["f3"] = XK_F3;
    keyMap["f4"] = XK_F4;
    keyMap["f5"] = XK_F5;
    keyMap["f6"] = XK_F6;
    keyMap["f7"] = XK_F7;
    keyMap["f8"] = XK_F8;
    keyMap["f9"] = XK_F9;
    keyMap["f10"] = XK_F10;
    keyMap["f11"] = XK_F11;
    keyMap["f12"] = XK_F12;
    
    // Media keys (using XF86keysym.h - ensure it's included)
    keyMap["volumeup"] = XF86XK_AudioRaiseVolume;
    keyMap["volumedown"] = XF86XK_AudioLowerVolume;
    keyMap["mute"] = XF86XK_AudioMute;
    keyMap["play"] = XF86XK_AudioPlay;
    keyMap["pause"] = XF86XK_AudioPause;
    keyMap["playpause"] = XF86XK_AudioPlay; // Often mapped to the same key
    keyMap["stop"] = XF86XK_AudioStop;
    keyMap["prev"] = XF86XK_AudioPrev;
    keyMap["next"] = XF86XK_AudioNext;

    // Punctuation and symbols
    keyMap["comma"] = XK_comma; // Add comma
    keyMap["period"] = XK_period;
    keyMap["semicolon"] = XK_semicolon;
    keyMap["slash"] = XK_slash;
    keyMap["backslash"] = XK_backslash;
    keyMap["bracketleft"] = XK_bracketleft;
    keyMap["bracketright"] = XK_bracketright;
    keyMap["minus"] = XK_minus; // Add minus
    keyMap["equal"] = XK_equal; // Add equal
    keyMap["grave"] = XK_grave; // Tilde key (~)
    keyMap["apostrophe"] = XK_apostrophe;
    
    // Letter keys (a-z)
    for (char c = 'a'; c <= 'z'; ++c) {
        keyMap[std::string(1, c)] = XStringToKeysym(std::string(1, c).c_str());
    }
    
    // Number keys (0-9)
    for (char c = '0'; c <= '9'; ++c) {
        keyMap[std::string(1, c)] = XStringToKeysym(std::string(1, c).c_str());
    }
    
    // Button names (for mouse events)
    keyMap["button1"] = Button1;
    keyMap["button2"] = Button2;
    keyMap["button3"] = Button3;
    keyMap["button4"] = Button4; // Wheel up
    keyMap["button5"] = Button5; // Wheel down
    
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
Key IO::handleKeyString(const std::string& key) {
    lo.debug("Handling key string: " + key);
    
    // Handle keycodes (kcXXX)
    if (key.size() > 2 && key.substr(0, 2) == "kc") {
        try {
            int keycode = std::stoi(key.substr(2));
            lo.debug("Detected direct keycode: " + std::to_string(keycode));
            return keycode;
        } catch (const std::exception& e) {
            lo.error("Failed to parse keycode from '" + key + "': " + e.what());
            return -1;
        }
    }
    
    // Special handling for NoSymbol
    if (key == "NoSymbol") {
        lo.debug("Explicitly handling NoSymbol as keysym 0x0");
        return 0x0; // Use keysym 0x0 for NoSymbol as requested
    }
    
    // Handle the "Menu" key explicitly
    if (key == "Menu") {
        lo.debug("Explicitly handling Menu key as keycode 135");
        return 135; // Direct keycode for Menu key on many keyboards
    }
    
    // The rest of the implementation handles normal keys
    KeySym keysym = XStringToKeysym(key.c_str());
    if (keysym == NoSymbol) {
        lo.warning("Key '" + key + "' could not be converted to KeySym (NoSymbol)");
        return -1;
    }
    
    Display* display = DisplayManager::GetDisplay();
    if (!display) {
        lo.error("No X display available for key conversion");
        return -1;
    }
    
    int keycode = XKeysymToKeycode(display, keysym);
    lo.debug("Converted key '" + key + "' to keycode " + std::to_string(keycode) + " (keysym: " + std::to_string(keysym) + ")");
    
    if (keycode == 0) {
        lo.warning("KeySym for '" + key + "' could not be converted to keycode");
        return -1;
    }
    
    return keycode;
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
    std::string hotkeyStrCopy = hotkeyStr;
    bool exclusive = true; // Default to exclusive grab (block other apps)
    
    // Check if this is a non-blocking hotkey (starts with ~)
    if (hotkeyStrCopy.length() > 0 && hotkeyStrCopy[0] == '~') {
        exclusive = false;
        hotkeyStrCopy = hotkeyStrCopy.substr(1);
        std::cout << "  Non-exclusive hotkey detected: " << hotkeyStrCopy << std::endl;
    }
    
    // Check if this is a direct keycode (format: "kcXXX")
    bool isDirectKeycode = false;
    if (hotkeyStrCopy.size() > 2 && hotkeyStrCopy.substr(0, 2) == "kc") {
        isDirectKeycode = true;
        keyName = hotkeyStrCopy;
        std::cout << "Direct keycode hotkey detected: " << keyName << std::endl;
    } else {
        // Split by '+' to separate modifiers and key
        std::vector<std::string> parts;
        size_t pos = 0;
        std::string delimiter = "+";
        std::string str = hotkeyStrCopy;
        
        while ((pos = str.find(delimiter)) != std::string::npos) {
            std::string part = str.substr(0, pos);
            // Remove leading/trailing whitespace manually
            part.erase(0, part.find_first_not_of(" \t\n\r\f\v"));
            part.erase(part.find_last_not_of(" \t\n\r\f\v") + 1);
            parts.push_back(part);
            str.erase(0, pos + delimiter.length());
        }
        
        // Handle the last part
        str.erase(0, str.find_first_not_of(" \t\n\r\f\v"));
        str.erase(str.find_last_not_of(" \t\n\r\f\v") + 1);
        if (!str.empty()) {
            parts.push_back(str);
        }
        
        if (parts.empty()) {
            std::cerr << "Invalid hotkey format: " << hotkeyStr << std::endl;
            return false;
        }
        
        // Last part is the key
        keyName = parts.back();
        parts.pop_back();
        
        // Process modifiers (convert to lowercase)
        for (const auto& mod : parts) {
            std::string modLower = mod;
            std::transform(modLower.begin(), modLower.end(), modLower.begin(), ::tolower);
            
            if (modLower == "ctrl" || modLower == "control") {
                modifiers |= ControlMask;
            } else if (modLower == "shift") {
                modifiers |= ShiftMask;
            } else if (modLower == "alt") {
                modifiers |= Mod1Mask;
            } else if (modLower == "win" || modLower == "super") {
                modifiers |= Mod4Mask;
            }
        }
    }
    
    std::cout << "  Parsed modifiers: " << modifiers << ", key: '" << keyName << "'" << std::endl;
    
    // Handle special keys
    KeySym keysym = 0; // Use KeySym for X11 processing
    KeyCode keycode = 0; // Use KeyCode for X11 registration
    
    if (isDirectKeycode) {
        // For direct keycodes, handleKeyString returns the keycode directly
        keycode = handleKeyString(keyName);
        if (keycode == -1) {
            std::cerr << "Failed to interpret direct keycode: " << keyName << std::endl;
            return false;
        }
        // Try to get the corresponding keysym for internal storage/logging if needed
        if (display) {
            keysym = XkbKeycodeToKeysym(display, keycode, 0, 0);
        }
        std::cout << "  Direct keycode: " << (int)keycode << " (keysym: " << keysym << ")" << std::endl;

    } else {
        // For named keys, find the keysym first
        std::string keyNameLower = keyName;
        std::transform(keyNameLower.begin(), keyNameLower.end(), keyNameLower.begin(), ::tolower);
        
        auto it = keyMap.find(keyNameLower);
        if (it != keyMap.end()) {
            keysym = it->second;
        } else if (keyName.length() == 1) {
             // Try converting single chars that might not be in the map
            keysym = XStringToKeysym(keyName.c_str());
        } 
        // Add more specific fallbacks if needed, e.g.
        // else if (keyNameLower == "specific_key") keysym = XK_SpecificKey;
        
        if (keysym == NoSymbol) {
            std::cerr << "Invalid key name or not found in map: " << keyName << std::endl;
            return false;
        }
        std::cout << "  Converted name '" << keyName << "' to keysym: " << keysym << std::endl;
        
        // Get the keycode from the keysym
        if (display) {
            keycode = XKeysymToKeycode(display, keysym);
            if (keycode == 0) {
                 std::cerr << "Could not convert keysym " << keysym << " to keycode for key: " << keyName << std::endl;
                // Optionally, treat as non-fatal if monitoring catches it
                // return false; 
            }
            std::cout << "  Keysym " << keysym << " maps to keycode: " << (int)keycode << std::endl;
        }
    }
    
    // If we couldn't get a valid keycode, we can't register the grab
    // However, we might still store the hotkey if event monitoring can handle it by keysym
    bool canGrab = (display && keycode != 0);

    // Create hotkey structure
    HotKey hotkey;
    hotkey.alias = hotkeyStr;
    hotkey.key = keysym; // Store keysym for event monitoring
    hotkey.modifiers = modifiers;
    hotkey.callback = action;
    hotkey.enabled = true;
    hotkey.exclusive = exclusive;
    
    // Register the hotkey internally regardless of grab success
    hotkeys[id] = hotkey;
    
    // Grab the key using X11 if possible
#ifdef __linux__
    if (canGrab) {
        Window root = DefaultRootWindow(display);
        
        // Use our improved method to handle modifier variants
        GrabKeyWithVariants(keycode, modifiers, root, exclusive);
        
        std::cout << "Successfully registered hotkey: " << hotkeyStr 
                 << " (exclusive: " << (exclusive ? "yes" : "no") << ")" << std::endl;
    }
#endif
    
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
    
    Window root = DefaultRootWindow(display);
    
    KeyCode keycode = XKeysymToKeycode(display, hotkey.key);
    if (keycode == 0) {
        std::cerr << "Invalid key code for hotkey: " << hotkey.alias << std::endl;
        return;
    }
    
    // Ungrab first in case it's already grabbed
    XUngrabKey(display, keycode, hotkey.modifiers, root);
    
    // Grab the key
    if (XGrabKey(display, keycode, hotkey.modifiers, root, False,
                GrabModeAsync, GrabModeAsync) != Success) {
        std::cerr << "Failed to grab key: " << hotkey.alias << std::endl;
    }
    
    XFlush(display);
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

// New methods for dynamic hotkey grabbing/ungrabbing
bool IO::GrabHotkey(int hotkeyId) {
#ifdef __linux__
    if (!display) return false;
    
    auto it = hotkeys.find(hotkeyId);
    if (it == hotkeys.end()) {
        std::cerr << "Hotkey ID not found: " << hotkeyId << std::endl;
        return false;
    }
    
    const HotKey& hotkey = it->second;
    Window root = DefaultRootWindow(display);
    KeyCode keycode = XKeysymToKeycode(display, hotkey.key);
    
    if (keycode == 0) {
        std::cerr << "Invalid keycode for hotkey: " << hotkey.alias << std::endl;
        return false;
    }
    
    // Use our improved method to grab with all modifier variants
    GrabKeyWithVariants(keycode, hotkey.modifiers, root, hotkey.exclusive);
    
    std::cout << "Successfully grabbed hotkey: " << hotkey.alias << std::endl;
    return true;
#else
    return false;
#endif
}

bool IO::UngrabHotkey(int hotkeyId) {
#ifdef __linux__
    if (!display) return false;
    
    auto it = hotkeys.find(hotkeyId);
    if (it == hotkeys.end()) {
        std::cerr << "Hotkey ID not found: " << hotkeyId << std::endl;
        return false;
    }
    
    const HotKey& hotkey = it->second;
    Window root = DefaultRootWindow(display);
    KeyCode keycode = XKeysymToKeycode(display, hotkey.key);
    
    if (keycode == 0) {
        std::cerr << "Invalid keycode for hotkey: " << hotkey.alias << std::endl;
        return false;
    }
    
    // Use our improved method to ungrab with all modifier variants
    UngrabKeyWithVariants(keycode, hotkey.modifiers, root);
    
    std::cout << "Successfully ungrabbed hotkey: " << hotkey.alias << std::endl;
    return true;
#else
    return false;
#endif
}

bool IO::GrabHotkeysByPrefix(const std::string& prefix) {
#ifdef __linux__
    if (!display) return false;
    
    bool success = true;
    for (const auto& [id, hotkey] : hotkeys) {
        if (hotkey.alias.find(prefix) == 0) {
            if (!GrabHotkey(id)) {
                success = false;
            }
        }
    }
    return success;
#else
    return false;
#endif
}

bool IO::UngrabHotkeysByPrefix(const std::string& prefix) {
#ifdef __linux__
    if (!display) return false;
    
    bool success = true;
    for (const auto& [id, hotkey] : hotkeys) {
        if (hotkey.alias.find(prefix) == 0) {
            if (!UngrabHotkey(id)) {
                success = false;
            }
        }
    }
    return success;
#else
    return false;
#endif
}

} // namespace H
