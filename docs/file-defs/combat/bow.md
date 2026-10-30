# bow

- **Path:** `src/scripts/Item/bow.h`, `src/scripts/Item/bow.cpp`
- **Topic:** combat
- **Purpose:** A held item that shoots an arrow into the scene's arrow list instead of swinging.

## Functions

### `bow` (constructor)

Creates a bow item with the bow cooldown as its use duration and keeps a reference to the arrow list.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pos` | `Vector2D<float>` | Start position of the bow |
| `size` | `Vector2D<float>` | Size of the bow |
| `image` | `std::string` | Sprite file name |
| `arrows` | `std::vector<arrow>&` | Scene arrow list that shots are added to; must outlive the bow |

**Output:** `bow`: the constructed item (constructors return no value)

**Side effects:** Passes `bowCooldown` (800.0f) to `item` as `useDuration`. Stores `arrows` by reference.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (spawn loop, `std::make_unique<bow>`) |

### `swings`

Reports that a bow does not swing. Overrides `item::swings`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| (none) | | |

**Output:** `bool`: always `false`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (E-key handling, line 896; use-timer check, line 1143) |
| ai | `src/scripts/AI/agent.cpp` | `agent::use` (called through base `item*`) |
| unit | `src/scripts/Item/item.cpp` | `item::getUseOffset`, `item::getUseAngle` (called through base `item`; not reached for bows because they return 0 early when `swings()` is false) |

### `use`

Shoots one arrow. Turns the user to its aim, moves the bow to the hand on the side the user now faces, then adds an `arrow` to the arrow list at the bow's middle, along the user's aim, with the height above the user's feet. The `target` argument is ignored. Overrides `item::use`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `user` | `npc&` | The npc holding the bow; it faces its aim and is the shooter |
| `target` | `GameObject*` | Not used; the arrow flies on its own |

**Output:** `void`

**Side effects:** Calls `user.faceAim()`. Moves the bow with `moveTo` to `user.getPos().x` plus or minus the previous horizontal distance, depending on `user.isFacingLeft()`, at its current y. Appends one `arrow` to `arrows` (`emplace_back` with center, `user.getAim()`, and `user.getPos().y - center.y`).

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (E-key use at start, line 896, passes `nullptr`; use at end of swing, line 1160, only for items where `swings()` is true, so not for bows) |
| ai | `src/scripts/AI/agent.cpp` | `agent::use` (line 344, passes `nullptr`, only when `!held->swings()`) |
