#!/bin/bash

# HvC Setup Script
# This script ensures the correct directory structure and migrates legacy files

echo "HvC Setup Script"
echo "================"

# Create necessary directories
echo "Setting up directory structure..."
mkdir -p config/hotkeys
mkdir -p log

# Migrate legacy config files if they exist
if [ -f "config.cfg" ] && [ ! -f "config/main.cfg" ]; then
    echo "Migrating config.cfg to config/main.cfg..."
    cp config.cfg config/main.cfg
    echo "✓ Config file migrated. Original file preserved at config.cfg"
fi

if [ -f "input.cfg" ] && [ ! -f "config/input.cfg" ]; then
    echo "Migrating input.cfg to config/input.cfg..."
    cp input.cfg config/input.cfg
    echo "✓ Input config file migrated. Original file preserved at input.cfg"
fi

# Check for legacy log files
if [ -f "HvC.log" ] && [ ! -f "log/HvC.log" ]; then
    echo "Moving HvC.log to log/HvC.log..."
    cp HvC.log log/HvC.log
    echo "✓ Log file moved. Original file preserved at HvC.log"
fi

if [ -f "errorlog" ] && [ ! -f "log/error.log" ]; then
    echo "Moving errorlog to log/error.log..."
    cp errorlog log/error.log
    echo "✓ Error log moved. Original file preserved at errorlog"
fi

# Set up git to ignore log files
if [ -f ".gitignore" ]; then
    if ! grep -q "log/\*.log" .gitignore; then
        echo "Updating .gitignore to ignore log files..."
        echo "# Log files" >> .gitignore
        echo "log/*.log" >> .gitignore
        echo "log/*.log.*" >> .gitignore
        echo "✓ .gitignore updated"
    fi
else
    echo "Creating .gitignore file..."
    echo "# Log files" > .gitignore
    echo "log/*.log" >> .gitignore
    echo "log/*.log.*" >> .gitignore
    echo "✓ .gitignore created"
fi

echo "Setup complete!"
echo ""
echo "Directory structure:"
echo "- config/      : Configuration files"
echo "  - main.cfg   : Main application configuration"
echo "  - input.cfg  : Input mapping configuration"
echo "  - hotkeys/   : Hotkey script directory"
echo "- log/         : Log files"
echo "  - HvC.log    : Main application log"
echo "  - error.log  : Error log"
echo ""
echo "Run 'chmod +x setup.sh' if you want to make this script executable" 