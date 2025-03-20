#pragma once
#include "WindowManager.hpp"
#include <regex>

namespace H {
class WindowRules {
public:
    struct Rule {
        std::regex classPattern;
        std::regex titlePattern;
        std::function<void(WindowManager&)> action;
    };

    void AddRule(Rule rule) {
        rules.push_back(rule);
    }

    void ApplyRules(WindowManager& wm) {
        auto [cls, title] = wm.GetWindowProperties(wm.GetActiveWindow());
        
        for(const auto& rule : rules) {
            if(std::regex_search(cls, rule.classPattern) &&
               std::regex_search(title, rule.titlePattern)) {
                rule.action(wm);
            }
        }
    }

private:
    std::vector<Rule> rules;
};
} 