# npc

- **Path:** `src/scripts/NPC/npc.h`, `src/scripts/NPC/npc.cpp`
- **Topic:** unit
- **Purpose:** The `npc` class: a unit's position, destinations, route, facing, selection, owner and aim, with no SDL calls.

Header constants: `maxDestinations` (3), `colliderRadius` (10), `arriveSlack` (20), `stuckLimit` (500 ms).
`enum class Facing`: `Up`, `Left`, `Right`, `Down`, `UpLeft`, `UpRight`, `DownLeft`, `DownRight`.
`pos` is the feet (center bottom of the sprite) in map px. `speed` is px per ms (0.2).

## Functions

### `npc` (constructor)

Creates an npc standing still at `pos`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `id` | `int` | npc id, stored in the public `id` |
| `pos` | `Vector2D<float>` | start feet position in map px |
| `player` | `int` | owner index, 0 = P1 (default 0) |

**Output:** none (constructor)

**Side effects:** sets `id`, `pos`, `player`; sets `targetedPos` to `pos` so the npc does not move.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (`npcs.emplace_back`) |

### `~npc`

Defaulted destructor. The npc owns no heap memory or SDL handle.

**Input:** none

**Output:** none

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| - | - | not called |

### `update`

Pops the next queued destination when arrived, then steps toward the first route waypoint or `targetedPos`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `deltaTime` | `float` | frame time in ms |

**Output:** `void`

**Side effects:** when standing with a queue, moves the front of `queuedPos` / `queuedMark` into `targetedPos` / `targetedMark` and calls `replan`. Drops route waypoints already stood on. Sets `facing` (8 directions) and `facingLeft` from the sign of dx and dy. Moves `pos` by `speed * deltaTime`; snaps onto the target and pops the waypoint when within one step.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `moveX`

Steps `pos.x` toward `targetedPos.x`, snapping onto it when within one step.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `step` | `float` | step length in px (absolute value is used) |

**Output:** `void`

**Side effects:** changes `pos.x`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| - | - | not called |

### `moveY`

Steps `pos.y` toward `targetedPos.y`, snapping onto it when within one step.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `step` | `float` | step length in px (absolute value is used) |

**Output:** `void`

**Side effects:** changes `pos.y`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| - | - | not called |

### `setDestinaion`

Gives a new order: replaces the destination and drops every queued one.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `clicked` | `Vector2D<float>` | point the npc walks to |
| `mark` | `Vector2D<float>` | point where the destination cross is drawn |

**Output:** `void`

**Side effects:** sets `targetedPos` and `targetedMark`, clears `queuedPos` and `queuedMark`, calls `replan`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::moveTo` |

### `addDestination`

Adds a destination after the current ones (shift right click), at most `maxDestinations` in total.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `clicked` | `Vector2D<float>` | point the npc walks to |
| `mark` | `Vector2D<float>` | point where the destination cross is drawn |

**Output:** `void`

**Side effects:** a standing npc with an empty queue takes it as `targetedPos` / `targetedMark` and calls `replan`. Otherwise it is pushed to `queuedPos` / `queuedMark` if `queuedPos.size() + 1 < maxDestinations`, else ignored.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `setSelected`

Sets the selection state.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `selected` | `bool` | new selection state |

**Output:** `void`

**Side effects:** sets `selected`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `pushTo`

Moves the npc to the position collision pushed it to.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pushed` | `Vector2D<float>` | new feet position from collision |

**Output:** `void`

**Side effects:** sets `pos`. If the npc was standing (not moving, empty queue) or is now within `arriveSlack` of `targetedPos`, sets `targetedPos` to `pos` and clears `route`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `setAim`

Sets `aim` to the unit vector from the feet to `point`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `point` | `Vector2D<float>` | map point to aim at |

**Output:** `void`

**Side effects:** sets `aim`. A point on the feet (zero length) keeps the old aim.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |
| ai | `src/scripts/AI/agent.cpp` | `agent::target` |

### `faceAim`

Turns the npc to the nearest of the 8 facings for its `aim` (45 degree steps, map y down).

**Input:** none

**Output:** `void`

**Side effects:** sets `facing`. Left-side facings set `facingLeft`, right-side ones clear it, straight up or down keep it.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| combat | `src/scripts/Item/bow.cpp` | `bow::use` |
| ai | `src/scripts/AI/agent.cpp` | `agent::target` |

### `needsRoute`

Tells whether the npc is moving without a path for its current destination.

**Input:** none

**Output:** `bool`: `isMoving() && !routed`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `setRoute`

Stores a path to walk, without its final point when that point is `targetedPos`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `points` | `std::vector<Vector2D<float>>` | path waypoints in walking order (from `findPath`) |

**Output:** `void`

