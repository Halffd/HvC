#pragma once
#include "IO.hpp"
#include "WindowManager.hpp"
#include "MPVController.hpp"
#include <functional>
#include <unordered_map>

namespace H {
class HotkeyManager {
public:
    HotkeyManager(IO& io, WindowManager& wm, MPVController& mpv) 
        : io(io), wm(wm), mpv(mpv) 
    {
        InitDefaultBindings();
    }

    void InitDefaultBindings() {
        // Mode toggles
        AddHotkey("ctrl+shift+g", [this]{ ToggleMode("gaming"); });
        AddHotkey("ctrl+shift+d", [this]{ ToggleMode("default"); });

        // Media controls
        AddHotkey("audio_play", [this]{ mpv.SendCommand({"cycle", "pause"}); });
        AddHotkey("audio_next", [this]{ mpv.SendCommand({"playlist-next"}); });
        AddHotkey("audio_prev", [this]{ mpv.SendCommand({"playlist-prev"}); });

        // Window management
        AddHotkey("alt+right", [this]{ wm.MoveWindowToNextMonitor(); });
        AddHotkey("alt+gr", [this]{ wm.ToggleAlwaysOnTop(); });

        // Volume controls
        AddHotkey("volume_up", [this]{ AdjustVolume(5); });
        AddHotkey("volume_down", [this]{ AdjustVolume(-5); });
        AddHotkey("volume_mute", [this]{ ToggleMute(); });

        // MPV-specific controls
        AddModeBindings("gaming", {
            {"z", [this]{ mpv.SendCommand({"cycle", "pause"}); }},
            {"o", [this]{ mpv.Seek(-3); }},
            {"p", [this]{ mpv.Seek(3); }},
            {"l", [this]{ mpv.ToggleSubtitleVisibility(); }}
        });
    }

    void AddHotkey(const std::string& combo, std::function<void()> action) {
        io.Hotkey(combo, [action]{ 
            try {
                action(); 
            } catch(const std::exception& e) {
                std::cerr << "Hotkey error: " << e.what() << "\n";
            }
        });
    }

    template<typename... Args>
    void AddModeBindings(const std::string& mode, Args... bindings) {
        (AddHotkey(mode + ":" + bindings.first, bindings.second), ...);
    }

    void AddContextualBinding(const std::string& context, 
                             const std::string& combo,
                             std::function<void()> action) {
        io.Hotkey(combo, [this, context, action]{
            if(CheckContext(context)) {
                action();
            }
        });
    }

private:
    IO& io;
    WindowManager& wm;
    MPVController& mpv;
    std::string currentMode = "default";

    void ToggleMode(const std::string& mode) {
        currentMode = (currentMode == mode) ? "default" : mode;
        Notifier::Show("Mode Changed", "Active mode: " + currentMode);
    }

    void AdjustVolume(int delta) {
        // Implementation using system calls or pulseaudio API
        std::system(("pactl set-sink-volume @DEFAULT_SINK@ " + 
                    std::to_string(delta) + "%").c_str());
    }

    void ToggleMute() {
        std::system("pactl set-sink-mute @DEFAULT_SINK@ toggle");
    }

    bool CheckContext(const std::string& context) {
        if(context.empty()) return true;
        
        if(context.starts_with("Window.Active(")) {
            size_t start = context.find('"') + 1;
            size_t end = context.find('"', start);
            std::string pattern = context.substr(start, end - start);
            return wm.IsWindowActive(pattern);
        }
        
        if(context.starts_with("Config.")) {
            std::string key = context.substr(6);
            return config.Get<bool>(key, false);
        }
        
        return false;
    }
};
} 