# HvC - Hotkey and Window Controller

HvC is a powerful utility for managing windows and hotkeys across multiple platforms, with a focus on Linux X11 environments. It allows for complex window management, hotkey configurations, and automated tasks via Lua scripting.

## Features

- Global hotkey registration and management
- Window tracking and manipulation
- Lua scripting for complex automation
- Configurable via text files
- Cross-platform support (primarily Linux X11)

## Directory Structure

- `/src` - Source code
- `/include` - Header files
- `/config` - Configuration files
- `/log` - Log files
- `/build` - Build directory

## Building HvC

### Prerequisites

- C++17 compatible compiler (GCC 9+ or Clang 10+)
- CMake 3.10+
- X11 development libraries (on Linux)
- Lua 5.3+ development libraries

### Dependencies

- X11 libraries: Xlib, Xutil, XTest, XKB, XRandR
- Sol2 Lua binding library
- pthread

### Quick Build

We provide a helper script to resolve common build issues:

```bash
# Fix common build issues
./fix_build.sh

# Build the project
mkdir -p build && cd build && cmake .. && cmake --build .
```

Alternatively, you can use the Makefile:

```bash
# Fix build issues and build
make rebuild
```

### Manual Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Configuration

HvC uses several configuration files located in the `config` directory:

- `main.cfg` - Main configuration file
- `hotkeys/*.lua` - Lua scripts for hotkey configurations

See the [configuration guide](config/README.md) for more details.

## Troubleshooting

If you encounter build issues, please refer to the [BUILD_ISSUES.md](BUILD_ISSUES.md) file for common problems and solutions.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. 