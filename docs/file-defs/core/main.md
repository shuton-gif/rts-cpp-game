# main

- **Path:** `src/scripts/GameScene/main.cpp`
- **Topic:** core
- **Purpose:** Program entry point. It opens the SDL3 window, loads sprites, scripts and the world, and runs the input, update and draw loop.

## Functions

### `main`

Starts SDL, builds the scene, runs the game loop until quit, then frees everything.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (unnamed) | `int` | argc, unused |
| (unnamed) | `char**` | argv, unused |

**Output:** `int`: 0 on normal exit, 1 (from `fail`) when an SDL call or a sprite load fails

**Side effects:** Calls `SDL_Init`, creates a full-screen window and renderer with vsync and mouse grab. Sets globals `density`, `zoom`, `camera`, `mouse`, `renderView`. Loads npc, footprint, cross, arrow, grass, tree, item, plank, post and Terminal button textures from `assets/`. Loads scripts from `assets/scripts` (again on F5 and on editor save). Creates npcs, trees, items, agents, a `chunkStore` and a `navGrid`. Each frame it handles events (Esc, U, I, F5, Shift+1, B, Q, E, mouse buttons, wheel, pixel size change), moves the camera, streams chunks, builds paths, updates npcs, collisions, items, agents, arrows, plank drops and hp bar timers, then draws and presents. On exit destroys all textures, the renderer and the window, and calls `SDL_Quit`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | entry point |

### `IntervalTimer::tick`

Returns the ms since the previous call and resets the start point.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| none | | |

**Output:** `float`: elapsed ms since `last`, from `SDL_GetTicksNS`

**Side effects:** Sets member `last` to now.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `cameraPx`

Returns the camera position rounded down to whole map px.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| none | | |

**Output:** `Vector2D<float>`: floor of global `camera`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `toMap` |
| core | `src/scripts/GameScene/main.cpp` | `drawBackground` |
| core | `src/scripts/GameScene/main.cpp` | `inRenderArea` |
| core | `src/scripts/GameScene/main.cpp` | `drawSprite` |
| core | `src/scripts/GameScene/main.cpp` | `drawOutline` |
| core | `src/scripts/GameScene/main.cpp` | `drawHitbox` |
| core | `src/scripts/GameScene/main.cpp` | `drawHpBar` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `toPixels`

Converts a window point to real screen pixels.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `x` | `const float` | x in window points |
| `y` | `const float` | y in window points |

**Output:** `Vector2D<float>`: the point times global `density`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `terminalButtonRect`

Returns the Terminal button's rect: the 16 px icon at a whole `3 x density` scale, its top 25% down the screen, 12 points from the left edge.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `viewport` | `const Vector2D<float>` | viewport size in real pixels |

**Output:** `SDL_FRect`: the button in real pixels

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `viewSize`

Returns the visible map area for a viewport at the current zoom.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `viewport` | `const Vector2D<float>` | window size in real pixels |

**Output:** `Vector2D<float>`: viewport divided by global `zoom`, in map px

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `clampCamera` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `toMap`

Converts a screen pixel point to a map px point.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `screen` | `const Vector2D<float>` | point in real screen pixels |

**Output:** `Vector2D<float>`: screen / zoom + whole-px camera

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `clampCamera`

Keeps the camera inside the map for the given viewport.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `viewport` | `const Vector2D<float>` | window size in real pixels |

**Output:** `void`

**Side effects:** Clamps global `camera` to 0 .. `bgSize` minus the view size.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `moveCamera` |
| core | `src/scripts/GameScene/main.cpp` | `zoomCamera` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `moveCamera`

Edge-scrolls the camera when the cursor is within `edgeMargin` of a window edge.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `viewport` | `const Vector2D<float>` | window size in real pixels |
| `deltaTime` | `const float` | frame time in ms |

**Output:** `void`

**Side effects:** Moves global `camera` by `cameraSpeed * density * deltaTime / zoom` per edge, then calls `clampCamera`. Reads global `mouse`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `zoomLowest`

Returns the smallest whole zoom so the view is never bigger than the map.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `viewport` | `const Vector2D<float>` | window size in real pixels |

**Output:** `float`: max of 1 and ceil(viewport / bgSize) on each axis

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `zoomCamera` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `zoomCamera`

Changes zoom by whole wheel notches and keeps the map point under the cursor fixed.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `viewport` | `const Vector2D<float>` | window size in real pixels |
| `cursor` | `const Vector2D<float>` | cursor in real screen pixels |
| `wheel` | `const float` | wheel movement this event |

**Output:** `void`

**Side effects:** Adds to global `wheelTotal`. On a full notch, sets global `zoom` (clamped between `zoomLowest` and `zoomMaxPerDensity * density`), shifts global `camera`, and calls `clampCamera`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `drawBackground`

