cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Debugger VERSION 1.0.0)

SET(CMAKE_CXX_FLAGS "-Wall -Wextra -g -O3")

file(GLOB_RECURSE sources src/*.cpp)
file(GLOB_RECURSE headers include/*.hpp)

add_executable(${PROJECT_NAME} ${sources})

target_link_libraries(${PROJECT_NAME} /usr/lib/x86_64-linux-gnu/libopcodes.so)

include_directories(include)
