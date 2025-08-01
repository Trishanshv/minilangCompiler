cmake_minimum_required(VERSION 3.10)
project(minilang LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Bison and Flex
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# Generate parser
BISON_TARGET(Parser
    ${CMAKE_SOURCE_DIR}/src/parser.y
    ${CMAKE_BINARY_DIR}/parser.cpp
    DEFINES_FILE ${CMAKE_BINARY_DIR}/parser.hpp
)

# Generate lexer
FLEX_TARGET(Lexer
    ${CMAKE_SOURCE_DIR}/src/lexer.l
    ${CMAKE_BINARY_DIR}/lexer.cpp
)

# Make sure lexer depends on parser
ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(LLVM_LIBS core irreader mcjit native support)

# Set LLVM components
llvm_map_components_to_libnames(LLVM_LIBS
    core
    executionengine
    irreader
    mcjit
    native
    support
    analysis
    all
)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

# Add executable
add_executable(minilang
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Lexer_OUTPUTS}
    src/main.cpp
    src/codegen.hpp
)

add_library(minilang_lib STATIC
    src/semantic.cpp
    src/codegen.cpp
    src/ast.cpp
    src/ast.hpp
    src/codegen.hpp
)
target_link_libraries(minilang PRIVATE minilang_lib ${LLVM_LIBS})

# Target properties
target_compile_definitions(minilang PRIVATE ${LLVM_DEFINITIONS})
target_include_directories(minilang PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}
    SYSTEM ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(minilang PRIVATE ${LLVM_LIBS})

# Windows-specific settings
if(WIN32)
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
endif()

if(LLVM_VERSION_MAJOR LESS 10)
    message(FATAL_ERROR "LLVM 10 or higher required")
endif()