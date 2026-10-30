# collision

- **Path:** `src/scripts/Collision/collision.h`, `src/scripts/Collision/collision.cpp`
- **Topic:** movement
- **Purpose:** Pushes overlapping solid colliders apart, so npcs, trees and walls do not share ground. A `Collider` is an oval: `pos` is its center in map px, `radius` is its half width (half height is `radius / 2`), and `fixed` marks things a collision never moves.

## Functions

### `separate`

Checks two colliders for overlap and, if they overlap, pushes them apart along the line between their centers.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `a` | `Collider&` | First collider. Its `pos` may be changed. |
| `b` | `Collider&` | Second collider. Its `pos` may be changed. |

**Output:** `bool`: `true` if the two overlapped and were pushed apart, `false` if they did not overlap or both are `fixed`.

**Side effects:** Changes `a.pos` and `b.pos` in place when they overlap. A `fixed` collider is not moved. If one is `fixed`, only the other moves, by the full overlap. If neither is `fixed`, each moves by half. Two colliders at the same point are pushed sideways along x. No other state is changed.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `canPlace` |
| core | `src/scripts/GameScene/main.cpp` | `main` (chop hitbox vs. swing check, tree stand) |
| core | `src/scripts/GameScene/main.cpp` | `main` (wall hitbox vs. swing check) |
| movement | `src/scripts/Movement/arrive.cpp` | `arriveAround` |
| movement | `src/scripts/Collision/collision.cpp` | `resolveCollisions` |
| ai | `src/scripts/AI/agent.cpp` | `overlaps` (static helper, used by `agent::beside`) |

### `resolveCollisions`

Pushes apart every overlapping pair in a list of colliders, repeating up to `passes` times.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `colliders` | `std::vector<Collider>&` | One collider per solid thing. Overlapping ones are moved in place. The caller copies the moved positions back. |

**Output:** `void`

**Side effects:** Changes `pos` of the non-`fixed` colliders in `colliders` in place, through `separate`. Allocates temporary grids (`fixedGrid`, `freeGrid`) and a `seen` vector on each call. Stops early when a pass moves nothing.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (builds the list from npcs, trees and built walls) |

### `cellKey`

Packs a grid cell coordinate into one 64-bit key.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `x` | `const int` | Cell column. |
| `y` | `const int` | Cell row. |

**Output:** `std::int64_t`: the key, `x` in the upper 32 bits and `y` in the lower 32 bits (XOR-combined).

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| movement | `src/scripts/Collision/collision.cpp` | `insert`, `resolveCollisions` |

### `cellSpan`

Computes the range of grid cells that a collider's bounding box covers.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `c` | `const Collider&` | The collider to measure. |
| `x0` | `int&` | Output: first column. |
| `y0` | `int&` | Output: first row. |
| `x1` | `int&` | Output: last column. |
| `y1` | `int&` | Output: last row. |

**Output:** `void`. The range is returned through `x0`, `y0`, `x1`, `y1`. The box spans `radius` in x and `radius / ovalRatio` in y, cells are `gridCell` px.

**Side effects:** none. Only the four output references are written.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| movement | `src/scripts/Collision/collision.cpp` | `insert`, `resolveCollisions` |

### `insert`

Adds a collider's index to every grid cell its bounding box covers.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `grid` | `Grid&` | Grid to add to (`std::unordered_map<std::int64_t, std::vector<std::size_t>>`). |
| `colliders` | `const std::vector<Collider>&` | All colliders. |
| `i` | `const std::size_t` | Index of the collider to insert. |

**Output:** `void`

**Side effects:** Appends `i` to the index lists of the covered cells in `grid`. Creates new cells in `grid` as needed. Does not change `colliders`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| movement | `src/scripts/Collision/collision.cpp` | `resolveCollisions` (for fixed colliders and for free colliders each pass) |