**Side effects:** pops the last point if it equals `targetedPos`, moves the rest into `route`, sets `routed` to true, resets `stuckTime` to 0.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `getRoute`

Returns the waypoints before `targetedPos`.

**Input:** none

**Output:** `const std::vector<Vector2D<float>>&`: `route`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (white path line) |

### `replan`

Drops the current path so `needsRoute` asks for a new one.

**Input:** none

**Output:** `void`

**Side effects:** sets `routed` to false, clears `route`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| unit | `src/scripts/NPC/npc.cpp` | `npc::update` |
| unit | `src/scripts/NPC/npc.cpp` | `npc::setDestinaion` |
| unit | `src/scripts/NPC/npc.cpp` | `npc::addDestination` |
| unit | `src/scripts/NPC/npc.cpp` | `npc::trackProgress` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `trackProgress`

Counts time spent barely moving and replans after `stuckLimit` ms.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `before` | `Vector2D<float>` | feet position before this frame |
| `deltaTime` | `float` | frame time in ms |

**Output:** `bool`: true when the npc was stuck for `stuckLimit` ms and replanned

**Side effects:** standing resets `stuckTime`. A frame moving under a quarter of `speed * deltaTime` adds `deltaTime` to `stuckTime`, any other frame resets it. Reaching `stuckLimit` resets `stuckTime` and calls `replan`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `getCollider`

Returns the solid oval at the feet.

**Input:** none

**Output:** `Collider`: `{pos, colliderRadius, false}`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |
| ai | `src/scripts/AI/agent.cpp` | `agent::beside` |

### `getPos`

Returns the feet position in map px.

**Input:** none

**Output:** `Vector2D<float>`: `pos`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `npcRect` |
| core | `src/scripts/GameScene/main.cpp` | `main` |
| combat | `src/scripts/Item/bow.cpp` | `bow::use` |
| ai | `src/scripts/AI/agent.cpp` | `agent::beside` |
| ai | `src/scripts/AI/agent.cpp` | `agent::find` |
| ai | `src/scripts/AI/agent.cpp` | `agent::moveTo` |
| ai | `src/scripts/AI/agent.cpp` | `agent::pickup` |
| ai | `src/scripts/AI/agent.cpp` | `agent::drop` |

### `getSize`

Returns the sprite size (6 x 10).

**Input:** none

**Output:** `Vector2D<float>`: `size`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `npcRect` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `getTargetedPos`

Returns the current destination.

**Input:** none

**Output:** `Vector2D<float>`: `targetedPos`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `getQueuedPos`

Returns the destinations after `targetedPos`.

**Input:** none

**Output:** `const std::vector<Vector2D<float>>&`: `queuedPos`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| - | - | not called |

### `getTargetedMark`

Returns where the cross for the current destination is drawn.

**Input:** none

**Output:** `Vector2D<float>`: `targetedMark`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |
| movement | `src/scripts/Movement/arrive.cpp` | `hasMark` |

### `getQueuedMark`

Returns the cross points of the queued destinations.

**Input:** none

**Output:** `const std::vector<Vector2D<float>>&`: `queuedMark`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |
| movement | `src/scripts/Movement/arrive.cpp` | `hasMark` |

### `getFacing`

Returns the current facing.

**Input:** none

**Output:** `Facing`: `facing`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `spriteFor` |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `isFacingLeft`

Returns the side last faced.

**Input:** none

**Output:** `bool`: `facingLeft`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `spriteFor` |
| core | `src/scripts/GameScene/main.cpp` | `main` |
| combat | `src/scripts/Item/bow.cpp` | `bow::use` |

### `isMoving`

Tells whether the npc is away from its destination.

**Input:** none

**Output:** `bool`: `pos != targetedPos` (exact compare)

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| unit | `src/scripts/NPC/npc.cpp` | `npc::update` |
| unit | `src/scripts/NPC/npc.cpp` | `npc::addDestination` |
| unit | `src/scripts/NPC/npc.cpp` | `npc::pushTo` |
| unit | `src/scripts/NPC/npc.cpp` | `npc::needsRoute` |
| unit | `src/scripts/NPC/npc.cpp` | `npc::trackProgress` |
| core | `src/scripts/GameScene/main.cpp` | `main` |
| ai | `src/scripts/AI/agent.cpp` | `agent::beside` |
| ai | `src/scripts/AI/agent.cpp` | `agent::moveTo` |

### `isSelected`

Returns the selection state.

**Input:** none

**Output:** `bool`: `selected`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `getPlayer`

Returns the owner index (0 = P1).

**Input:** none

**Output:** `int`: `player`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `spriteFor` |

### `getAim`

Returns the aim direction.

**Input:** none

**Output:** `Vector2D<float>`: `aim`, a unit vector from the feet (right until set)

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| combat | `src/scripts/Item/bow.cpp` | `bow::use` |
