#include "logger.h"
#include <iostream>
#include <thread>
#include <memory>
#include "window/WindowManager.hpp"
#include "core/IO.hpp"
#include "core/ConfigManager.hpp"
#include "core/ScriptEngine.hpp"
#include <csignal>
#include "core/HotkeyManager.hpp"
#include "media/MPVController.hpp"
#include "core/SocketServer.hpp"
#include "core/AutoClicker.hpp"
#include "core/EmergencySystem.hpp"
#include "core/SequenceDetector.hpp"
#include "utils/Notifier.hpp"
#include "window/WindowRules.hpp"
#include "core/MouseGesture.hpp"
#include "core/MacroSystem.hpp"
#include "utils/Logger.h"

// Forward declare test_main
int test_main(int argc, char* argv[]);

volatile std::sig_atomic_t gSignalStatus = 0;

void SignalHandler(int signal) {
    lo.info("Received signal: " + std::to_string(signal));
    gSignalStatus = signal;
}

class AppServer : public H::SocketServer {
protected:
    void HandleCommand(const std::string& cmd) override {
        lo.debug("Socket command received: " + cmd);
        if (cmd == "toggle_mute") ToggleMute();
        else if (cmd.starts_with("volume:")) {
            int vol = std::stoi(cmd.substr(7));
            AdjustVolume(vol);
        }
        // Add more command handlers
    }

private:
    void ToggleMute() { /* ... */ }
    void AdjustVolume(int vol) { /* ... */ }
};

int main(int argc, char* argv[]) {
#ifdef RUN_TESTS
    return test_main(argc, argv);
#else
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    try {
        lo.info("HvC application starting up");
        
        // Ensure config directories exist
        H::ConfigPaths::EnsureConfigDir();
        
        // Ensure log directories exist
        LoggerPaths::EnsureLogDir();
        
        // Check for existing config.cfg in root and migrate if needed
        if (std::filesystem::exists("config.cfg") && !std::filesystem::exists("config/main.cfg")) {
            try {
                std::filesystem::copy_file("config.cfg", "config/main.cfg");
                lo.info("Migrated config.cfg to config/main.cfg");
            } catch (const std::filesystem::filesystem_error& e) {
                lo.error("Failed to migrate config.cfg: " + std::string(e.what()));
            }
        }
        
        // Load configurations
        auto& config = H::Configs::Get();
        config.Load();
        lo.info("Configuration loaded successfully");
        
        // Load and bind hotkeys
        auto& mappings = H::Mappings::Get();
        mappings.Load();
        lo.info("Key mappings loaded successfully");
        
        // Create main objects
        auto io = std::make_shared<H::IO>();
        mappings.BindHotkeys(*io);
        
        // Create a WindowManager instance
        auto windowManager = std::make_unique<H::WindowManager>();
        lo.info("Detected window manager: " + windowManager->GetCurrentWMName());
        
        if (!windowManager->IsWMSupported()) {
            lo.warning("Current window manager may not be fully supported");
        }
        
        // Load default configuration values
        int moveSpeed = config.Get<int>("Window.MoveSpeed", 10);
        std::string theme = config.Get<std::string>("UI.Theme", "dark");
        
        // Create MPV controller
        auto mpv = std::make_unique<H::MPVController>();
        
        // Create script engine
        auto scriptEngine = std::make_shared<H::ScriptEngine>(*io, *windowManager);
        
        // Initialize hotkey manager with script engine
        auto hotkeyManager = std::make_unique<H::HotkeyManager>(*io, *windowManager, *mpv, *scriptEngine);
        
        // Start listening for hotkeys
        io->HotkeyListen();
        
        // Watch for theme changes
        H::Configs::Get().Watch<std::string>("UI.Theme", [](auto oldVal, auto newVal) {
            lo.info("Theme changed from " + oldVal + " to " + newVal);
            H::Notifier::Show("Theme changed to " + newVal);
        });
        
        // Setup socket server for IPC
        AppServer server;
        server.Start();
        lo.info("Socket server started");
        
        // Main application loop
        lo.info("Entering main application loop, press Esc to exit");
        
        while (!gSignalStatus) {
            // Check for config changes every second
            static auto lastCheck = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            
            if(std::chrono::duration_cast<std::chrono::seconds>(now - lastCheck) > std::chrono::seconds(1)) {
                config.Reload();
                mappings.Reload();
                
                if(mappings.CheckRebind()) {
                    lo.info("Hotkey rebind needed, updating bindings");
                    io->ClearHotkeys();
                    mappings.BindHotkeys(*io);
                }
                
                // Update window manager with new config
                int newMoveSpeed = config.Get<int>("Window.MoveSpeed", 10);
                WindowManager::SetMoveSpeed(newMoveSpeed);
                
                lastCheck = now;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Cleanup resources
        lo.info("Application shutting down, cleaning up resources");
        server.Stop();
        H::DisplayManager::Close();
        
        lo.info("Shutdown complete");
        return 0;
    }
    catch (const std::exception& e) {
        lo.error("Fatal error: " + std::string(e.what()));
        H::Notifier::Error("Application crashed: " + std::string(e.what()));
        return 1;
    }
#endif
}