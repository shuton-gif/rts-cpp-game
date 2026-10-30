# path

- **Path:** `src/scripts/Pathfinding/path.h`, `src/scripts/Pathfinding/path.cpp`
- **Topic:** pathfinding
- **Purpose:** Builds a nav grid of blocked cells over a window of the map and finds A* paths around solid colliders.

## Functions

### `navWindowFor`

Returns the map rect to grid for one trip from `from` to `to`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `from` | `Vector2D<float>` | trip start in map px |
| `to` | `Vector2D<float>` | trip end in map px |
| `mapSize` | `Vector2D<float>` | map size in map px |

**Output:** `navWindow`: origin and size in map px. Per axis, the span of `from` and `to` grows by `navMargin`, is clamped to the map and snapped outward to `navCell` multiples. A side over `navMaxCells` cells is cut to `navMaxCells` cells, starting `navMargin` behind `from` on the side away from `to`, clamped to the map.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `navGrid::build`

Rebuilds the grid over one window and blocks every cell where an npc at the cell center would overlap a solid.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `solids` | `const std::vector<Collider>&` | solid colliders (standing trees, built walls) |
| `origin` | `Vector2D<float>` | window top left in map px |
| `size` | `Vector2D<float>` | window size in map px |

**Output:** `void`

**Side effects:** Sets `origin`, `width`, `height`. Reassigns `blocked` to all zero, then sets blocked cells. Grows `cost`, `parent`, `stamp`, `closed`, `component` by resize when the window is larger than any before. Sets `labeled` to false. A cell is blocked when the distance from its center to the solid, with y stretched by `ovalRatio` (2), is below `solid.radius + colliderRadius`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `navGrid::findPath`

Returns waypoints after `from` that always end with exactly `to`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `from` | `Vector2D<float>` | start in map px |
| `to` | `Vector2D<float>` | destination in map px |

**Output:** `std::vector<Vector2D<float>>`: `{to}` when the grid is empty or the straight line is clear. Else an A* path (8 neighbors, octile heuristic, no corner cutting) pulled tight into straight segments, ending with `to`. A blocked goal is swapped for its nearest free cell. An unreachable goal is swapped for the reachable cell closest to it.

**Side effects:** Increments `generation` (zeroes `stamp` and `closed` when it wraps to 0). Writes `stamp`, `cost`, `parent`, `closed`. Calls `label` when `labeled` is false.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `navGrid::cellOf`

Returns the grid cell index of a map point, clamped to the edge cells.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `point` | `Vector2D<float>` | point in map px |

**Output:** `int`: cell index `y * width + x`.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| pathfinding | `src/scripts/Pathfinding/path.cpp` | `navGrid::lineOfSight` |
| pathfinding | `src/scripts/Pathfinding/path.cpp` | `navGrid::findPath` |

### `navGrid::isFree`

Tells whether a cell is walkable; the start cell always counts as free.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `cell` | `int` | cell index |
| `start` | `int` | start cell index of the search |

**Output:** `bool`: true when the cell is not blocked or is `start`.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| pathfinding | `src/scripts/Pathfinding/path.cpp` | `navGrid::nearestFree` |
| pathfinding | `src/scripts/Pathfinding/path.cpp` | `navGrid::findPath` |

### `navGrid::lineOfSight`

Checks that no blocked cell lies on segment `a` to `b`, sampled every `sightStep` (a quarter cell).

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `a` | `Vector2D<float>` | segment start in map px |
| `b` | `Vector2D<float>` | segment end in map px |
| `skipCell` | `int` | cell ignored even if blocked (the cell the walk starts in) |

**Output:** `bool`: true when the segment is clear.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| pathfinding | `src/scripts/Pathfinding/path.cpp` | `navGrid::findPath` |

### `navGrid::nearestFree`

Finds the free cell nearest to a blocked goal by scanning rings of growing radius up to `goalSearchCells`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `goal` | `int` | blocked goal cell index |
| `start` | `int` | start cell index (counts as free) |

**Output:** `int`: nearest free cell by straight distance, or -1 when none.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| pathfinding | `src/scripts/Pathfinding/path.cpp` | `navGrid::findPath` |

### `navGrid::label`

Joins free cells into connected areas with one union find pass, using the same steps as A* (a diagonal only when both cut cells are free).

**Input:** none

**Output:** `void`

**Side effects:** Writes `component` (area root per free cell, -1 for blocked cells). Sets `labeled` to true.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| pathfinding | `src/scripts/Pathfinding/path.cpp` | `navGrid::findPath` |

### `navNode::operator>`

Orders A* open list nodes: larger `f` is greater; on equal `f`, smaller `g` is greater, so the node with more cost is taken first.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `other` | `const navNode&` | node to compare against |

**Output:** `bool`: true when this node ranks after `other`.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| pathfinding | `src/scripts/Pathfinding/path.cpp` | `navGrid::findPath` (via `std::greater<navNode>` in the `std::priority_queue`) |
