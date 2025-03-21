#include "ScriptEngine.hpp"
#include <regex>

namespace H {

void ScriptEngine::ParseAHKLine(const std::string& line) {
    // Match basic hotkey patterns
    std::regex ahkPattern(R"((.*?)::\s*(.*))");
    std::smatch match;
    
    if(std::regex_match(line, match, ahkPattern)) {
        std::string hotkey = match[1];
        std::string action = match[2];
        
        // Convert AHK modifiers
        std::map<char, std::string> modMap = {
            {'^', "ctrl+"}, {'!', "alt+"}, {'#', "win+"}, {'+', "shift+"}
        };
        
        for(auto& [ahkChar, modStr] : modMap) {
            size_t pos = hotkey.find(ahkChar);
            if(pos != std::string::npos) {
                hotkey.replace(pos, 1, modStr);
            }
        }
        
        // Convert action
        if(action.find("Run ") == 0) {
            action = "Run('" + action.substr(4) + "')";
        }
        else if(action.find("Send ") == 0) {
            action = "io.Send('" + action.substr(5) + "')";
        }
        
        lua.script(fmt::format("hotkeys.Add('{}', function() {} end)", hotkey, action));
    }
}

} // namespace H 