# wall

- **Path:** `src/scripts/Wall/wall.h`, `src/scripts/Wall/wall.cpp`
- **Topic:** economy
- **Purpose:** One wall post in a group of up to `wallGroupSize`: the group shares planks, build time and hp, and all its posts stand and disappear together.
- **Types:** `WallGroup` holds the shared `delivered`, `progress`, `built`, `hp`, the placed `posts` count, the `first` post's feet and `feetSum`.

## Functions

### `wall`

Constructs a post plan at pos with size, and passes size.x as the GameObject radius, solid true, and image to GameObject.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pos` | `Vector2D<float>` | Feet position of the post on the map |
| `size` | `Vector2D<float>` | Size of the post; `size.x` is used as the collider radius |
| `image` | `std::string` | Sprite name, moved into the GameObject base |
| `group` | `std::shared_ptr<WallGroup>` | Group this post shares state with |

**Output:** `wall`: a new post that reads its state from `group`; it is not counted in the group until `join`

**Side effects:** none beyond initializing the GameObject base

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `makeWall` |

### `join`

Counts this post in its group and adds its feet to the group's sum; the first post to join becomes the group's `first`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** none

**Side effects:** changes `posts`, `feetSum` and, for the first post, `first` in the group

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (drag release) |

### `isGroupFirst`

Checks whether this post is the first placed in its group, which draws the group's bar.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `bool`: true when this post's feet equal the group's `first`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `getGroupMiddle`

Returns the average of the feet of the group's placed posts.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `Vector2D<float>`: middle of the group, in map px

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `deliver`

Adds one plank to the plan's group, until it has `wallCost` planks.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `bool`: true when the plank was taken; false when the plan already has all planks (the caller keeps the plank)

**Side effects:** increments the group's `delivered` when it returns true

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

Adds deltaTime to the group's build progress, once all planks are in. Progress is capped at `wallBuildTime`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `deltaTime` | `float` | Milliseconds of building work added this call |

**Output:** `bool`: true on the call that reaches `wallBuildTime`; false otherwise, including when already built or planks are missing

**Side effects:** changes the group's `progress` (capped at `wallBuildTime`); sets its `built` to true when it finishes, for every post

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

Takes `amount` off the group's hp, never below 0. Only a built, standing group takes damage.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `amount` | `int` | Hp to remove |

**Output:** `bool`: true when this hit destroys the whole group; false otherwise, including when not built or already destroyed

**Side effects:** changes the group's `hp` when it is built and not destroyed

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

Returns the group's current hp.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `int`: the group's current hp

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
