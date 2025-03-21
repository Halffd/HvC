#pragma once
#include "IO.hpp"
#include "WindowManager.hpp"
#include "MPVController.hpp"
#include "ScriptEngine.hpp"
#include <functional>
#include <filesystem>

namespace H {
class HotkeyManager {
public:
    HotkeyManager(IO& io, WindowManager& wm, MPVController& mpv, ScriptEngine& script) 
        : io(io), wm(wm), mpv(mpv), scriptEngine(script) 
    {
        LoadHotkeyConfigurations();
    }

    void LoadHotkeyConfigurations() {
        namespace fs = std::filesystem;
        
        // Load primary configuration file
        scriptEngine.LoadScript("config/input.hvl");
        
        // Load user configuration if exists
        if (fs::exists("input.hvl")) {
            scriptEngine.LoadScript("input.hvl");
            
            // Migrate user config to correct location if needed
            try {
                if (!fs::exists("config/user.hvl")) {
                    fs::copy_file("input.hvl", "config/user.hvl");
                    lo.info("Migrated user hotkey configuration to config/user.hvl");
                }
            } catch (const fs::filesystem_error& e) {
                lo.error("Failed to migrate user config: " + std::string(e.what()));
            }
        } else if (fs::exists("config/user.hvl")) {
            scriptEngine.LoadScript("config/user.hvl");
        }
        
        // Load any additional configuration files
        if (fs::exists("config/hotkeys")) {
            for (const auto& entry : fs::directory_iterator("config/hotkeys")) {
                if (entry.path().extension() == ".hvl") {
                    scriptEngine.LoadScript(entry.path().string());
                    lo.debug("Loaded hotkey module: " + entry.path().filename().string());
                }
            }
        }
        
        lo.info("Hotkey configurations loaded successfully");
    }

    void ReloadConfigurations() {
        lo.info("Reloading hotkey configurations...");
        LoadHotkeyConfigurations();
    }

private:
    IO& io;
    WindowManager& wm;
    MPVController& mpv;
    ScriptEngine& scriptEngine;
};
} 