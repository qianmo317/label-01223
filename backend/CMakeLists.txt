cmake_minimum_required(VERSION 3.16)
project(tetris_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/game.cpp
    src/websocket_server.cpp
)

# Create executable
add_executable(tetris_server ${SOURCES})

# Include directories
target_include_directories(tetris_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Link libraries
target_link_libraries(tetris_server PRIVATE Threads::Threads)

# Install
install(TARGETS tetris_server DESTINATION bin)
