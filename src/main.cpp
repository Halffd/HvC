#include <iostream>
#include <thread>
#include <memory>
#include "window/WindowManager.hpp"
#include "core/IO.hpp"

// Forward declare test_main
int test_main(int argc, char* argv[]);

int main(int argc, char* argv[]) {
#ifdef RUN_TESTS
    return test_main(argc, argv);
#else
    try {
        // Create a WindowManager instance
        auto windowManager = std::make_unique<H::WindowManager>();
        std::cout << "Detected window manager: " << windowManager->GetCurrentWMName() << std::endl;
        
        if (!windowManager->IsWMSupported()) {
            std::cerr << "Warning: Current window manager may not be fully supported\n";
        }
        
        // Create an IO instance
        auto io = std::make_shared<H::IO>();
        
        // Register some hotkeys
        io->Hotkey("f9", [&io]() {
            io->Suspend();
        });
        
        io->Hotkey("!Esc", [&io]() {
            io->Suspend();
            exit(0);
        });
        
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