cmake_minimum_required(VERSION 4.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(smbc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23")

set(SOURCE_FILES
    source/Main.cpp
    source/Emulation/APU.cpp
    source/Emulation/Controller.cpp
    source/Emulation/Fm2Movie.cpp
    source/Emulation/PPU.cpp
    source/SMB/SMB.cpp
    source/SMB/SMBArea.cpp
    source/SMB/SMBData.cpp
    source/SMB/SMBEnemies.cpp
    source/SMB/SMBEnemyGfx.cpp
    source/SMB/SMBEngine.cpp
    source/SMB/SMBGame.cpp
    source/SMB/SMBObject.cpp
    source/SMB/SMBPlayer.cpp
    source/SMB/SMBSound.cpp
    source/Util/RamMap.cpp
    source/Util/Video.cpp)

find_package(SDL2 REQUIRED)

# Dear ImGui, for the debugging interface. It ships no build of its own: the
# sources are compiled into whatever uses them, along with the backends for the
# platform (SDL2) and the renderer (SDL_Renderer).
#
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)

if(NOT EXISTS ${IMGUI_DIR}/imgui.cpp)
    message(FATAL_ERROR
        "Dear ImGui is missing from ${IMGUI_DIR}. "
        "Run \"git submodule update --init\" to fetch it.")
endif()

set(IMGUI_SOURCE_FILES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp)

add_executable(smbc ${SOURCE_FILES} ${IMGUI_SOURCE_FILES})
target_include_directories(smbc PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends ${SDL2_INCLUDE_DIRS})
target_link_libraries(smbc ${SDL2_LIBRARY} ${SDL2_LIBRARIES})
