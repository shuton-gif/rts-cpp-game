# Vector2D.h

- **Path:** `src/scripts/Vector2D.h`
- **Topic:** core
- **Purpose:** Header-only generic 2D pair (`x`, `y`) used for positions, sizes, directions and tile coordinates.

## Fields

| Name | Type | Meaning |
| --- | --- | --- |
| `x` | `T` | X component. |
| `y` | `T` | Y component. |

## Functions

### `operator==`

Defaulted member (`= default`). Compares `x` and `y` of both sides.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `other` | `const Vector2D&` | Value to compare with. |

**Output:** `bool`: `true` when both `x` and `y` are equal.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/Vector2D.h` | Defined here; call sites not searched |

<!-- Repeat the ### block for each function. List exported ones first. -->

## Used in (type)

Files that include `Vector2D.h` or name `Vector2D<float>` / `Vector2D<int>`. Found with `grep -rln "Vector2D" src/scripts`.

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameObject/gameObject.h` | `pos`, `size` members; `getPos()`, `getSize()`; constructor |
| core | `src/scripts/GameObject/gameObject.cpp` | constructor, `getPos()`, `getSize()` |
| core | `src/scripts/GameScene/main.cpp` | camera, mouse, viewport, tile and draw helpers, NPC and tree spawns |
| economy | `src/scripts/Tree/tree.h` | constructor parameters `pos`, `size` |
| economy | `src/scripts/Tree/tree.cpp` | constructor |
| economy | `src/scripts/Wall/wall.h` | constructor parameters `pos`, `size` |
| economy | `src/scripts/Wall/wall.cpp` | constructor |
| economy | `src/scripts/Item/axe.h` | constructor parameters `pos`, `size` |
| economy | `src/scripts/Item/axe.cpp` | constructor |
| combat | `src/scripts/Arrow/arrow.h` | members `pos`, `dir`; constructor; `getPos()` |
| combat | `src/scripts/Arrow/arrow.cpp` | constructor, `getPos()` |
| combat | `src/scripts/Item/bow.h` | constructor parameters `pos`, `size` |
| combat | `src/scripts/Item/bow.cpp` | constructor |
| unit | `src/scripts/NPC/npc.h` | members `pos`, `size`, `targetedPos`, `targetedMark`, `aim`, route and queue vectors; getters; `setAim()`, `pushTo()`, `setRoute()`, `trackProgress()` |
| unit | `src/scripts/NPC/npc.cpp` | constructor, setters, getters, `trackProgress()` |
| unit | `src/scripts/Item/item.h` | constructor parameters; `drop()`, `moveTo()` |
| unit | `src/scripts/Item/item.cpp` | constructor, `drop()`, `moveTo()` |
| map | `src/scripts/Grass/grass.h` | `GrassClump` members `offset`, `pos`, `size`; `makeGrassClump()` |
| map | `src/scripts/Grass/grass.cpp` | `randomSpot()`, `placeStrips()`, `makeGrassClump()` (`Vector2D<int>`) |
| map | `src/scripts/Map/chunk.h` | members `mapSize`, `treeSize`, `tile`; `chunkStore::update()` |
| map | `src/scripts/Map/chunk.cpp` | `addArea()`, `chunkStore::update()`, feet and center vectors |
| movement | `src/scripts/Movement/arrive.h` | `fuzzyArrive()`, `arriveAround()`, `hasMark()` |
| movement | `src/scripts/Movement/arrive.cpp` | `fuzzyArrive()`, `arriveAround()`, `hasMark()` |
| movement | `src/scripts/Collision/collision.h` | member `pos` |
| pathfinding | `src/scripts/Pathfinding/path.h` | `navWindow` members `origin`, `size`; `navWindowFor()`; `navGrid` methods `cellOf()`, `lineOfSight()`, `build()`, `findPath()` |
| pathfinding | `src/scripts/Pathfinding/path.cpp` | `navWindowFor()`, `navGrid` methods, path corners |
| ai | `src/scripts/AI/agent.h` | `Slot` members `point`, `mark`; member `mapSize`; `position()` |
| ai | `src/scripts/AI/agent.cpp` | `distance()`, `agent::position()`, feet and mark vectors |
| ui | `src/scripts/Editor/editor.h` | `contains()`, `draw()` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::contains()`, `scriptEditor::draw()`, pixel vector |
