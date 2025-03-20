#pragma once
#include <sol/sol.hpp>
#include "ConfigManager.hpp"

namespace H {
class ScriptEngine {
public:
    ScriptEngine(IO& io, WindowManager& wm) : io(io), wm(wm) {
        lua.open_libraries(sol::lib::base, sol::lib::package);
        BindAPI();
    }

    void LoadScript(const std::string& path) {
        try {
            lua.script_file(path);
        } catch(const sol::error& e) {
            std::cerr << "Script error: " << e.what() << std::endl;
        }
    }

private:
    sol::state lua;
    IO& io;
    WindowManager& wm;

    void BindAPI() {
        lua["config"] = Configs::Get();
        lua["hotkeys"] = Mappings::Get();
        
        lua.set_function("Send", [this](const std::string& keys) {
            io.Send(keys);
        });

        lua.set_function("Run", [this](const std::string& cmd) {
            return wm.Run(cmd, ProcessMethod::ForkProcess);
        });

        lua.new_enum("Direction",
            "Up", 1,
            "Down", 2,
            "Left", 3,
            "Right", 4
        );

        lua.set_function("MoveWindow", [this](int dir, int dist) {
            wm.MoveWindow(dir, dist);
        });

        // Add more API bindings...
    }
};
}; 