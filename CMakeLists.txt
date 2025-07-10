cmake_minimum_required(VERSION 3.10)
project(minilang LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Set LLVM includes and defines
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Map LLVM components
llvm_map_components_to_libnames(LLVM_LIBS core orcjit native)

# Add the executable after defining sources
add_executable(minilang
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Lexer_OUTPUTS}
    src/main.cpp
    src/semantic.cpp
    src/codegen.cpp
    src/codegen.hpp
)

# Include parser headers and src directory
target_include_directories(minilang PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Link LLVM libraries
target_link_libraries(minilang PRIVATE ${LLVM_LIBS})
