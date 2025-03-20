#pragma once
#include <unordered_map>
#include <string>
#include <functional>
#include <fstream>
#include <sstream>
#include "types.hpp"
#include <filesystem>

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

    void BindHotkeys(IO& io) {
        try {
            for (const auto& [keyCombo, command] : hotkeys) {
                try {
                    io.Hotkey(keyCombo, [this, command]() {
                        SafeExecute(command);
                    });
                } catch(const std::exception& e) {
                    std::cerr << "Failed to bind hotkey '" << keyCombo 
                              << "': " << e.what() << "\n";
                }
            }
        } catch(...) {
            std::cerr << "Critical error in hotkey binding system\n";
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

    void Reload() {
        auto oldHotkeys = hotkeys;
        Load();
        if(hotkeys != oldHotkeys) {
            needsRebind = true;
        }
    }

    bool CheckRebind() {
        if(needsRebind) {
            needsRebind = false;
            return true;
        }
        return false;
    }

private:
    std::unordered_map<std::string, std::string> hotkeys;
    bool needsRebind = false;

    void ExecuteCommand(const std::string& command) {
        try {
            if(command.empty()) return;
            
            // Split command into parts
            std::vector<std::string> parts;
            std::istringstream iss(command);
            std::string part;
            while(std::getline(iss, part, ' ')) {
                if(!part.empty()) parts.push_back(part);
            }
            
            if(parts[0] == "Run") {
                if(parts.size() >= 2) {
                    std::string args = parts.size() > 2 ? 
                        command.substr(command.find(parts[2])) : "";
                    WindowManager::Run(parts[1], ProcessMethod::ForkProcess, 
                                     "normal", args, 0);
                }
            }
            else if(parts[0] == "Send") {
                if(parts.size() >= 2) {
                    io.Send(command.substr(command.find(parts[1])));
                }
            }
            else if(parts[0] == "If") {
                // Simple condition support
                if(parts.size() >= 4 && parts[2] == "==") {
                    std::string var = config.Get<std::string>(parts[1], "");
                    if(var == parts[3]) {
                        ExecuteCommand(command.substr(command.find(parts[4])));
                    }
                }
            }
            // Add other command types...
        }
        catch(const std::exception& e) {
            std::cerr << "Command error: " << e.what() << std::endl;
        }
    }

    void SafeExecute(const std::string& command) noexcept {
        try {
            ExecuteCommand(command);
        } catch(const std::exception& e) {
            std::cerr << "Command execution failed: " << e.what() 
                      << " (Command: " << command << ")\n";
        } catch(...) {
            std::cerr << "Unknown error executing command: " << command << "\n";
        }
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

    template<typename T>
    void Watch(const std::string& key, std::function<void(T,T)> callback) {
        watchers[key].push_back([=](const std::string& oldVal, const std::string& newVal) {
            T oldT = Convert<T>(oldVal);
            T newT = Convert<T>(newVal);
            callback(oldT, newT);
        });
    }

    void Reload() {
        auto oldSettings = settings;
        Load();
        
        for(const auto& [key, newVal] : settings) {
            if(oldSettings[key] != newVal) {
                for(auto& watcher : watchers[key]) {
                    watcher(oldSettings[key], newVal);
                }
            }
        }
    }

    void Validate() const {
        const std::set<std::string> validKeys = {
            "Window.MoveSpeed", "Window.ResizeSpeed", 
            "Hotkeys.GlobalSuspend", "UI.Theme"
        };

        for(const auto& [key, val] : settings) {
            if(validKeys.find(key) == validKeys.end()) {
                std::cerr << "Warning: Unknown config key '" << key << "'\n";
            }
        }
    }

    template<typename T>
    T Get(const std::string& key, T defaultValue, T min, T max) const {
        T value = Get(key, defaultValue);
        if(value < min || value > max) {
            std::cerr << "Config value out of range: " << key 
                      << "=" << value << " (Valid: " 
                      << min << "-" << max << ")\n";
            return defaultValue;
        }
        return value;
    }

private:
    std::unordered_map<std::string, std::string> settings;
    std::unordered_map<std::string, std::vector<std::function<void(std::string, std::string)>>> watchers;

    template<typename T>
    static T Convert(const std::string& val) {
        std::istringstream iss(val);
        T result;
        iss >> result;
        return result;
    }
};

void BackupConfig(const std::string& path = "config.cfg") {
    namespace fs = std::filesystem;
    try {
        fs::path configPath(path);
        if(fs::exists(configPath)) {
            fs::path backup = configPath;
            backup += ".bak";
            fs::copy_file(configPath, backup, fs::copy_options::overwrite_existing);
        }
    } catch(const fs::filesystem_error& e) {
        std::cerr << "Config backup failed: " << e.what() << "\n";
    }
}

void RestoreConfig(const std::string& path = "config.cfg") {
    namespace fs = std::filesystem;
    try {
        fs::path backup(path + ".bak");
        if(fs::exists(backup)) {
            fs::copy_file(backup, path, fs::copy_options::overwrite_existing);
        }
    } catch(const fs::filesystem_error& e) {
        std::cerr << "Config restore failed: " << e.what() << "\n";
    }
}

} // namespace H 