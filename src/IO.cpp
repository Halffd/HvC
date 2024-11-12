#include "IO.hpp"
#include "Windowmanager.hpp"

#include <string>
#include <algorithm>
#include <winuser.h>

HHOOK IO::keyboardHook = NULL;
std::unordered_map<int, Hotkey> IO::hotkeys; // Map to store hotkeys by ID
IO::IO(){
    //ioWindow = WindowManager::NewWindow("IO");
}
void IO::removeSpecialCharacters(str& keyName) {
    // Define the characters to remove
    const std::string charsToRemove = "^+!#";

    // Remove characters
    keyName.erase(std::remove_if(keyName.begin(), keyName.end(),
                                  [&charsToRemove](char c) {
                                      return charsToRemove.find(c) != std::string::npos;
                                  }),
                  keyName.end());
}
// Helper function to convert string to virtual key code
int IO::StringToVirtualKey(str keyName) {
    removeSpecialCharacters(keyName);
    if (keyName.length() == 1) {
        return VkKeyScan(keyName[0]);
    }
    keyName = ToLower(keyName);
    // Map string names to virtual key codes
    if (keyName == "home") return VK_HOME;
    if (keyName == "end") return VK_END;
    if (keyName == "pgup") return VK_PRIOR;
    if (keyName == "pgdn") return VK_NEXT;
    if (keyName == "insert") return VK_INSERT;
    if (keyName == "delete") return VK_DELETE;
    if (keyName == "numpad0") return VK_NUMPAD0;
    if (keyName == "numpad1") return VK_NUMPAD1;
    if (keyName == "numpad2") return VK_NUMPAD2;
    if (keyName == "numpad3") return VK_NUMPAD3;
    if (keyName == "numpad4") return VK_NUMPAD4;
    if (keyName == "numpad5") return VK_NUMPAD5;
    if (keyName == "numpad6") return VK_NUMPAD6;
    if (keyName == "numpad7") return VK_NUMPAD7;
    if (keyName == "numpad8") return VK_NUMPAD8;
    if (keyName == "numpad9") return VK_NUMPAD9;
    if (keyName == "up") return VK_UP;
    if (keyName == "down") return VK_DOWN;
    if (keyName == "left") return VK_LEFT;
    if (keyName == "right") return VK_RIGHT;
    if (keyName == "f1") return VK_F1;
    if (keyName == "f2") return VK_F2;
    if (keyName == "f3") return VK_F3;
    if (keyName == "f4") return VK_F4;
    if (keyName == "f5") return VK_F5;
    if (keyName == "f6") return VK_F6;
    if (keyName == "f7") return VK_F7;
    if (keyName == "f8") return VK_F8;
    if (keyName == "f9") return VK_F9;
    if (keyName == "f10") return VK_F10;
    if (keyName == "f11") return VK_F11;
    if (keyName == "f12") return VK_F21;
    if (keyName == "f13") return VK_F13;
    if (keyName == "f14") return VK_F14;
    if (keyName == "f15") return VK_F15;
    if (keyName == "f16") return VK_F16;
    if (keyName == "f17") return VK_F17;
    if (keyName == "f18") return VK_F18;
    if (keyName == "f19") return VK_F19;
    if (keyName == "f20") return VK_F20;
    if (keyName == "f21") return VK_F21;
    if (keyName == "f22") return VK_F22;
    if (keyName == "f23") return VK_F23;
    if (keyName == "f24") return VK_F24;
    if (keyName == "enter") return VK_RETURN;
    if (keyName == "space") return VK_SPACE;
    if (keyName == "lbutton") return VK_LBUTTON;
    if (keyName == "rbutton") return VK_RBUTTON;
    if (keyName == "apps") return VK_APPS;
    if (keyName == "win") return 0x5B;
    if (keyName == "lwin") return VK_LWIN;
    if (keyName == "rwin") return VK_RWIN;
    if (keyName == "ctrl") return VK_CONTROL;
    if (keyName == "lctrl") return VK_LCONTROL;
    if (keyName == "rctrl") return VK_RCONTROL;
    if (keyName == "shift") return VK_SHIFT;
    if (keyName == "lshift") return VK_LSHIFT;
    if (keyName == "rshift") return VK_RSHIFT;
    if (keyName == "alt") return VK_MENU;
    if (keyName == "lalt") return VK_LMENU;
    if (keyName == "ralt") return VK_RMENU;
    if (keyName == "backspace") return VK_BACK;
    if (keyName == "tab") return VK_TAB;
    if (keyName == "capslock") return VK_CAPITAL;
    if (keyName == "numlock") return VK_NUMLOCK;
    if (keyName == "scrolllock") return VK_SCROLL;
    if (keyName == "pausebreak") return VK_PAUSE;
    if (keyName == "printscreen") return VK_SNAPSHOT;
    if (keyName == "volumeup") return VK_VOLUME_UP;
    if (keyName == "volumedown") return VK_VOLUME_DOWN;

    return 0; // Default case for unrecognized keys
}

