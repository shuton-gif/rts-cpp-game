# agent

- **Path:** `src/scripts/AI/agent.h`, `src/scripts/AI/agent.cpp`
- **Topic:** ai
- **Purpose:** One agent per npc runs that npc's text script every frame, chaining skills (find, moveTo, pickup, use, build, ...) on named slots.

Also declared in `agent.h`: `sightRange` (1000 map px), `buildReach` (120 map px), `enum class Status { Running, Done, Failed }`, `struct Slot` (kind None/Point/Item/Tree/Wall, point, mark, thing, index), `struct World` (references to main's npcs, items, trees, walls, rng, consumed, plus mapSize and deltaTime).

## Functions

### `agent`

Constructor. Stores the npc's index.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `npcId` | `int` | index of the npc in `npcs` |

**Output:** none (constructor)

**Side effects:** sets `npcId`. `active` starts false.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (`agents.emplace_back(unit.id)`) |

### `start`

Replaces the running behavior with a new script and slots.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `script` | `const std::string&` | name of the script to run |
| `slots` | `std::map<std::string, Slot>` | named targets for the script |
| `world` | `World&` | scene lists and frame data |

**Output:** `void`

**Side effects:** calls `stop(world)` first. Sets `script`, `slots`, `active = true`. Every ground item (not held) in a slot gets `setFetcher(npcId)`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (right click order: buildWall, pickup, gatherWood, moveTo) |

### `stop`

Ends the behavior and frees reserved items.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `world` | `World&` | scene lists and frame data |

**Output:** `void`

**Side effects:** sets `active = false`. Every item in `world.items` with fetcher `npcId` and not held gets `setFetcher(-1)`. Clears `slots`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::start` |
| ai | `src/scripts/AI/agent.cpp` | `agent::call` (script call `stop`) |
| core | `src/scripts/GameScene/main.cpp` | `main` (shift right click) |

### `forget`

Drops every slot that points at an item about to be erased.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `thing` | `const item*` | item main will erase |

**Output:** `void`

**Side effects:** erases slots whose `thing` equals `thing`. Does nothing for nullptr.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (before erasing consumed planks) |

### `run`

Plays one frame of the running script from the top.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `world` | `World&` | scene lists and frame data |
| `scripts` | `const std::map<std::string, Script>&` | loaded scripts by name |

**Output:** `void`

**Side effects:** none when not active or the script is not loaded. Otherwise calls `runBlock` on the script body, which runs skills that change the scene.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (every agent, every frame) |

### `remapTrees`

Moves each tree slot to its new index after trees were erased.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `newIndex` | `const std::vector<int>&` | old tree index to new tree index |

**Output:** `void`

**Side effects:** every Tree slot's `index` becomes `newIndex[index]`, or -1 when the old index is outside the table.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (after chunk streaming erases trees) |

### `isActive`

Returns whether a behavior is running.

**Input:** none

**Output:** `bool`: `active`

**Side effects:** none

**Used in:** not called

### `getScript`

Returns the running script's name.

**Input:** none

**Output:** `const std::string&`: `script`

**Side effects:** none

**Used in:** not called

### `runBlock`

Runs the statements of one block in order.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `block` | `const std::vector<Node>&` | statements to run |
| `world` | `World&` | scene lists and frame data |

**Output:** `bool`: true if every statement was Done; false once a statement is Running or Failed, or the behavior stopped.

**Side effects:** calls skills through `call`. An if node runs its `then` or `otherwise` block: pass is `(status == Done) != negate`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::run` |
| ai | `src/scripts/AI/agent.cpp` | `agent::runBlock` (if branches) |

### `call`

Dispatches one node to the skill named by `node.call`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `node` | `const Node&` | script call with `call` and `arg` |
| `world` | `World&` | scene lists and frame data |

**Output:** `Status`: the skill's status; Done for `stop`; Failed for an unknown call.

**Side effects:** those of the called skill. `stop` calls `agent::stop`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::runBlock` |

### `valid`

Checks that a named slot still points at something usable.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `name` | `const std::string&` | slot name |
| `world` | `World&` | scene lists and frame data |

**Output:** `bool`: false if the slot is missing. Point: always true. Item: not consumed, not held by another npc, fetcher -1 or this npc. Tree: index in range and not felled. Wall: index in range; slot `wall` needs built, slot `plan` needs not built and not destroyed, other names need not destroyed.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::find` |
| ai | `src/scripts/AI/agent.cpp` | `agent::moveTo` |
| ai | `src/scripts/AI/agent.cpp` | `agent::pickup` |
| ai | `src/scripts/AI/agent.cpp` | `agent::target` |
| ai | `src/scripts/AI/agent.cpp` | `agent::use` |
| ai | `src/scripts/AI/agent.cpp` | `agent::full` |
| ai | `src/scripts/AI/agent.cpp` | `agent::build` |

### `consumed`

Checks whether an item is in `world.consumed`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `thing` | `const item*` | item to check |
| `world` | `World&` | scene lists and frame data |

**Output:** `bool`: true if `thing` is in `world.consumed`.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::held` |
| ai | `src/scripts/AI/agent.cpp` | `agent::valid` |
| ai | `src/scripts/AI/agent.cpp` | `agent::find` |
| ai | `src/scripts/AI/agent.cpp` | `agent::pickup` |

### `held`

Returns the item this npc holds, skipping consumed ones.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `world` | `World&` | scene lists and frame data |

**Output:** `item*`: first item with holder `npcId` not in `world.consumed`, or nullptr.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::holding` |
| ai | `src/scripts/AI/agent.cpp` | `agent::use` |
| ai | `src/scripts/AI/agent.cpp` | `agent::drop` |

### `object`

Returns the tree or wall a slot points at.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `slot` | `const Slot&` | slot to resolve |
| `world` | `World&` | scene lists and frame data |

**Output:** `const GameObject*`: `&world.trees[index]` for Tree, `&world.walls[index]` for Wall, else nullptr. No range check.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::position` |
| ai | `src/scripts/AI/agent.cpp` | `agent::moveTo` |

### `position`

Returns a slot's ground spot.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `slot` | `const Slot&` | slot to resolve |
| `world` | `World&` | scene lists and frame data |

**Output:** `Vector2D<float>`: Point: `slot.point`. Item: the item's pos. Tree or wall: its collider pos.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::target` |

### `beside`

Checks whether the npc is at the tree or wall in a slot.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `name` | `const std::string&` | slot name |
| `world` | `World&` | scene lists and frame data |

**Output:** `bool`: Tree: npc collider overlaps the tree's chop hitbox. Wall: npc collider overlaps the wall's hitbox, or the wall is not built, the npc is not moving, and its pos is within `buildReach` of the wall's pos. False for a missing slot or other kinds.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::moveTo` |
| ai | `src/scripts/AI/agent.cpp` | `agent::use` |
| ai | `src/scripts/AI/agent.cpp` | `agent::build` |

### `find`

Fills a slot with the nearest matching target in sight.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `name` | `const std::string&` | slot name and what to look for: `tree`, `plan`, `wall`, or an item kind |
| `world` | `World&` | scene lists and frame data |

**Output:** `Status`: Done if the slot is still valid or a target was found; Failed if nothing is within `sightRange` of the npc's feet.

**Side effects:** on Failed, erases the slot. On a find, frees the old slot item (`setFetcher(-1)` if this npc fetched it and it is not held), reserves a found item (`setFetcher(npcId)`), and writes the slot. Candidates: standing trees; unbuilt, not destroyed walls for `plan`; built walls for `wall`; else ground items of that kind, not consumed, free or fetched by this npc.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::call` |

### `holding`

Checks what the npc holds.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `name` | `const std::string&` | empty for any item, else a slot name or item kind |
| `world` | `World&` | scene lists and frame data |

**Output:** `Status`: Failed if nothing held. Done if `name` is empty, the held item is the item in slot `name`, or the held item's kind is `name`. Else Failed.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::call` |
| ai | `src/scripts/AI/agent.cpp` | `agent::use` |

### `moveTo`

Walks the npc to a slot until arrived.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `name` | `const std::string&` | slot name |
| `world` | `World&` | scene lists and frame data |

**Output:** `Status`: Failed if the slot is not valid (or has no object). Point: Running until the order is set and the npc stops, then Done. Item: Done when held by this npc or the npc is stopped within `arriveSlack`; else Running. Tree or wall: Done when `beside`; else Running.

**Side effects:** calls `npc::setDestinaion`. Point: to `slot.point` with `slot.mark`. Item: to the item's pos when the mark is not set or the npc stopped. Tree or wall: first to `arriveAround(world.rng, object, world.mapSize)`, then to the object's collider pos once stopped; when beside and moving, halts at the feet.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::call` |

### `pickup`

Takes the item in a slot.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `name` | `const std::string&` | slot name |
| `world` | `World&` | scene lists and frame data |

**Output:** `Status`: Done if already held by this npc or picked up. Failed if the slot is not valid, not an Item, or farther than `arriveSlack` from the feet.

**Side effects:** drops every other non-consumed item this npc holds at `{its x, feet y}`, then calls `item::pickUp(npcId)`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::call` |

### `target`

Aims the npc at a slot's ground spot.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `name` | `const std::string&` | slot name |
| `world` | `World&` | scene lists and frame data |

**Output:** `Status`: Done, or Failed if the slot is not valid.

**Side effects:** calls `npc::setAim(position(slot))` and `npc::faceAim()`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::call` |

### `use`

Acts with the held item.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `name` | `const std::string&` | passed to `holding` (empty, slot name, or kind) |
| `world` | `World&` | scene lists and frame data |

**Output:** `Status`: Failed if `holding(name)` is not Done. Plank: Done when delivered to slot `plan`; Failed if no valid plan slot, not beside, or `deliver()` fails. Tool: Running while `isUsing()` or `startUse()` fails; Done after starting.

**Side effects:** plank: `wall::deliver()` on the plan, pushes the plank into `world.consumed`. Tool: `item::startUse()`; if the item does not swing, `item::use(npc, nullptr)` at once.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::call` |

### `full`

Checks whether a wall slot has all its planks.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `name` | `const std::string&` | slot name |
| `world` | `World&` | scene lists and frame data |

**Output:** `Status`: Done if the slot is a valid Wall and `hasAllPlanks()`; else Failed.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::call` |

### `build`

Adds this frame's build time to a full plan the npc is beside.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `name` | `const std::string&` | slot name |
| `world` | `World&` | scene lists and frame data |

**Output:** `Status`: Done if the wall is already built or `build()` finishes it. Running while building. Failed if the slot is missing, not a Wall, out of range, not valid, not full, or not beside.

**Side effects:** calls `wall::build(world.deltaTime)`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::call` |

### `drop`

Lets go of the held item.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `world` | `World&` | scene lists and frame data |

**Output:** `Status`: always Done.

**Side effects:** if an item is held, calls `item::drop({item x, npc feet y})`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::call` |

### `kindOf` (static)

Returns an item's kind: its image name without `.png`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `thing` | `const item&` | item to name |

**Output:** `std::string`: `getImage()` with a trailing `.png` removed.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::find` |
| ai | `src/scripts/AI/agent.cpp` | `agent::holding` |
| ai | `src/scripts/AI/agent.cpp` | `agent::use` |

### `distance` (static)

Returns the distance between two points.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `a` | `Vector2D<float>` | first point |
| `b` | `Vector2D<float>` | second point |

**Output:** `float`: `std::hypot(a.x - b.x, a.y - b.y)`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::beside` |
| ai | `src/scripts/AI/agent.cpp` | `agent::find` |
| ai | `src/scripts/AI/agent.cpp` | `agent::moveTo` |
| ai | `src/scripts/AI/agent.cpp` | `agent::pickup` |

### `overlaps` (static)

Tests two ovals for overlap without moving anything.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `hitbox` | `Collider` | first oval (copy) |
| `body` | `Collider` | second oval (copy) |

**Output:** `bool`: result of `separate(hitbox, body)` on copies with `fixed = false`.

**Side effects:** none (works on copies)

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `agent::beside` |
