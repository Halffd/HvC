#include <iostream>
#include <thread>
#include <memory>
#include "window/WindowManager.hpp"
#include "core/IO.hpp"
#include "core/ConfigManager.hpp"
#include "core/ScriptEngine.hpp"
#include <csignal>

// Forward declare test_main
int test_main(int argc, char* argv[]);

volatile std::sig_atomic_t gSignalStatus = 0;

void SignalHandler(int signal) {
    gSignalStatus = signal;
}

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