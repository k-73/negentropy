cmake_minimum_required(VERSION 3.16)
project(negentropy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

find_package(SDL2 REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "Core/*.cpp" "Core/*.hpp")

add_executable(negentropy ${SOURCES})

target_include_directories(negentropy PRIVATE Core)

target_link_libraries(negentropy PRIVATE SDL2::SDL2 nlohmann_json::nlohmann_json)