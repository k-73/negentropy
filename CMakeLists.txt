# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(Diagram LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
add_executable(Diagram main.cpp)
target_link_libraries(Diagram PRIVATE SDL2::SDL2)
