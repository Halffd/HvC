#pragma once
#include <X11/Xlib.h>
#include "../common/types.hpp"
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <unordered_map>
#include <thread>
#include <iostream>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <atomic>
#include <cstring>
#include <set>
#include <sstream>
#include <linux/uinput.h>   // ✅ This gives you UI_SET_* and uinput_setup
#include <sys/ioctl.h>
#include <memory>

namespace havel {

enum class MouseButton {
    Left = BTN_LEFT,
    Right = BTN_RIGHT,
    Middle = BTN_MIDDLE,
    Side1 = BTN_SIDE,
    Side2 = BTN_EXTRA
};

enum class MouseAction {
    Hold = 1,
    Release = 0,
    Click = 2
};

    struct HotKey {
        std::string alias;
        Key key;
        int modifiers;
        std::function<void()> callback;
        std::string action;
        std::vector<std::function<bool()> > contexts;
        bool enabled = true;
        bool blockInput = false;
        bool suspend = false;
        bool exclusive = false;
        bool success = false;
        bool evdev = false;
        bool isKeyUp = false; // Tracks if this is a key release hotkey
    };

    struct ModifierState {
        bool leftCtrl = false;
        bool rightCtrl = false;
        bool leftShift = false;
        bool rightShift = false;
        bool leftAlt = false;
        bool rightAlt = false;
        bool leftMeta = false;
        bool rightMeta = false;
    };

    struct IoEvent {
        Key key;
        int modifiers;
        bool isDown;
    };

    class IO {
        std::thread evdevThread;
        std::atomic<bool> evdevRunning{false};
        std::string evdevDevicePath;

    public:
        static std::unordered_map<int, HotKey> hotkeys;
        bool suspendHotkeys = false;

        IO();

        ~IO();

        // Key sending methods
        void Send(Key key, bool down = true);

        void Send(cstr keys);
        void SendUInput(int keycode, bool down);
        void SendSpecific(const std::string &keys);

        void ControlSend(const std::string &control, const std::string &keys);

        void ProcessKeyCombination(const std::string &keys);

        void SendX11Key(const std::string &keyName, bool press);

        // Hotkey methods
        bool ContextActive(std::vector<std::function<bool()> > contexts);

        bool AddHotkey(const std::string &alias, Key key, int modifiers,
                       std::function<void()> callback);

        HotKey AddHotkey(const std::string &rawInput,
                         std::function<void()> action, int id) const;

        bool Hotkey(const std::string &hotkeyStr, std::function<void()> action,
                    int id = 0);

        bool Suspend(int id);

        bool Resume(int id);

        // Suspend or resume all hotkeys
        void suspendAllHotkeys(bool suspend) {
            suspendHotkeys = suspend;
            std::cout << "All hotkeys " << (suspend ? "suspended" : "resumed") << std::endl;
        }

        // Mouse methods
        void MouseMove(int x, int y);

        void MouseClick(int button);

        void MouseDown(int button);

        void MouseUp(int button);

        void MouseWheel(int amount);

        // State methods
        int GetState(const std::string &keyName, const std::string &mode = "");

        static void PressKey(const std::string &keyName, bool press);

        // Utility methods
        std::shared_ptr<std::atomic<bool>> SetTimer(int milliseconds, const std::function<void()> &func);

        void MsgBox(const std::string &message);

        int GetMouse();

        int GetKeyboard();

        int ParseModifiers(std::string str);

        void AssignHotkey(HotKey hotkey, int id);

        // Add new methods for dynamic hotkey grabbing/ungrabbing
        bool GrabHotkey(int hotkeyId);

        bool UngrabHotkey(int hotkeyId);

        bool GrabHotkeysByPrefix(const std::string &prefix);

        bool UngrabHotkeysByPrefix(const std::string &prefix);

        // Static methods
        static void removeSpecialCharacters(std::string &keyName);

        static void HandleKeyEvent(XEvent &event);

        static void HandleMouseEvent(XEvent &event);

        static Key StringToButton(const std::string &buttonNameRaw);

        static Key handleKeyString(const std::string &keystr);

        static Key StringToVirtualKey(std::string keyName);

        // Call this to start listening on your keyboard device
        bool StartEvdevHotkeyListener(const std::string &devicePath);

        // Call this to stop the thread cleanly
        void StopEvdevHotkeyListener();

        template<typename T, typename S>
        bool Click(T button, S action) {
            int btnCode;

            if constexpr (std::is_same_v<T, int>) {
                btnCode = button;
            } else if constexpr (std::is_same_v<T, std::string>) {
                if (button == "left") btnCode = BTN_LEFT;
                else if (button == "right") btnCode = BTN_RIGHT;
                else if (button == "middle") btnCode = BTN_MIDDLE;
                else if (button == "side1") btnCode = BTN_SIDE;
                else if (button == "side2") btnCode = BTN_EXTRA;
                else {
                    std::cerr << "Unknown button string: " << button << "\n";
                    return false;
                }
            } else if constexpr (std::is_enum_v<T>) {
                btnCode = static_cast<int>(button);
            } else {
                static_assert(always_false<T>, "Unsupported type for button");
            }

            if constexpr (std::is_same_v<S, int>) {
                return EmitClick(btnCode, S(action));
            } else if constexpr (std::is_enum_v<S>) {
                return EmitClick(btnCode, static_cast<int>(action));
            } else {
                static_assert(always_false<S>, "Unsupported type for action");
            }
        }

        bool MouseClick(int btnCode, int dx, int dy, int speed, float accel);
        bool MouseMove(int dx, int dy, int speed, float accel);
        bool Scroll(int dy, int dx = 0);
    private:
        template<typename T>
        static constexpr bool always_false = false;
        bool EmitClick(int btnCode, int action);

        bool InitUinputDevice();

        void EmitToUinput(int code, bool down);

        void CleanupUinputDevice();
        bool SetupUinputDevice();
        // X11 hotkey monitoring
        void MonitorHotkeys();

        static Key EvdevNameToKeyCode(std::string keyName);
        bool MatchModifiers(uint hotkeyMods, const std::map<int, bool> &keyState);
        // Platform specific implementations
        Display *display;
        std::map<std::string, Key> keyMap;
        std::map<int, bool> evdevKeyState;
        std::map<std::string, HotKey> instanceHotkeys;
        // Renamed to avoid conflict
        std::map<std::string, bool> hotkeyStates;
        std::thread timerThread;
        bool timerRunning = false;
        int uinputFd = -1;
        std::set<int> blockedKeys;

        // Static members
        static bool hotkeyEnabled;
        static int hotkeyCount;
        std::mutex hotkeyMutex;
        std::mutex blockedKeysMutex;
        std::map<int, bool> keyDownState;

        // Key mapping and sending utilities
        void InitKeyMap();

        void SendKeyEvent(Key key, bool down);

        std::vector<IoEvent> ParseKeysString(const std::string &keys);

        // Helper methods for X11 key grabbing
        void Grab(Key input, unsigned int modifiers, Window root,
                  bool exclusive, bool isMouse = false);

        void Ungrab(Key input, unsigned int modifiers, Window root);
    };
} // namespace havel
