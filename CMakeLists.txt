cmake_minimum_required(VERSION 3.20)
project(lpcvm CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W3 /EHsc)
else()
    add_compile_options(-Wall -Wextra)
endif()

add_executable(lpcvm
    src/main.cpp
    src/value.cpp
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/interpreter.cpp
    src/builtins.cpp
    src/o_loader.cpp
    src/lzss.cpp
    src/pak_loader.cpp
    src/original_vm.cpp
)

target_include_directories(lpcvm PRIVATE src)
if(WIN32)
    target_link_libraries(lpcvm PRIVATE ws2_32)
endif()
