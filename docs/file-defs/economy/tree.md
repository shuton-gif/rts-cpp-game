# tree

- **Path:** `src/scripts/Tree/tree.h`, `src/scripts/Tree/tree.cpp`
- **Topic:** economy
- **Purpose:** A solid, fixed GameObject with hp that the player chops down for wood; a felled tree (hp 0) is removed from the map. Also defines `chopScale` and `trunkWidth`.

## Functions

### `tree` (constructor)

Builds a tree at full hp with a solid oval collider as wide as its trunk.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pos` | `Vector2D<float>` | Feet position, passed to `GameObject` |
| `size` | `Vector2D<float>` | Sprite size; `size.x * trunkWidth / 2` becomes the collider radius |
| `image` | `std::string` | Sprite file name, moved into `GameObject` |
| `maxHp` | `int` | Max hp; default `5`. Sets both `maxHp` and `hp` |

**Output:** none (constructor)

**Side effects:** Calls `GameObject(pos, size, size.x * trunkWidth / 2.0f, true, image)`, so the tree is solid and never pushed. Sets `maxHp` and `hp` to `maxHp`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::load` (`emplace_back`, `"tree.png"`) |
| core | `src/scripts/GameScene/main.cpp` | `main` (`emplace_back`, `"tree.png"`) |

### `getCollider`

Returns the solid oval, moved up by half its radius so its bottom sits on the trunk bottom.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | - | Const member; uses `this` |

**Output:** `Collider`: the oval with `pos.y` lowered by `radius / 2` (i.e. moved up on screen).

**Side effects:** none

**Used in**

Virtual override of `GameObject::getCollider`. Callers using a `GameObject` pointer or reference dispatch here.

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::position` (via `GameObject*`) |
| ai | `src/scripts/AI/agent.cpp` | `agent::find` (`world.trees[t].getCollider()`) |
| ai | `src/scripts/AI/agent.cpp` | `agent::moveTo` (via `GameObject*`) |
| movement | `src/scripts/Movement/arrive.cpp` | `arrive` helpers (via `GameObject&`) |
| core | `src/scripts/GameScene/main.cpp` | `main` (solids, colliders, `drawHitbox`, `canPlace`) |
| core | `src/scripts/GameScene/main.cpp` | `canPlace` (`stand.getCollider()`, static helper) |

### `chop`

Subtracts damage from hp, clamped at 0, and reports whether this chop felled the tree.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `damage` | `int` | Hp to remove |

**Output:** `bool`: `true` only when this call brings hp to 0. Returns `false` at once if the tree is already felled.

**Side effects:** Changes `hp` (never below 0).

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| economy | `src/scripts/Item/axe.cpp` | `axe::use` (`chopped->chop(chopDamage)`) |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::load` (restores hurt trees: `chop(getMaxHp() - hurt)`) |
| core | `src/scripts/GameScene/main.cpp` | `main` (arrow hit: `stand.chop(arrowDamage)`) |

### `getChopHitbox`

Returns a bigger copy of the collider for axe hits.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | - | Const member; uses `getCollider()` |

**Output:** `Collider`: same center as `getCollider()` with `radius * chopScale` (`chopScale` = 1.5).

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::beside` (`overlaps(...getChopHitbox(), unit.getCollider())`) |
| core | `src/scripts/GameScene/main.cpp` | `main` (axe swing check; debug hitbox draw) |

### `getHp`

Returns current hp.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | - | Const member |

**Output:** `int`: current `hp`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::unload` (saves hp of hurt trees) |
| core | `src/scripts/GameScene/main.cpp` | `main` (hp bar; per-frame hit age; `treeLastHp`) |

### `getMaxHp`

Returns max hp.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | - | Const member |

**Output:** `int`: `maxHp`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::unload` (hurt check), `chunkStore::load` (restore damage) |
| core | `src/scripts/GameScene/main.cpp` | `main` (hp bar fraction) |

### `isFelled`

Tells whether the tree is gone (hp at or below 0).

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | - | Const member |

**Output:** `bool`: `hp <= 0`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::valid`, `agent::find` |
| core | `src/scripts/GameScene/main.cpp` | `canPlace`, `main` (skip felled trees in hits, draws, plank drop) |
| economy | `src/scripts/Tree/tree.cpp` | `tree::chop` (early return), `tree::chop` (return value) |

<!-- Helper-free file: all functions are members; no file-static helpers. -->
