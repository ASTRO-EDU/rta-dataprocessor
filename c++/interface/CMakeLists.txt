cmake_minimum_required(VERSION 3.10)

# Interface library for DAMS packet definitions
add_library(InterfacePacket STATIC
    src/packet.cpp
)

target_include_directories(InterfacePacket PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set compile options
target_compile_options(InterfacePacket PRIVATE -Wall -Wextra)

