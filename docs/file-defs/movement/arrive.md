# arrive

- **Path:** `src/scripts/Movement/arrive.h`, `src/scripts/Movement/arrive.cpp`
- **Topic:** movement
- **Purpose:** Shared free helpers that pick where units stand when sent to a point or an object, and check whether a unit already targets a mark.

## Functions

### `fuzzyArrive`

Returns a random point inside an oval around `target`, clamped to the map, so a group of units spreads out instead of stacking on one spot.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `rng` | `std::mt19937&` | Random generator, advanced by two draws. |
| `target` | `Vector2D<float>` | Center of the spread. |
| `mapSize` | `Vector2D<float>` | Upper clamp for x and y. |
| `rad` | `float` | Horizontal radius in map px. Default `groundArriveRad` (50). Vertical radius is `rad / 2`. |

**Output:** `Vector2D<float>`: a floored point with x in `[0, mapSize.x]` and y in `[0, mapSize.y]`.

**Side effects:** Advances `rng`. No other state changes.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (click move for more than one selected unit) |
| economy | `src/scripts/GameScene/main.cpp` | `main` (plank item placement near a tree) |

### `aroundRadius`

Returns the spread radius around an object's solid oval: the oval's width (`max(radius * 2, radius)` of its collider) plus `colliderRadius`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `object` | `const GameObject&` | Object whose collider is measured. |

**Output:** `float`: the spread radius in map px.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| movement | `src/scripts/Movement/arrive.cpp` | `arriveAround` |

### `arriveAround`

Returns a spot around an object's solid oval where a unit fits. It redraws with `fuzzyArrive` up to `arriveTries` (30) times while `separate` reports overlap with the object's collider, then returns the last spot drawn.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `rng` | `std::mt19937&` | Random generator, advanced by each draw. |
| `object` | `const GameObject&` | Object the unit is sent to. |
| `mapSize` | `Vector2D<float>` | Map size used to clamp each draw. |

**Output:** `Vector2D<float>`: the chosen spot. If all tries overlap, it is the last drawn spot (not checked after the final draw).

**Side effects:** Advances `rng`. No other state changes.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::moveTo` |

### `hasMark`

Checks whether a unit already has a mark as its current target or in its queued marks.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `unit` | `const npc&` | Unit to check. |
| `mark` | `Vector2D<float>` | Mark position to look for. |

**Output:** `bool`: `true` if `mark` equals the targeted mark or is in the queued marks.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::moveTo` |