void IO::Send(cstr keys) {
    ProcessKeyCombination(keys);
}

void IO::ProcessKeyCombination(cstr keys) {
    int modifiers = 0;

    // Parse the input string for modifiers and special sequences
    for (size_t i = 0; i < keys.length(); ++i) {
        if (keys[i] == '{') {
            // Start of a special key sequence
            size_t end = keys.find('}', i);
            if (end != str::npos) {
                str sequence = keys.substr(i + 1, end - i - 1);
                if (sequence == "Alt down") {
                    modifiers |= MOD_ALT;
                    keybd_event(VK_MENU, 0, 0, 0); // Press down Alt
                } else if (sequence == "Alt up") {
                    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0); // Release Alt
                } else if (sequence == "Ctrl down") {
                    modifiers |= MOD_CONTROL;
                    keybd_event(VK_CONTROL, 0, 0, 0); // Press down Ctrl
                } else if (sequence == "Ctrl up") {
                    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0); // Release Ctrl
                } else if (sequence == "Shift down") {
                    modifiers |= MOD_SHIFT;
                    keybd_event(VK_SHIFT, 0, 0, 0); // Press down Shift
                } else if (sequence == "Shift up") {
                    keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0); // Release Shift
                } else if (sequence.length() >= 5 && sequence.compare(sequence.length() - 5, 5, " down") == 0) {
                    str key = sequence.substr(0, sequence.length() - 5); // Remove " down"
                    int virtualKey = StringToVirtualKey(key);
                    if (virtualKey) {
                        keybd_event(virtualKey, 0, 0, 0); // Press down
                    }
                } else if (sequence.length() >= 4 && sequence.compare(sequence.length() - 4, 4, " up") == 0) {
                    str key = sequence.substr(0, sequence.length() - 4); // Remove " up"
                    int virtualKey = StringToVirtualKey(key);
                    if (virtualKey) {
                        keybd_event(virtualKey, 0, KEYEVENTF_KEYUP, 0); // Release
                    }
                } else {
                    int virtualKey = StringToVirtualKey(sequence);
                    if (virtualKey) {
                        keybd_event(virtualKey, 0, 0, 0); // Press down
                        keybd_event(virtualKey, 0, KEYEVENTF_KEYUP, 0); // Release
                    }
                }
                i = end; // Move past the closing brace
                continue;
            }
        }

        // Regular character processing
        int virtualKey = StringToVirtualKey(str(1, keys[i]));
        if (virtualKey) {
            keybd_event(virtualKey, 0, 0, 0); // Press down
            keybd_event(virtualKey, 0, KEYEVENTF_KEYUP, 0); // Release
        }
    }

    // Release any held modifiers
    if (modifiers & MOD_ALT) keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    if (modifiers & MOD_CONTROL) keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    if (modifiers & MOD_SHIFT) keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
}

void IO::AddHotkey(cstr hotkeyStr, std::function<void()> action, bool blockInput, int id) {
    str keyStr = hotkeyStr;
    int modifiers = ParseModifiers(keyStr);
    int virtualKey = StringToVirtualKey(keyStr);
    
    if (virtualKey) {
        AssignHotkey({modifiers, {virtualKey, keyStr}, action, blockInput}, id);
    } else {
        std::cerr << "Invalid key name: " << keyStr;
    }
}

