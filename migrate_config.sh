#!/bin/bash

# Configuration Migration Script for HvC
# Migrates configuration files to the new structure

echo "HvC Configuration Migration"
echo "=========================="

# Create directories if they don't exist
mkdir -p config/hotkeys
mkdir -p log

# List of known configuration files to migrate
declare -a CONFIG_FILES=("config.cfg:main.cfg" "input.cfg:input.cfg" "input.hvl:input.hvl" "user.hvl:user.hvl")

# Migrate config files
for file_pair in "${CONFIG_FILES[@]}"; do
    source_file="${file_pair%%:*}"
    dest_file="config/${file_pair##*:}"
    
    if [ -f "$source_file" ]; then
        echo "Migrating $source_file to $dest_file..."
        cp "$source_file" "$dest_file"
        
        # Create backup with timestamp
        timestamp=$(date +"%Y%m%d%H%M%S")
        backup_file="${source_file}.bak.${timestamp}"
        cp "$source_file" "$backup_file"
        
        echo "✓ Migrated $source_file (backup created at $backup_file)"
    else
        echo "× File $source_file not found, skipping"
    fi
done

# Migrate hotkey scripts
if [ -d "hotkeys" ]; then
    echo "Migrating hotkey scripts from hotkeys/ to config/hotkeys/..."
    cp -r hotkeys/* config/hotkeys/
    echo "✓ Hotkey scripts migrated"
else
    echo "× Hotkeys directory not found, skipping"
fi

# Migrate log files
if [ -f "HvC.log" ]; then
    echo "Moving HvC.log to log/HvC.log..."
    cp "HvC.log" "log/HvC.log"
    echo "✓ Log file migrated (original preserved)"
fi

if [ -f "errorlog" ]; then
    echo "Moving errorlog to log/error.log..."
    cp "errorlog" "log/error.log"
    echo "✓ Error log migrated (original preserved)"
fi

echo ""
echo "Migration complete!"
echo ""
echo "You may now delete the original files if everything is working correctly:"
echo ""
echo "rm config.cfg input.cfg input.hvl user.hvl HvC.log errorlog"
echo ""
echo "Or keep them as backups." 