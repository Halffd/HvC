#pragma once
#include <unordered_map>
#include <string>
#include <functional>
#include <fstream>
#include <sstream>
#include "types.hpp"

namespace H {

class Mappings {
public:
    static Mappings& Get() {
        static Mappings instance;
        return instance;
    }

    void Load(const std::string& filename = "input.cfg") {
        std::ifstream file(filename);
        std::string line;
        while (std::getline(file, line)) {
            size_t delim = line.find('=');
            if (delim != std::string::npos) {
                std::string key = line.substr(0, delim);
                std::string value = line.substr(delim+1);
                hotkeys[key] = value;
            }
        }
    }

    void Save(const std::string& filename = "input.cfg") {
        std::ofstream file(filename);
        for (const auto& [key, value] : hotkeys) {
            file << key << "=" << value << "\n";
        }
    }

    void BindHotkeys(class IO& io) {
        for (const auto& [keyCombo, command] : hotkeys) {
            io.Hotkey(keyCombo, [this, command]() {
                ExecuteCommand(command);
            });
        }
    }

    void Add(const std::string& keyCombo, const std::string& command) {
        hotkeys[keyCombo] = command;
    }

    void Remove(const std::string& keyCombo) {
        hotkeys.erase(keyCombo);
    }

    std::string GetCommand(const std::string& keyCombo) const {
        auto it = hotkeys.find(keyCombo);
        return it != hotkeys.end() ? it->second : "";
    }

private:
    std::unordered_map<std::string, std::string> hotkeys;

    void ExecuteCommand(const std::string& command) {
        // Implement command parsing and execution
        if (command.starts_with("WindowManager::")) {
            HandleWindowCommand(command);
        }
        else if (command.starts_with("IO::")) {
            HandleIOCommand(command);
        }
    }

    void HandleWindowCommand(const std::string& command) {
        size_t pos = command.find('(');
        std::string func = command.substr(15, pos-15);
        std::string args = command.substr(pos+1, command.find(')')-pos-1);

        if (func == "MoveWindow") {
            int dir = std::stoi(args);
            WindowManager::MoveWindow(dir);
        }
        else if (func == "ResizeWindow") {
            int dir = std::stoi(args);
            WindowManager::ResizeWindow(dir);
        }
        // Add other window commands...
    }

    void HandleIOCommand(const std::string& command) {
        // Similar implementation for IO commands
    }
};

class Configs {
public:
    static Configs& Get() {
        static Configs instance;
        return instance;
    }

    void Load(const std::string& filename = "config.cfg") {
        std::ifstream file(filename);
        std::string line, currentSection;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == ';') continue;
            
            if (line[0] == '[') {
                currentSection = line.substr(1, line.find(']')-1);
            }
            else {
                size_t delim = line.find('=');
                if (delim != std::string::npos) {
                    std::string key = currentSection + "." + line.substr(0, delim);
                    std::string value = line.substr(delim+1);
                    settings[key] = value;
                }
            }
        }
    }

    void Save(const std::string& filename = "config.cfg") {
        std::ofstream file(filename);
        std::string currentSection;
        
        for (const auto& [key, value] : settings) {
            size_t dotPos = key.find('.');
            std::string section = key.substr(0, dotPos);
            std::string name = key.substr(dotPos+1);

            if (section != currentSection) {
                file << "[" << section << "]\n";
                currentSection = section;
            }
            
            file << name << "=" << value << "\n";
        }
    }

    template<typename T>
    T Get(const std::string& key, T defaultValue = T()) const {
        auto it = settings.find(key);
        if (it != settings.end()) {
            std::istringstream iss(it->second);
            T result;
            if (iss >> result) return result;
        }
        return defaultValue;
    }

    template<>
    std::string Get<std::string>(const std::string& key, std::string defaultValue) const {
        auto it = settings.find(key);
        return it != settings.end() ? it->second : defaultValue;
    }

    template<typename T>
    void Set(const std::string& key, T value) {
        settings[key] = std::to_string(value);
    }

    template<>
    void Set<std::string>(const std::string& key, std::string value) {
        settings[key] = value;
    }

private:
    std::unordered_map<std::string, std::string> settings;
};

} // namespace H 