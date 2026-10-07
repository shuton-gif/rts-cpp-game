cmake_minimum_required(VERSION 3.24)
project(rts-cpp-game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# build: editor could not find SDL3 headers; CMake now writes build/compile_commands.json so the editor uses the same include paths as the compiler

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.16
    GIT_SHALLOW TRUE
    FIND_PACKAGE_ARGS 3.4 CONFIG
)
FetchContent_MakeAvailable(SDL3)
# build: SDL3 must build the same on every machine and sprites load with SDL_LoadPNG (new in 3.4); an installed SDL3 3.4+ is used when found, else the pinned tag is fetched and built from source

add_executable(rts
    src/scripts/AI/agent.cpp
    src/scripts/AI/script.cpp
    src/scripts/Arrow/arrow.cpp
    src/scripts/Collision/collision.cpp
    src/scripts/Editor/editor.cpp
    src/scripts/GameObject/gameObject.cpp
    src/scripts/GameScene/main.cpp
    src/scripts/Grass/grass.cpp
    src/scripts/Item/axe.cpp
    src/scripts/Item/bow.cpp
    src/scripts/Item/item.cpp
    src/scripts/Movement/arrive.cpp
    src/scripts/Pathfinding/path.cpp
    src/scripts/Map/chunk.cpp
# map: grass and trees load per chunk near the view instead of over the whole map; chunk.cpp holds the chunk store
    src/scripts/NPC/npc.cpp
    src/scripts/Tree/tree.cpp
    src/scripts/Wall/wall.cpp
)
# pathfinding: npcs need paths around trees and walls; path.cpp holds the nav grid and A*
target_include_directories(rts PRIVATE src/scripts)
# build: Editor/editor.cpp is new; every source under src/scripts is listed in `rts` and src/scripts is the include root
target_link_libraries(rts PRIVATE SDL3::SDL3)
target_compile_definitions(rts PRIVATE RTS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
# ui_editor: the in-game script editor saves into the repo so edits survive the asset copy of the next build; RTS_SOURCE_DIR is the repo root as a string literal

add_custom_command(TARGET rts POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:rts>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:rts>/assets
)
# assets: game loads sprites from the folder of the executable and grass strips are found by pattern, so a deleted sprite must not linger; every build removes the copied folder and copies assets/ next to `rts` again

if(MSVC)
    target_compile_options(rts PRIVATE /W4)
else()
    target_compile_options(rts PRIVATE -Wall -Wextra)
endif()
# build: first target of the game; one executable `rts` from GameScene/main.cpp with warnings on for clang, gcc and MSVC
