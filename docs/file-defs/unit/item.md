# item

- **Path:** `src/scripts/Item/item.h`, `src/scripts/Item/item.cpp`
- **Topic:** unit
- **Purpose:** Base class for items that npcs pick up, hold, drop and use. Each item type overrides `use`.

## Functions

### `item` (constructor)

Builds a GameObject with radius 0, not fixed and not solid, and stores the use duration.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pos` | `Vector2D<float>` | Start position |
| `size` | `Vector2D<float>` | Sprite size |
| `image` | `std::string` | Sprite file name, moved into GameObject |
| `useDuration` | `float` | Swing or cooldown length in ms, default 300 |

**Output:** `item`: new object (no return value)

**Side effects:** Sets `useDuration`. Calls the GameObject constructor with radius 0 and fixed `false`. Items are never solid.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (make_unique of `axe` and `bow`) |
| economy | `src/scripts/Item/axe.cpp` | `axe::axe` (base init) |
| combat | `src/scripts/Item/bow.cpp` | `bow::bow` (base init) |

### `use`

Virtual. Base version does nothing. Each item type overrides it to act on the npc using it.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `user` | `npc&` | The npc holding the item |
| `target` | `GameObject*` | Object within reach when the swing ends, `nullptr` on a miss (unnamed in base) |

**Output:** `void`

**Side effects:** None in the base class. Overrides act on the world (see `axe::use`, `bow::use`).

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (line 1160, swing end, `target` set; line 896, non-swinging item, `nullptr`) |
| ai | `src/scripts/AI/agent.cpp` | `agent::use` (line 344, non-swinging item, `nullptr`) |

### `swings`

Virtual. Returns `true` for items that swing and use at the end of the swing. The bow overrides it to return `false`.

**Input:** none

**Output:** `bool`: `true` for swinging items

**Side effects:** None.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (line 896, line 1143) |
| ai | `src/scripts/AI/agent.cpp` | `agent::use` (line 344) |
| unit | `src/scripts/Item/item.cpp` | `item::getUseOffset`, `item::getUseAngle` (internal) |

### `startUse`

Starts a swing if the item is held and not already swinging. Sets `useElapsed` to 0.

**Input:** none

**Output:** `bool`: `true` if the swing started, `false` if the item is not held or is already swinging

**Side effects:** Sets `useElapsed` to `0.0f` on success.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (line 896) |
| ai | `src/scripts/AI/agent.cpp` | `agent::use` (line 343) |

### `updateUse`

Advances the swing by `deltaTime`. Returns `true` on the one frame the swing reaches `useDuration`, then clears the swing.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `deltaTime` | `float` | Frame time in ms |

**Output:** `bool`: `true` on the frame the swing ends, else `false`

**Side effects:** Adds `deltaTime` to `useElapsed`. Sets `useElapsed` to `-1.0f` when the swing ends.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (line 1143) |

### `isUsing`

Returns whether a swing is in progress.

**Input:** none

**Output:** `bool`: `true` if `useElapsed >= 0`

**Side effects:** None.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (line 1328) |
| ai | `src/scripts/AI/agent.cpp` | `agent::use` (line 342) |
| unit | `src/scripts/Item/item.cpp` | `item::startUse`, `item::updateUse`, `item::getUseOffset`, `item::getUseAngle`, `item::drop` (internal) |

### `getUseOffset`

Returns the sprite's y shift during a swing. Eased with t squared, from `-useRise` to `useDip`.

**Input:** none

**Output:** `float`: y offset in map px, `0` when not swinging or the item does not swing

**Side effects:** None.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (lines 1168, 1170) |

### `getUseAngle`

Returns the swing turn angle in degrees, from 0 to `useTurn`. Unsigned. The caller picks the side.

**Input:** none

**Output:** `float`: degrees, `0` when not swinging or the item does not swing

**Side effects:** None.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (line 1329) |

### `pickUp`

Sets the holder to `id` and clears the fetcher.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `id` | `int` | Npc id that picks the item up |

**Output:** `void`

**Side effects:** Sets `holder` to `id` and `fetcher` to `-1`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::pickup` (line 315) |

### `drop`

Puts the item on the ground at `pos`. Clears the holder and cancels any swing without calling `use`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pos` | `Vector2D<float>` | Ground position |

**Output:** `void`

**Side effects:** Sets `holder` to `-1`, sets `pos` to `pos`, sets `useElapsed` to `-1.0f`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (line 886) |
| ai | `src/scripts/AI/agent.cpp` | `agent::pickup` (line 313), `agent::drop` (line 374) |

### `setFetcher`

Sets the id of the npc walking to pick the item up. `-1` means no one.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `id` | `int` | Fetcher npc id, or `-1` |

**Output:** `void`

**Side effects:** Sets `fetcher` to `id`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (lines 933, 939) |
| ai | `src/scripts/AI/agent.cpp` | `agent::start` (line 46), `agent::stop` (line 55), `agent::find` (lines 244, 246) |

### `moveTo`

Sets the item position. Used to make a held item follow its holder's hand.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pos` | `Vector2D<float>` | New position |

**Output:** `void`

**Side effects:** Sets `pos`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (lines 1168, 1170), every frame for held items |
| combat | `src/scripts/Item/bow.cpp` | `bow::use` (line 20) |

### `getHolder`

Returns the id of the npc holding the item.

**Input:** none

**Output:** `int`: npc id, or `-1` when not held

**Side effects:** None.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `handsFree` (line 514), `main` (lines 885, 886, 895, 896, 1142, 1168, 1170, 1314, 1321, 1323) |
| ai | `src/scripts/AI/agent.cpp` | `agent::held` (line 131), `agent::valid` (line 148), `agent::pickup` (lines 308, 312) |

### `getFetcher`

Returns the id of the npc walking to pick the item up.

**Input:** none

**Output:** `int`: npc id, or `-1` for none

**Side effects:** None.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `handsFree` (line 514) |
| ai | `src/scripts/AI/agent.cpp` | `agent::valid` (line 149), `agent::find` (lines 229, 244) |

### `isHeld`

Returns whether an npc holds the item.

**Input:** none

**Output:** `bool`: `true` if `holder >= 0`

**Side effects:** None.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (lines 885, 895, 927, 1141, 1265, 1314, 1321, 1322) |
| ai | `src/scripts/AI/agent.cpp` | `agent::start` (line 46), `agent::stop` (line 55), `agent::valid` (line 148), `agent::find` (lines 228, 244) |
