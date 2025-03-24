cmake_minimum_required(VERSION 3.10)
project(hvc VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create a boolean option for assembly generation
option(GENERATE_ASM "Generate assembly files" OFF)

# Add option to enable/disable tests
option(ENABLE_TESTS "Enable testing" OFF)

# Detect platform
if(WIN32)
    add_definitions(-DWINDOWS)
elseif(UNIX AND NOT APPLE)
    add_definitions(-D__linux__)
    # Find X11
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Exclude GUI files for now
file(GLOB_RECURSE source_files 
    "src/*.cpp"
    "src/core/*.cpp"
    "src/window/*.cpp"
    "src/utils/*.cpp"
)
file(GLOB_RECURSE source_c_files "src/*.c")

# Exclude GUI files
list(FILTER source_files EXCLUDE REGEX "src/gui/.*\\.cpp$")

set(src ${source_files} ${source_c_files})

# Create executable (only once)
add_executable(hvc ${src})

# Include directories
include_directories("${CMAKE_SOURCE_DIR}/include")

# Set output properties 
set_target_properties(hvc PROPERTIES OUTPUT_NAME HvC)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

# Set the output directory for the assembly files
set(CMAKE_ASM_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/asm)

# Ensure the output directory exists
file(MAKE_DIRECTORY ${CMAKE_ASM_OUTPUT_DIRECTORY})

target_compile_options(hvc PRIVATE
        -Wall        # Enable all warnings
        -Wextra     # Enable extra warnings
        -Wpedantic   # Enable strict ISO C/C++ compliance
        # -Werror     # Treat warnings as errors
)

# Loop through source files to generate assembly if the option is enabled
if(GENERATE_ASM)
    foreach(source_file IN LISTS src)
        get_filename_component(source_name ${source_file} NAME_WE)
        add_custom_command(
            OUTPUT ${CMAKE_ASM_OUTPUT_DIRECTORY}/${source_name}.s  # Assembly output file
            COMMAND ${CMAKE_C_COMPILER} -I${CMAKE_SOURCE_DIR}/include -I${CMAKE_SOURCE_DIR}/lib -g -S -O0 ${source_file} -o ${CMAKE_ASM_OUTPUT_DIRECTORY}/${source_name}.s
            DEPENDS ${source_file}
            COMMENT "Generating assembly for ${source_file}"
        )
        add_custom_target(generate_asm_${source_name} ALL DEPENDS ${CMAKE_ASM_OUTPUT_DIRECTORY}/${source_name}.s)
    endforeach()
endif()

# Check for Linux-specific libraries
if(UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux...")

    # Add XKB library
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(XKB REQUIRED xkbcommon)
    if(XKB_FOUND)
        include_directories(${XKB_INCLUDE_DIRS})
        target_link_libraries(hvc PRIVATE ${XKB_LIBRARIES})
    endif()

    # Add XF86 keysym support
    target_compile_definitions(hvc PRIVATE __linux__)
    
    # Link X11 libraries (using PRIVATE keyword)
    target_link_libraries(hvc PRIVATE ${X11_LIBRARIES} ${X11_Xtst_LIB} pthread)
endif()

if(ENABLE_TESTS)
    add_definitions(-DRUN_TESTS)
endif()

# Find Lua
find_package(Lua REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

# Add to executable target (using PRIVATE keyword)
target_link_libraries(hvc PRIVATE 
    ${LUA_LIBRARIES}
    pthread
    m
)

# Install Lua if needed
if(NOT LUA_FOUND)
    message(FATAL_ERROR "Lua not found - required for scripting support")
endif()

# Install target
install(TARGETS hvc DESTINATION bin)