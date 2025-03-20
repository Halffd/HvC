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
#include "core/Notifier.hpp"
#include "window/WindowRules.hpp"
#include "core/MouseGesture.hpp"
#include "core/MacroSystem.hpp"

// Forward declare test_main
int test_main(int argc, char* argv[]);

volatile std::sig_atomic_t gSignalStatus = 0;

void SignalHandler(int signal) {
    gSignalStatus = signal;
}

class AppServer : public H::SocketServer {
protected:
    void HandleCommand(const std::string& cmd) override {
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
        // Load configurations
        auto& config = H::Configs::Get();
        config.Load();
        
        // Load and bind hotkeys
        auto& mappings = H::Mappings::Get();
        mappings.Load();
        
        auto io = std::make_shared<H::IO>();
        mappings.BindHotkeys(*io);
        
        // Access config values
        int moveSpeed = config.Get<int>("Window.MoveSpeed", 10);
        std::string theme = config.Get<std::string>("UI.Theme", "dark");
        
        // Save modified configs
        config.Set("UI.FontSize", 14);
        config.Save();
        
        mappings.Add("^#Left", "WindowManager::ManageVirtualDesktops(1)");
        mappings.Save();
        
        // Create a WindowManager instance
        auto windowManager = std::make_unique<H::WindowManager>();
        std::cout << "Detected window manager: " << windowManager->GetCurrentWMName() << std::endl;
        
        if (!windowManager->IsWMSupported()) {
            std::cerr << "Warning: Current window manager may not be fully supported\n";
        }
        
        // Start listening for hotkeys
        io->HotkeyListen();
        
        // Keep the main thread alive
        std::cout << "Press Esc to exit" << std::endl;
        
        H::ScriptEngine scriptEngine(*io, *windowManager);
        scriptEngine.LoadScript("config.hvl");  // Hv2 Lua config

        // Watch for theme changes
        H::Configs::Get().Watch<std::string>("UI.Theme", [](auto oldVal, auto newVal) {
            std::cout << "Theme changed from " << oldVal << " to " << newVal << std::endl;
            WindowManager::SetTheme(newVal);
        });

        // Dynamic key remapping example
        H::Mappings::Get().Add("<^>!Tab", R"(
            local wm = require("window_manager")
            wm.AltTabMenu()
            coroutine.yield()  // Allow UI update
            if config.Get("UI.AltTabStyle") == "mac" then
                wm.CycleWindowsReverse()
            else
                wm.CycleWindows()
            end
        )");
        
        H::MPVController mpv;
        AppServer server;
        
        // Set up hotkey system
        H::HotkeyManager hotkeys(*io, *windowManager, mpv);
        
        // Start socket server
        server.Start();
        
        H::AutoClicker autoClicker(*io);
        H::EmergencySystem::RegisterResetHandler(*io);

        // Add sequence detection
        H::SequenceDetector helpSequence({"h", "e", "l", "p"}, []{
            Notifier::Show("Help", "Available commands...");
        });

        io->OnKeyPress([&](const std::string& key){
            EmergencySystem::TrackKeyPress(key);
            helpSequence.ProcessEvent(key);
        });

        // Add autoclicker toggle
        mappings.Add("enter", [&autoClicker]{
            autoClicker.Toggle();
        });
        
        H::WindowRules windowRules;
        windowRules.AddRule({
            std::regex("Chrom.*"), 
            std::regex(""), 
            [](WindowManager& wm){
                wm.SetOpacity(wm.GetActiveWindow(), 0.9f);
                wm.MoveWindowToPosition(wm.GetActiveWindow(), 100, 100);
            }
        });

        H::MouseGesture gestures(*io);
        gestures.DefineGesture("CloseTab", {{0,0}, {100,0}}, []{
            io->Send("^{w}");
        });

        H::MacroSystem macros(*io);
        mappings.Add("^#r", [&macros]{ macros.StartRecording(); });
        mappings.Add("^#s", [&macros]{ macros.StopAndSave("macro1"); });
        mappings.Add("^#p", [&macros]{ macros.Play("macro1"); });
        
        while (!gSignalStatus) {
            // Check for config changes every second
            static auto lastCheck = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            
            if(std::chrono::duration_cast<std::chrono::seconds>(now - lastCheck) > 1s) {
                config.Reload();
                mappings.Reload();
                
                if(mappings.CheckRebind()) {
                    io->ClearHotkeys();
                    mappings.BindHotkeys(*io);
                }
                
                // Update window manager with new config
                int newMoveSpeed = config.Get<int>("Window.MoveSpeed", 10);
                WindowManager::SetMoveSpeed(newMoveSpeed);
                
                lastCheck = now;
            }
            
            std::this_thread::sleep_for(100ms);
        }
        
        // Cleanup resources
        H::DisplayManager::Close();
        
        std::cout << "Shutting down gracefully...\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
#endif
}