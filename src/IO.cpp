#include "IO.hpp"

// Helper function to convert string to virtual key code
int IO::StringToVirtualKey(str keyName) {
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
str IO::ToLower(cstr string) {
    str lowerStr = string;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
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

void IO::RegisterHotkey(Hotkey hotkey, int id) {
    if (id == -1) {
        id = ++hotkeyCount; // Auto increment if id is not provided
    }
    if (RegisterHotKey(NULL, id, hotkey.modifiers, hotkey.key.virtualKey)) {
        hotkeys[id] = hotkey;
        lo << "Hotkey registered: " << hotkey.key.name;
    } else {
        std::cerr << "Failed to register hotkey: " << GetLastError();
    }
}

void IO::AddHotkey(cstr hotkeyStr, std::function<void()> action, bool blockInput, int id) {
    str keyStr = hotkeyStr;
    int modifiers = ParseModifiers(keyStr);
    int virtualKey = StringToVirtualKey(keyStr);
    
    if (virtualKey) {
        RegisterHotkey({modifiers, {virtualKey, keyStr}, action, blockInput}, id);
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

void IO::HotkeyListen() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            Hotkey hotkey = hotkeys[msg.wParam];
            lo << "Hotkey pressed: " << hotkey.key.name;
            if (hotkey.blockInput) {
                // Block input while hotkey is active
                // Implementation of input blocking can be done here
            }
            if (hotkey.action) {
                hotkey.action(); // Execute the associated action
            }
            MsgBox("Hotkey activated!");
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
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
    if (action == "down") {
        keybd_event(virtualKey, 0, 0, 0);
    } else if (action == "up") {
        keybd_event(virtualKey, 0, KEYEVENTF_KEYUP, 0);
    }
}
void printBinary(short value) {
    for (int i = sizeof(value) * 8 - 1; i >= 0; --i) {
        printf("%d", (value >> i) & 1);
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
        short state = GetAsyncKeyState(virtualKey);
            
            // Print the state in hexadecimal
            printf("Key State (Hex): 0x%04X\n", state);
            
            // Print the state in binary
            printf("Key State (Binary): ");
            printBinary(state);
            printf("\n");

            // Bitwise operations
            short and1 = state & 0x8;
            short and2 = state & 0x8;
            short and3 = state & 0x80;
            short and4 = state & 0x8000;
            short and5 = state & 0x800;
            short and6 = state & 0x0001;
            short and7 = state & 0x1;
            short and8 = state & virtualKey;

            // Print results of bitwise operations in hexadecimal and binary
            printf("and (0x8) (Hex): 0x%04X, (Binary): ", and1);
            printBinary(and1);
            printf("\n");

            printf("and2 (0x8) (Hex): 0x%04X, (Binary): ", and2);
            printBinary(and2);
            printf("\n");

            printf("and3 (0x80) (Hex): 0x%04X, (Binary): ", and3);
            printBinary(and3);
            printf("\n");

            printf("and4 (0x8000) (Hex): 0x%04X, (Binary): ", and4);
            printBinary(and4);
            printf("\n");

            printf("and5 (0x800) (Hex): 0x%04X, (Binary): ", and5);
            printBinary(and5);
            printf("\n");

            printf("and6 (0x0001) (Hex): 0x%04X, (Binary): ", and6);
            printBinary(and6);
            printf("\n");

            printf("and7 (0x1) (Hex): 0x%04X, (Binary): ", and7);
            printBinary(and7);
            printf("\n");

            printf("and8 (virtualKey) (Hex): 0x%04X, (Binary): ", and8);
            printBinary(and8);
            printf("\n");

            return (state & 0x8) ? 1 : 0;
    } else if (mode == "T") {
        // Toggle state for keys like CapsLock, NumLock, ScrollLock
        return (GetKeyState(virtualKey) & 0x1) ? 1 : 0;
    }

    return 0; // Default case if mode is not recognized
}