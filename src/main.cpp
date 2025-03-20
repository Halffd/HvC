#include <iostream>
#include <thread>
#include <memory>
#include "window/WindowManager.hpp"
#include "core/IO.hpp"
#include "core/ConfigManager.hpp"

// Forward declare test_main
int test_main(int argc, char* argv[]);

int main(int argc, char* argv[]) {
#ifdef RUN_TESTS
    return test_main(argc, argv);
#else
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
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
#endif
}