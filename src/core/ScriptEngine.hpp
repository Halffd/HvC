#pragma once
#include <sol/sol.hpp>
#include "ConfigManager.hpp"
#include <fstream>

namespace H {
class ScriptEngine {
public:
    ScriptEngine(IO& io, WindowManager& wm) : io(io), wm(wm) {
        lua.open_libraries(sol::lib::base, sol::lib::package);
        BindAPI();
    }

    void LoadScript(const std::string& path) {
        std::ifstream file(path);
        std::string line;
        
        while(std::getline(file, line)) {
            if(IsAHKLine(line)) {
                ParseAHKLine(line);
            } else {
                lua.script(line);
            }
        }
    }

private:
    sol::state lua;
    IO& io;
    WindowManager& wm;

    void BindAPI() {
        lua["config"] = Configs::Get();
        
        lua.set_function("Send", [this](const std::string& keys) {
            io.Send(keys);
        });

        lua.set_function("AddHotkey", [this](const std::string& combo, sol::object action) {
            std::string converted = ConvertAHKModifiers(combo);
            
            if (action.is<sol::function>()) {
                sol::function func = action.as<sol::function>();
                io.Hotkey(converted, [this, func]{
                    try {
                        func();
                    } catch(const sol::error& e) {
                        std::cerr << "Lua hotkey error: " << e.what() << "\n";
                    }
                });
            } else if (action.is<std::string>()) {
                std::string cmd = action.as<std::string>();
                io.Hotkey(converted, [this, cmd]{
                    try {
                        lua.script(cmd);
                    } catch(const sol::error& e) {
                        std::cerr << "Lua hotkey command error: " << e.what() << "\n";
                    }
                });
            }
        });

        lua.set_function("AddContextual", [this](const std::string& combo, 
                                               const std::string& context,
                                               sol::object action) {
            std::string converted = ConvertAHKModifiers(combo);
            
            if (action.is<sol::function>()) {
                sol::function func = action.as<sol::function>();
                io.Hotkey(converted, [this, context, func]{
                    if(CheckContext(context)) {
                        func();
                    }
                });
            } else if (action.is<std::string>()) {
                std::string cmd = action.as<std::string>();
                io.Hotkey(converted, [this, context, cmd]{
                    if(CheckContext(context)) {
                        lua.script(cmd);
                    }
                });
            }
        });
        
        lua.set_function("AddChord", [this](const std::string& modifier, 
                                          const std::string& key, 
                                          sol::object action) {
            std::string converted = ConvertAHKModifiers(modifier + " & " + key);
            
            if (action.is<sol::function>()) {
                sol::function func = action.as<sol::function>();
                io.Hotkey(converted, [this, func]{
                    try {
                        func();
                    } catch(const sol::error& e) {
                        std::cerr << "Lua chord error: " << e.what() << "\n";
                    }
                });
            } else if (action.is<std::string>()) {
                std::string cmd = action.as<std::string>();
                io.Hotkey(converted, [this, cmd]{
                    try {
                        lua.script(cmd);
                    } catch(const sol::error& e) {
                        std::cerr << "Lua chord command error: " << e.what() << "\n";
                    }
                });
            }
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

    bool IsAHKLine(const std::string& line) {
        return line.find("::") != std::string::npos || 
               line.find("#If") != std::string::npos;
    }

    void ParseAHKLine(const std::string& line) {
        // Implementation of ParseAHKLine method
    }

    std::string ConvertAHKModifiers(const std::string& combo) {
        std::string result = combo;
        std::map<char, std::string> modMap = {
            {'^', "ctrl+"}, {'!', "alt+"}, {'#', "win+"}, {'+', "shift+"}
        };
        
        // Handle special cases first
        size_t ampPos = result.find('&');
        if(ampPos != std::string::npos) {
            result.replace(ampPos, 1, " ");
        }
        
        for(const auto& [symbol, replacement] : modMap) {
            size_t pos = result.find(symbol);
            while(pos != std::string::npos) {
                result.replace(pos, 1, replacement);
                pos = result.find(symbol, pos + replacement.length());
            }
        }
        
        // Remove trailing '+' if any
        if(!result.empty() && result.back() == '+') {
            result.pop_back();
        }
        
        // Convert remaining uppercase letters to lowercase
        std::transform(result.begin(), result.end(), result.begin(),
                    [](unsigned char c){ return std::tolower(c); });
        
        return result;
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
}; 