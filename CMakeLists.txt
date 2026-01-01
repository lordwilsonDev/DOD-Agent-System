cmake_minimum_required(VERSION 3.10)
project(DOD_Agent_System)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags for optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
)

# Create executable
add_executable(dod_simulation ${SOURCES})

# Enable warnings
target_compile_options(dod_simulation PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

print("DOD Agent System configured successfully")