Fills the whole map as one green (#61d28d) rect shifted by the camera.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | target renderer |

**Output:** `bool`: false if an SDL call fails

**Side effects:** `SDL_SetRenderDrawColor`, `SDL_RenderFillRect`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `loadSprite`

Loads a PNG from the executable's `assets/` folder as a nearest-scaled texture.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | renderer that owns the texture |
| `name` | `const char*` | file name inside `assets/` |

**Output:** `SDL_Texture*`: the texture, or nullptr on any failure

**Side effects:** Reads the file with `SDL_LoadPNG`, creates a texture, sets `SDL_SCALEMODE_NEAREST`. Caller owns the texture.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `loadGrassSprites` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `loadRecolored`

Loads a PNG and swaps every pixel that exactly matches one color for another.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | renderer that owns the texture |
| `name` | `const char*` | file name inside `assets/` |
| `from` | `const SDL_Color` | color to replace (exact r, g, b, a match) |
| `to` | `const SDL_Color` | new r, g, b (alpha kept) |

**Output:** `SDL_Texture*`: the recolored texture, or nullptr on any failure

**Side effects:** Reads the file, converts to RGBA32, edits pixels under `SDL_LockSurface`, creates a nearest-scaled texture. Caller owns the texture.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `facingAngle`

Maps a facing to a clockwise angle from up, for footprints.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `facing` | `const Facing` | npc facing |

**Output:** `double`: 0, 45, 90, ... 315 degrees

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `inRenderArea`

Tells if a map px rect touches the visible map grown by `renderMargin`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `rect` | `const SDL_FRect&` | rect in map px |

**Output:** `bool`: true if any part is inside the render area

**Side effects:** none (reads globals `renderView`, `zoom`, `camera`)

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `drawSprite` |
| core | `src/scripts/GameScene/main.cpp` | `drawHitbox` |
| core | `src/scripts/GameScene/main.cpp` | `drawHpBar` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `drawSprite`

Draws a texture centered on a map point, scaled, faded, rotated and flipped.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | target renderer |
| `texture` | `SDL_Texture*` | sprite to draw |
| `center` | `const Vector2D<float>` | center in map px |
| `alpha` | `const Uint8` | alpha mod |
| `angle` | `const double` | clockwise degrees, default 0 |
| `size` | `const float` | multiplier on `spriteScale`, default 1 |
| `flip` | `const SDL_FlipMode` | flip mode, default `SDL_FLIP_NONE` |

**Output:** `bool`: false if an SDL call fails; true when skipped outside the render area

**Side effects:** `SDL_SetTextureAlphaMod` on the texture, `SDL_RenderTextureRotated`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `spriteFor`

Picks the npc sprite for its player color and facing.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `sprites` | `const Sprites&` | loaded sprite sets |
| `unit` | `const npc&` | npc to draw |

**Output:** `SDL_Texture*`: up, left or right texture; straight down uses the side last faced

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `npcRect`

Returns an npc's sprite rect in map px, from its feet up.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `unit` | `const npc&` | npc |

**Output:** `SDL_FRect`: rect centered on x, bottom at pos, size times `spriteScale`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `boxBetween`

Returns the tile-snapped selection box between two map points.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `a` | `const Vector2D<float>` | drag start in map px |
| `b` | `const Vector2D<float>` | drag end in map px |

**Output:** `SDL_FRect`: box covering every whole tile from a's tile to b's tile

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `overlaps`

Tests if two rects touch, edges included.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `a` | `const SDL_FRect&` | first rect |
| `b` | `const SDL_FRect&` | second rect |

**Output:** `bool`: true if they overlap or touch

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `canPlace` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `drawOutline`

Draws the drag selection box: white fill at `selectFillAlpha`, then a solid white outline.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | target renderer |
| `rect` | `SDL_FRect` | box in map px |

**Output:** `bool`: false if an SDL call fails

**Side effects:** Sets blend mode to `SDL_BLENDMODE_BLEND`, draw color, `SDL_RenderFillRect`, `SDL_RenderRect`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `drawHitbox`

Draws an oval collider's bounding rect in red at `hitboxAlpha` (dev view).

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | target renderer |
| `oval` | `const Collider&` | collider to show |

**Output:** `bool`: false if an SDL call fails; true when skipped outside the render area

**Side effects:** Sets blend mode and draw color, `SDL_RenderFillRect`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `drawHpBar`

Draws a white hp or progress bar `hpBarGap` below a feet point.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | target renderer |
| `feet` | `const Vector2D<float>` | owner's feet in map px |
| `left` | `const float` | filled part, 0 to 1 |
| `width` | `const float` | bar width in map px, default `hpBarSize.x` |

**Output:** `bool`: false if an SDL call fails; true when skipped outside the render area

**Side effects:** Sets draw color, `SDL_RenderFillRect`, `SDL_RenderRect`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `drawPathLine`

Draws a white path `pathWidth` wide through points, with a square on each joint.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | target renderer |
| `points` | `const std::vector<SDL_FPoint>&` | path points, already minus the camera |

**Output:** `bool`: false if an SDL call fails

**Side effects:** `SDL_RenderGeometry` per segment, sets draw color, `SDL_RenderFillRect` per point.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `loadGrassSprites`

Loads the grass sprite `grassv2.png`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | renderer that owns the texture |

**Output:** `std::vector<SDL_Texture*>`: one texture, or empty on failure

**Side effects:** Creates a texture via `loadSprite`. Caller owns it.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `addDrawable`

Queues a sprite whose feet are at a map point, for the feet-y sorted draw list.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `drawables` | `std::vector<Drawable>&` | draw list |
| `texture` | `SDL_Texture*` | sprite |
| `feet` | `const Vector2D<float>` | center bottom in map px |
| `size` | `const float` | multiplier on `spriteScale` |
| `flip` | `const SDL_FlipMode` | flip mode, default `SDL_FLIP_NONE` |

**Output:** `bool`: false if `SDL_GetTextureSize` fails

**Side effects:** Pushes one `Drawable` onto `drawables` (center half its height above feet, angle 0).

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `addGrassClump` |
| core | `src/scripts/GameScene/main.cpp` | `addWallDrawables` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `addGrassClump`

Queues every strip of a grass clump at `grassSize`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `drawables` | `std::vector<Drawable>&` | draw list |
| `grassSprites` | `const std::vector<SDL_Texture*>&` | grass textures |
| `clump` | `const GrassClump&` | clump data |

**Output:** `bool`: false if any `addDrawable` fails

**Side effects:** Pushes one `Drawable` per strip onto `drawables`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `objectRect`

Returns a game object's sprite rect in map px, from its feet up.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `object` | `const GameObject&` | tree, wall or item |

**Output:** `SDL_FRect`: rect centered on x, bottom at pos, size already in map px

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `handsFree`

Tells if an npc holds no item and is not fetching one.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `items` | `const std::vector<std::unique_ptr<item>>&` | all items |
| `id` | `const int` | npc id |

**Output:** `bool`: true if no item has this id as holder or fetcher

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `tileOf`

Returns the tile a map point is in.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `point` | `const Vector2D<float>` | point in map px |

**Output:** `Vector2D<int>`: floor(point / tile size)

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `snapPost` |
| core | `src/scripts/GameScene/main.cpp` | `planLine` |

### `tileFeet`

Returns where a post on a tile stands: the middle of the tile's bottom edge.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `tile` | `const Vector2D<int>` | tile coords |

**Output:** `Vector2D<float>`: feet point in map px

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `planLine` |

### `postOf`

Returns which post sprite a wall uses.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `segment` | `const wall&` | wall |

**Output:** `int`: 1 if its image is `postImages[1]`, else 0

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `planLine` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `snapPost`

Finds the nearest non-destroyed post or plan within `snapReach` tiles and moves the tile onto it.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `walls` | `const std::vector<wall>&` | all walls and plans |
| `tile` | `Vector2D<int>&` | tile to snap, changed in place |

**Output:** `int`: index of the snapped wall, or -1 if none is close enough

**Side effects:** Writes `tile` when a post is found.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `planLine` |

### `planLine`

Plans a straight line of posts between two map points (Bresenham), snapped to nearby posts.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `from` | `const Vector2D<float>` | drag start in map px |
| `to` | `const Vector2D<float>` | drag end in map px |
| `walls` | `const std::vector<wall>&` | existing walls and plans |

**Output:** `std::vector<PlanSpot>`: feet and post sprite (alternating 0/1) of each tile on the line

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `makeWall`

Builds a wall object from a plan spot.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `spot` | `const PlanSpot&` | feet and post index |
| `look` | `const WallLook&` | post sizes |
| `group` | `std::shared_ptr<WallGroup>` | group the post shares; default is a new one, used for ghosts |

**Output:** `wall`: wall at `spot.pos` with that post's size, image name and group

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `wallFootprint`

Returns the one tile a post stands on.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `segment` | `const wall&` | wall |

**Output:** `SDL_FRect`: tile-sized rect above the wall's feet

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `canPlace` |

### `addWallDrawables`

Queues a post sprite at a plan spot with an alpha.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `drawables` | `std::vector<Drawable>&` | draw list |
| `look` | `const WallLook&` | post textures and draw size |
| `spot` | `const PlanSpot&` | feet and post index |
| `alpha` | `const Uint8` | alpha to draw with |

**Output:** `bool`: false if `addDrawable` fails

**Side effects:** Pushes one `Drawable` onto `drawables` and sets its alpha.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `canPlace`

Tells if a planned post fits: on the map, not on another post's tile, not inside a standing tree's solid oval.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `candidate` | `const wall&` | planned post |
| `walls` | `const std::vector<wall>&` | existing walls and plans |
| `trees` | `const std::vector<tree>&` | all trees |

**Output:** `bool`: true if the post can be placed

**Side effects:** none (tree test uses collider copies)

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `clumpRect`

Returns a grass clump's sprite bound in map px, grown by `clumpMargin`, for culling.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `clump` | `const GrassClump&` | clump |

**Output:** `SDL_FRect`: bound centered on the clump pos

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `fail`

Logs an SDL error and quits SDL.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `what` | `const char*` | name of the failed step |

**Output:** `int`: always 1

**Side effects:** `SDL_Log` with `SDL_GetError`, then `SDL_Quit`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |
