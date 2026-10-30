# axe

- **Path:** `src/scripts/Item/axe.h`, `src/scripts/Item/axe.cpp`
- **Topic:** economy
- **Purpose:** The axe item: its `use` chops a tree by `chopDamage` (1) hp, and also damages a wall by the same amount.

## Functions

### `axe::axe`

Constructs an axe item at a position with a size and an image file.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pos` | `Vector2D<float>` | Position of the item |
| `size` | `Vector2D<float>` | Size of the item |
| `image` | `std::string` | Image file name, moved into the base `item` |

**Output:** `axe`: the constructed item (constructor, no return value)

**Side effects:** none (passes `pos`, `size` and `image` to `item`)

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (`std::make_unique<axe>` when spawning `axe.png`) |

### `axe::use`

Override of `item::use`. If `target` is a `tree`, calls `tree::chop(chopDamage)`. If `target` is a `wall`, calls `wall::damage(chopDamage)`. Any other target, or `nullptr`, is ignored. The user `npc` is not read.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `user` | `npc&` | The npc holding the axe (unnamed, not used) |
| `target` | `GameObject*` | The object hit by the swing; may be `nullptr` |

**Output:** `void`

**Side effects:** Reduces the hp of the target tree (`tree::chop`) or wall (`wall::damage`) by `chopDamage` (1). Nothing else changes when the target is another type or `nullptr`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (E key loop: `thing->use(npcs[...], nullptr)`, a call with no target, so no effect for the axe) |
| core | `src/scripts/GameScene/main.cpp` | `main` (item loop: `thing.use(holder, target)` when a swing ends) |
| ai | `src/scripts/AI/agent.cpp` | `agent::use` (`held->use(world.npcs[this->npcId], nullptr)`, a call through the base `item` pointer, so no effect for the axe) |
