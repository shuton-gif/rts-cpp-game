# arrow

- **Path:** `src/scripts/Arrow/arrow.h`, `src/scripts/Arrow/arrow.cpp`
- **Topic:** combat
- **Purpose:** One shot arrow: it flies straight along its direction for `arrowRange` map px, then lies still until `arrowLife` ms after the shot, and can be spent when it hits a tree or wall.

## Functions

### `arrow` (constructor)

Creates an arrow at the bow's position, flying along the aim at the bow's height.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pos` | `Vector2D<float>` | Start point in map px (center of the sprite) |
| `dir` | `Vector2D<float>` | Flight direction, a unit vector |
| `height` | `float` | Height above the ground, used by `getFeet` |

**Output:** `arrow`: a new arrow (no return value; constructor)

**Side effects:** none (sets the member fields only)

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| combat | `src/scripts/Item/bow.cpp` | `bow::use` (via `arrows.emplace_back`) |

### `update`

Ages the arrow and moves it along `dir`, stopping exactly at `arrowRange`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `deltaTime` | `float` | Frame time in ms |

**Output:** `void`

**Side effects:** Adds `deltaTime` to `age`. Moves `pos` by `dir * step` and adds `step` to `traveled`, where `step` is `min(arrowSpeed * deltaTime, arrowRange - traveled)`. Does nothing to `pos` once `step <= 0`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| combat | `src/scripts/GameScene/main.cpp` | `main` |

### `getPos`

Returns the current center position of the arrow.

**Input:** none

**Output:** `Vector2D<float>`: `pos` in map px

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| combat | `src/scripts/GameScene/main.cpp` | `main` (hit test point, and draw call) |

### `getFeet`

Returns the y used for depth sorting: `pos.y + height`.

**Input:** none

**Output:** `float`: ground y in map px

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| combat | `src/scripts/GameScene/main.cpp` | `main` (drawable sort key) |

### `getAngle`

Returns the flight direction in degrees, clockwise from right (map y points down).

**Input:** none

**Output:** `double`: `atan2(dir.y, dir.x)` converted to degrees

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| combat | `src/scripts/GameScene/main.cpp` | `main` (sprite rotation) |

### `isGone`

Tells whether the arrow should be removed from the list.

**Input:** none

**Output:** `bool`: true if `spent` is set or `age >= arrowLife`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| combat | `src/scripts/GameScene/main.cpp` | `main` (`std::erase_if` lambda) |

### `isFlying`

Tells whether the arrow can still hit something.

**Input:** none

**Output:** `bool`: true if not `spent` and `traveled < arrowRange`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| combat | `src/scripts/GameScene/main.cpp` | `main` (skip non-flying arrows; wall loop condition) |

### `spend`

Marks the arrow as having hit something, so it stops flying and is removed on the next `isGone` check.

**Input:** none

**Output:** `void`

**Side effects:** Sets `spent = true`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| combat | `src/scripts/GameScene/main.cpp` | `main` (tree hit and wall hit) |
