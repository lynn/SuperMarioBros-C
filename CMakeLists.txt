cmake_minimum_required(VERSION 3.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(smbc)

add_subdirectory(codegen)

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
    source/Util/Video.cpp)

find_package(SDL2 REQUIRED)

add_executable(smbc ${SOURCE_FILES})
target_link_libraries(smbc ${SDL2_LIBRARY} ${SDL2_LIBRARIES})

add_dependencies(smbc codegen)
