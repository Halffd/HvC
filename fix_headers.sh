#!/bin/bash

# Script to fix common header issues in the codebase

echo "HvC Header Fix Script"
echo "===================="

# Create include directory if it doesn't exist
mkdir -p include

# Create a header file with common X11 includes
cat > include/x11_includes.h << 'EOF'
#pragma once

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xrandr.h>
#include <X11/XF86keysym.h>
#endif

// Common typedefs to avoid redefinition errors
#ifndef Key_typedef
#define Key_typedef
typedef unsigned long Key;
#endif
EOF

# Create a header file with common system includes
cat > include/system_includes.h << 'EOF'
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <fstream>
#include <sstream>
EOF

# Create a logger.h symlink in include for easier access
ln -sf "../src/utils/Logger.h" include/logger.h

echo "✓ Created common header files in include directory"

# Function to add missing includes to source files
add_missing_includes() {
    local file=$1
    
    # Check file extension
    if [[ $file == *.cpp ]]; then
        # For C++ files, check if we need to add X11 includes
        if grep -q "XK_" "$file" || grep -q "XTest" "$file" || grep -q "XKB" "$file"; then
            if ! grep -q "#include \"x11_includes.h\"" "$file"; then
                echo "Adding X11 includes to $file"
                sed -i '1i#include "x11_includes.h"' "$file"
            fi
        fi
        
        # Check if logger is used but not included
        if grep -q "lo\." "$file" || grep -q "lo <<" "$file"; then
            if ! grep -q "#include \"logger.h\"" "$file"; then
                echo "Adding logger.h to $file"
                sed -i '1i#include "logger.h"' "$file"
            fi
        fi
    fi
}

echo "Scanning source files for missing includes..."

# Find all source files and check them
find src -name "*.cpp" -o -name "*.c" | while read -r file; do
    add_missing_includes "$file"
done

echo "✓ Fixed missing includes in source files"

echo ""
echo "Header fix complete! The include directory now contains common header files."
echo "Please rebuild your project with 'make rebuild' or 'cmake ..'."
echo ""
echo "You can run 'chmod +x fix_headers.sh' to make this script executable." 