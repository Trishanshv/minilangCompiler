cmake_minimum_required(VERSION 3.10)
project(minilang LANGUAGES C CXX)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(Parser
    ${CMAKE_SOURCE_DIR}/src/parser.y
    ${CMAKE_BINARY_DIR}/parser.cpp
    DEFINES_FILE ${CMAKE_BINARY_DIR}/parser.hpp
)

FLEX_TARGET(Lexer
    ${CMAKE_SOURCE_DIR}/src/lexer.l
    ${CMAKE_BINARY_DIR}/lexer.cpp
)

ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

add_executable(minilang
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Lexer_OUTPUTS}
    src/main.cpp
    src/ast.hpp
)

target_include_directories(minilang PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
)
