#pragma once
#include "IO.hpp"
#include <vector>
#include <cmath>

namespace H {
class MouseGesture {
public:
    using Point = std::pair<int, int>;
    
    MouseGesture(IO& io) : io(io) {
        StartTracking();
    }

    void DefineGesture(const std::string& name, 
                      const std::vector<Point>& pattern,
                      std::function<void()> action) {
        gestures[name] = {pattern, action};
    }

private:
    IO& io;
    std::vector<Point> currentPath;
    std::unordered_map<std::string, 
                      std::pair<std::vector<Point>, std::function<void()>>> gestures;

    void StartTracking() {
        io.OnMouseMove([this](int x, int y){
            if(io.IsMousePressed(3)) { // Right button held
                currentPath.emplace_back(x, y);
            }
        });

        io.OnMouseUp([this](int button){
            if(button == 3 && !currentPath.empty()) {
                CheckGestures();
                currentPath.clear();
            }
        });
    }

    void CheckGestures() {
        auto normalized = NormalizePath(currentPath);
        
        for(const auto& [name, gesture] : gestures) {
            if(ComparePaths(normalized, gesture.first)) {
                gesture.second();
                break;
            }
        }
    }

    std::vector<Point> NormalizePath(const std::vector<Point>& path) {
        // Implementation for path normalization
    }

    bool ComparePaths(const std::vector<Point>& path1, 
                     const std::vector<Point>& path2) {
        // Implementation for path comparison
    }
};
} 