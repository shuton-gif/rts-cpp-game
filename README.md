# rts-cpp-game
real time strategy game in c++

## Game Style

2D pixel RTS PVP or PVE (1v1, 2v2, 3v3, 4v4, etc)

## Dependencies

Platforms: Windows, macOS

### Toolchain

| need | pick | version |
| --- | --- | --- |
| language | C++ | 20 |
| build | CMake | 3.24+ |
| packages | CMake FetchContent | pinned git tag per library |

### Libraries

| need | pick | note |
| --- | --- | --- |
| window, input, render | SDL3 | 3.2+, `SDL_Renderer` |
| image | SDL3_image | PNG sprite sheets |
| font | SDL3_ttf | |
| audio | SDL3_mixer | version TBD |
| fixed-point math | fpm | simulation only, no floats in sim |
| network transport | GameNetworkingSockets | same API as Steam networking |
| data files | nlohmann/json | unit stats, maps, settings |
| debug UI | Dear ImGui | dev builds only |
| test | doctest | sim and determinism tests |

### Multiplayer

- deterministic lockstep: only player commands are sent, every client runs the same simulation
- simulation uses fixed tick and fixed-point math so all clients get the same result

### Steam (future)

- Steamworks SDK, behind a compile flag, game builds and runs without it
- SDK is not in this repo (licence forbids redistribution), build takes a path to it
- transport swaps from GameNetworkingSockets to Steam relay
<!-- build: dependencies decided (lockstep, GameNetworkingSockets, FetchContent, Windows + macOS); libraries are fetched by CMake at pinned tags and Steam stays optional behind a flag -->
