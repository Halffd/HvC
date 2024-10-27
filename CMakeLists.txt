cmake_minimum_required(VERSION 3.28.1)
project(hvc CXX)

# Create a boolean option for assembly generation
option(GENERATE_ASM "Generate assembly files" ON)

file(GLOB_RECURSE source_files "src/*.cpp")
file(GLOB_RECURSE source_c_files "src/*.c")

set(src ${source_files} ${source_c_files})

add_executable(hvc ${src})

include_directories("${CMAKE_SOURCE_DIR}/include")

# Set output properties 
set_target_properties(hvc PROPERTIES OUTPUT_NAME c)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

# Set the output directory for the assembly files
set(CMAKE_ASM_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/asm)

# Ensure the output directory exists
file(MAKE_DIRECTORY ${CMAKE_ASM_OUTPUT_DIRECTORY})

# Loop through source files to generate assembly if the option is enabled
if(GENERATE_ASM)
    foreach(source_file IN LISTS src)
        get_filename_component(source_name ${source_file} NAME_WE)
        add_custom_command(
            OUTPUT ${CMAKE_ASM_OUTPUT_DIRECTORY}/${source_name}.s  # Assembly output file
            COMMAND ${CMAKE_C_COMPILER} -I${LUAJIT_DIR}/include -I${CMAKE_SOURCE_DIR}/lib -g -S -O0 ${source_file} -o ${CMAKE_ASM_OUTPUT_DIRECTORY}/${source_name}.s
            DEPENDS ${source_file}
            COMMENT "Generating assembly for ${source_file}"
        )
        add_custom_target(generate_asm_${source_name} ALL DEPENDS ${CMAKE_ASM_OUTPUT_DIRECTORY}/${source_name}.s)
    endforeach()
endif()