cmake_minimum_required(VERSION 3.16)
project(negentropy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.15.3
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.1
)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)

FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG v3.15.0
)

FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG v0.9.7
)

FetchContent_Declare(
  pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG v1.15
)

FetchContent_Declare(
  boost_pfr
  GIT_REPOSITORY https://github.com/boostorg/pfr.git
  GIT_TAG "boost-1.88.0"
)

FetchContent_MakeAvailable(
    boost_pfr
    nlohmann_json
    spdlog
    imgui
    glm
    entt
    magic_enum
    pugixml
)

if(EMSCRIPTEN)
    set(USE_FLAGS "-s USE_SDL=2 -s WASM=1 -s FULL_ES2=1")
    set(PERFORMANCE_FLAGS "-O3 -s GL_UNSAFE_OPTS=1 -s TEXTDECODER=2 -s ENVIRONMENT=web")
    set(RESPONSIVENESS_FLAGS "-s GL_PREINITIALIZED_CONTEXT=1 -s GL_POOL_TEMP_BUFFERS=0 -s OFFSCREENCANVAS_SUPPORT=0 -s OFFSCREEN_FRAMEBUFFER=0")
    set(IMMEDIATE_FLAGS "-s GL_TRACK_ERRORS=0 -s GL_ASSERTIONS=0 -s GL_DEBUG=0 -s DISABLE_EXCEPTION_CATCHING=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS} ${PERFORMANCE_FLAGS} ${RESPONSIVENESS_FLAGS} ${IMMEDIATE_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS} ${PERFORMANCE_FLAGS} ${RESPONSIVENESS_FLAGS} ${IMMEDIATE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS} ${PERFORMANCE_FLAGS} ${RESPONSIVENESS_FLAGS} ${IMMEDIATE_FLAGS} -s ALLOW_MEMORY_GROWTH=1 --preload-file ${CMAKE_SOURCE_DIR}/Assets@Assets --preload-file ${CMAKE_SOURCE_DIR}/Workspace@Workspace")
else()
    find_package(SDL2 REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "Core/*.cpp" "Core/*.hpp")

# ImGui configuration
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# ImGui sources
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

add_executable(negentropy ${SOURCES} ${IMGUI_SOURCES})

target_include_directories(negentropy PRIVATE
    Core
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${glm_SOURCE_DIR}
    ${magic_enum_SOURCE_DIR}/include
    ${entt_SOURCE_DIR}/src
    ${boost_pfr_SOURCE_DIR}/include
)

target_compile_definitions(negentropy PRIVATE PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

if(EMSCRIPTEN)
    target_link_libraries(negentropy
            PRIVATE
            spdlog::spdlog
            EnTT::EnTT
            nlohmann_json::nlohmann_json
            magic_enum::magic_enum
            pugixml::pugixml
    )
else()
    target_link_libraries(negentropy
            PRIVATE
            SDL2::SDL2 
            PkgConfig::SDL2_TTF
            spdlog::spdlog
            EnTT::EnTT
            nlohmann_json::nlohmann_json
            magic_enum::magic_enum
            pugixml::pugixml
    )
endif()

if(EMSCRIPTEN)
    set_target_properties(negentropy PROPERTIES SUFFIX ".html")
    set_target_properties(negentropy PROPERTIES 
        LINK_FLAGS "--shell-file ${CMAKE_SOURCE_DIR}/shell.html"
    )
endif()