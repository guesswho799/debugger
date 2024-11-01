cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Debugger VERSION 1.0.0)

SET(CMAKE_CXX_FLAGS "-Wall -Wextra -g -O3")

file(GLOB_RECURSE sources src/*.cpp)
file(GLOB_RECURSE headers include/*.hpp)

include(FetchContent)
FetchContent_Declare(
  FTXUI
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG v5.0.0
)
FetchContent_MakeAvailable(FTXUI)

add_executable(${PROJECT_NAME} ${sources})

target_link_libraries(${PROJECT_NAME} /usr/lib/x86_64-linux-gnu/libopcodes.so)

include_directories(include)
