# wall

- **Path:** `src/scripts/Wall/wall.h`, `src/scripts/Wall/wall.cpp`
- **Topic:** economy
- **Purpose:** One wall post: a plan that takes planks and build time, then a solid GameObject with hp that can be damaged and destroyed.

## Functions

### `wall`

Constructs a post plan at pos with size, and passes size.x as the GameObject radius, solid true, and image to GameObject.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pos` | `Vector2D<float>` | Feet position of the post on the map |
| `size` | `Vector2D<float>` | Size of the post; `size.x` is used as the collider radius |
| `image` | `std::string` | Sprite name, moved into the GameObject base |

**Output:** `wall`: a new post, not built, with 0 planks delivered and full hp (`wallHp`)

**Side effects:** none beyond initializing the GameObject base

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `makeWall` |

### `deliver`

Adds one plank to the plan, until it has `wallCost` planks.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `bool`: true when the plank was taken; false when the plan already has all planks (the caller keeps the plank)

**Side effects:** increments `delivered` when it returns true

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::use` |

### `hasAllPlanks`

Checks whether the plan has `wallCost` or more planks.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `bool`: true when `delivered >= wallCost`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::full`, `agent::build` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `getDelivered`

Returns how many planks the plan has.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `int`: planks delivered so far

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| | | not called |

### `build`

Adds deltaTime to the build progress, once all planks are in. Progress is capped at `wallBuildTime`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `deltaTime` | `float` | Milliseconds of building work added this call |

**Output:** `bool`: true on the call that reaches `wallBuildTime`; false otherwise, including when already built or planks are missing

**Side effects:** changes `progress` (capped at `wallBuildTime`); sets `built` to true when it finishes

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::build` |

### `getProgress`

Returns build progress as a fraction.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `float`: `progress / wallBuildTime`, from 0 to 1

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `damage`

Takes `amount` off hp, never below 0. Only a built, standing wall takes damage.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `amount` | `int` | Hp to remove |

**Output:** `bool`: true when this hit destroys the wall; false otherwise, including when not built or already destroyed

**Side effects:** changes `hp` when the wall is built and not destroyed

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| economy | `src/scripts/Item/axe.cpp` | `axe::use` |
| core | `src/scripts/GameScene/main.cpp` | `main` (arrow hits) |

### `getHitbox`

Returns the collider used for hits: the GameObject collider with its radius multiplied by `chopScale`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `Collider`: copy of the wall collider with the scaled radius

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::beside` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `getHp`

Returns current hp.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `int`: current hp

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `getMaxHp`

Returns max hp.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `int`: max hp (`wallHp`)

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `isBuilt`

Checks for a standing built wall.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `bool`: true when `built` is set and `hp > 0`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::valid`, `agent::beside`, `agent::find`, `agent::build` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `isDestroyed`

Checks for a wall that was built and lost all hp.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `bool`: true when `built` is set and `hp <= 0`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::valid` |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::load` (lambda) |
| core | `src/scripts/GameScene/main.cpp` | `snapPost`, `canPlace`, `main` |
