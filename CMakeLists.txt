cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_COMPILER /usr/bin/gcc-14)
set(CMAKE_CXX_COMPILER /usr/bin/g++-14)

project(Debugger VERSION 1.0.0)

SET(CMAKE_CXX_FLAGS "-Wall -Wextra -O3")

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

set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION)

target_link_libraries(${PROJECT_NAME}
	PRIVATE /usr/lib/x86_64-linux-gnu/libopcodes.so
	PRIVATE ftxui::screen
	PRIVATE ftxui::dom
	PRIVATE ftxui::component
)

include_directories(include)
