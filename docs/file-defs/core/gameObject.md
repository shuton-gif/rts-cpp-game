# gameObject

- **Path:** `src/scripts/GameObject/gameObject.h`, `src/scripts/GameObject/gameObject.cpp`
- **Topic:** core
- **Purpose:** Base class for placed map objects. Holds position, size, solid oval, sprite name and clicked state, with no SDL code.

## Functions

### `GameObject` (constructor)

Stores pos, size, collider radius, fixed flag and image name. The image string is moved in.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pos` | `Vector2D<float>` | Center bottom (feet) position in map px |
| `size` | `Vector2D<float>` | Sprite size |
| `colliderRadius` | `float` | Radius of the solid oval |
| `fixed` | `bool` | True if the object does not move |
| `image` | `std::string` | Sprite file name, moved into `this->image` |

**Output:** none (constructor)

**Side effects:** Sets the five protected members. None outside the object.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| economy | `src/scripts/Tree/tree.cpp` | `tree::tree` (base initializer, `fixed` true) |
| economy | `src/scripts/Wall/wall.cpp` | `wall::wall` (base initializer, `fixed` true) |
| unit | `src/scripts/Item/item.cpp` | `item::item` (base initializer, `fixed` false, radius 0) |

### `~GameObject` (virtual destructor)

Defaulted. Lets derived types run their own destructor when deleted through a `GameObject*`.

**Input:** none

**Output:** none (destructor)

**Side effects:** none. The base owns no SDL handle.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (implicit, when derived objects are destroyed) |

### `onClick` (virtual)

Click action. Empty in the base. Each type overrides it with its own action.

**Input:** none

**Output:** `void`

**Side effects:** none in the base.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (called on `stand` when the click starts on it and it was not clicked yet) |

No subclass in `src/scripts` overrides `onClick`.

### `setClicked`

Sets the clicked flag.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `clicked` | `bool` | New clicked state |

**Output:** `void`

**Side effects:** Writes `this->clicked`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (on `stand`, set to whether the object was hit) |

### `getCollider` (virtual, const)

Returns the solid oval built from the current `pos`, `colliderRadius` and `fixed`. Built on every call.

**Input:** none

**Output:** `Collider`: `{pos, colliderRadius, fixed}`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| economy | `src/scripts/Tree/tree.cpp` | `tree::getCollider` (override, calls `GameObject::getCollider`) |
| economy | `src/scripts/Tree/tree.cpp` | `tree::getChopHitbox` (`this->getCollider`) |
| economy | `src/scripts/Wall/wall.cpp` | `wall::getHitbox` (`this->getCollider`) |
| core | `src/scripts/GameScene/main.cpp` | `main` (`stand`, `candidate`, `segment` solids, and hitbox drawing) |
| core | `src/scripts/GameScene/main.cpp` | `canPlace` (`stand`, `candidate`) |
| movement | `src/scripts/Movement/arrive.cpp` | `arriveAround` (`object`) |
| ai | `src/scripts/AI/agent.cpp` | `agent::position` (`object(...)`) |
| ai | `src/scripts/AI/agent.cpp` | `agent::moveTo` (`object->getCollider`) |
| ai | `src/scripts/AI/agent.cpp` | `agent::find` (`world.trees[t]`, `segment`) |
| ai | `src/scripts/AI/agent.cpp` | `agent::beside` (`world.trees[...]` only; `segment.getHitbox` reaches it through `wall::getHitbox`) |

Calls on `npc` objects (`npc::getCollider`, a separate class not derived from `GameObject`) are not listed.

Overrides: `tree::getCollider` in `src/scripts/Tree/tree.cpp`. `wall` and `item` do not override it.

### `getPos`

Returns the feet position.

**Input:** none

**Output:** `Vector2D<float>`: copy of `pos`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `objectRect` (`object`) |
| core | `src/scripts/GameScene/main.cpp` | `snapPost` (`walls[i]`, `walls[nearest]`) |
| core | `src/scripts/GameScene/main.cpp` | `wallFootprint` (`segment`) |
| core | `src/scripts/GameScene/main.cpp` | `main` (`thing`, `trees[t]`, `walls[w]`, `items[i]`, `stand`, `segment`) |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::load` (`post` lambda over `walls`, `trees[i]`) |
| ai | `src/scripts/AI/agent.cpp` | `agent::position` (`slot.thing`) |
| ai | `src/scripts/AI/agent.cpp` | `agent::beside` (`segment`) |
| ai | `src/scripts/AI/agent.cpp` | `agent::find` (`thing`, `item` candidates) |
| ai | `src/scripts/AI/agent.cpp` | `agent::moveTo` (`slot.thing`, `object`) |
| ai | `src/scripts/AI/agent.cpp` | `agent::pickup` (`thing`, `other`) |
| ai | `src/scripts/AI/agent.cpp` | `agent::drop` (`held`) |
| combat | `src/scripts/Item/bow.cpp` | `bow::use` (`this`) |

Calls on `npc`, `arrow` and `npc`-typed holders are not listed.

### `getSize`

Returns the sprite size.

**Input:** none

**Output:** `Vector2D<float>`: copy of `size`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `objectRect` (`object`) |
| core | `src/scripts/GameScene/main.cpp` | `main` (`thing`, `items[i]`) |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::load` (`trees[i]`) |
| combat | `src/scripts/Item/bow.cpp` | `bow::use` (`this`) |

### `getImage`

Returns the sprite file name.

**Input:** none

**Output:** `const std::string&`: reference to `image`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `postOf` (`segment`) |
| core | `src/scripts/GameScene/main.cpp` | `main` (`items[picked]`, `thing`, `items[i]`) |
| ai | `src/scripts/AI/agent.cpp` | `kindOf` (`thing`) |

### `isClicked`

Returns the clicked flag.

**Input:** none

**Output:** `bool`: value of `clicked`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (`stand`, before `onClick` and in the draw loop) |