void IO::ControlSend(cstr control, cstr keys) {
    HWND hwnd = WindowManager::Find(control);
    if (hwnd) {
        for (char key : keys) {
            int virtualKey = StringToVirtualKey(str(1, key));
            if (virtualKey) {
                SendMessage(hwnd, WM_KEYDOWN, virtualKey, 0);
                SendMessage(hwnd, WM_KEYUP, virtualKey, 0);
            }
        }
    } else {
        std::cerr << "Window not found!";
    }
}

void IO::SetTimer(int milliseconds, const std::function<void()>& func) {
    std::thread([=]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
        func();
    }).detach();
}

void IO::MsgBox(cstr message) {
    MessageBoxA(NULL, message.c_str(), "Message", MB_OK);
}
void IO::AssignHotkey(Hotkey hotkey, int id) {
    if (id == -1) {
        id = ++hotkeyCount; // Auto increment if id is not provided
    }
    if(hotkey.blockInput){
        if (RegisterHotKey(0, id, hotkey.modifiers, hotkey.key.virtualKey)) {
            lo << "Hotkey registered: " << hotkey.key.name;
        } else {
            std::cerr << "Failed to register hotkey: " << GetLastError();
            return;
        }
    } 
    hotkeys[id] = hotkey;
}

LRESULT CALLBACK IO::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;

        for (const auto& [id, hotkey] : hotkeys) {
            if(!hotkey.blockInput){
                if (pKeyboard->vkCode == hotkey.key.virtualKey) {
                    if (hotkey.action) {
                        hotkey.action(); // Call the associated action
                    }
                    break; // Exit after the first match
                }
            }
        }
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}
void IO::HotkeyListen() {
    MSG msg;
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (!keyboardHook) {
        std::cerr << "Failed to install hook: " << GetLastError() << std::endl;
        return;
    }
    while (GetMessage(&msg, 0, 0, 0)) {
        if (msg.message == WM_HOTKEY && hotkeys.count(msg.wParam)) {
            Hotkey hotkey = hotkeys[msg.wParam];
            if (hotkey.action) {
                hotkey.action();
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(keyboardHook);
}

int IO::ParseModifiers(str str) {
    int modifiers = 0;
    if (str.find("+") != str::npos) {
        modifiers |= MOD_SHIFT;
        str.erase(str.find("+"), 1);
    }
    if (str.find("^") != str::npos) {
        modifiers |= MOD_CONTROL;
        str.erase(str.find("^"), 1);
    }
    if (str.find("!") != str::npos) {
        modifiers |= MOD_ALT;
        str.erase(str.find("!"), 1);
    }
    if (str.find("#") != str::npos) {
        modifiers |= MOD_WIN;
        str.erase(str.find("#"), 1);
    }
    return modifiers;
}
void IO::HandleKeyAction(cstr action, cstr keyName) {
    int virtualKey = StringToVirtualKey(keyName);
    if (virtualKey) {
        if (action == "down") {
            keybd_event(virtualKey, 0, KEYEVENTF_EXTENDEDKEY, 0); // Press key
        } else if (action == "up") {
            keybd_event(virtualKey, 0, KEYEVENTF_KEYUP, 0); // Release key
        } else if (action == "toggle") {
            // Check the current state and toggle accordingly
            if (GetAsyncKeyState(virtualKey) & 0x8000) {
                keybd_event(virtualKey, 0, KEYEVENTF_KEYUP, 0); // Release if pressed
            } else {
                keybd_event(virtualKey, 0, 0, 0); // Press if released
            }
        } else {
            keybd_event(virtualKey, 0, NULL, 0);
        }
    }
}
int IO::GetState(cstr keyName, cstr mode) {
    // Define the virtual key code
    int virtualKey = 0;

    // Check for known key names and map them to virtual key codes
    virtualKey = StringToVirtualKey(keyName);
    
    // Retrieve the state using GetAsyncKeyState
    if (mode.empty() || mode == "P") {
        // Physical state
        return (GetAsyncKeyState(virtualKey) & 0x8000) ? 1 : 0;
    } else if (mode == "T") {
        // Toggle state for keys like CapsLock, NumLock, ScrollLock
        return (GetKeyState(virtualKey) & 0x1) ? 1 : 0;
    }

    return 0; // Default case if mode is not recognized
}