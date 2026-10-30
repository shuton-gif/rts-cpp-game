# fog

- **Path:** `src/scripts/Fog/fog.cpp`
- **Topic:** fog
- **Purpose:** Per-team fog of war: keeps each team's sight and discovered cells in blocks, and a snapshot of trees and walls last seen in sight.

## Functions

### `fogMap`

Constructor. Sizes the cell grid and block grid from the map size. No block is allocated yet, so every cell starts Unknown.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `mapSize` | `Vector2D<float>` | map size in map px |

**Output:** none (constructor)

**Side effects:** sets `cellsX`, `cellsY`, `blocksX`, `blocksY`; resizes `blocks` to `blocksX * blocksY` empty entries.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `stampRow`

Sets one row of cells from `fromX` to `toX` to Sight, crossing block borders. Private.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `cy` | `int` | cell row |
| `fromX` | `int` | first cell column |
| `toX` | `int` | last cell column, inclusive |

**Output:** `void`

**Side effects:** allocates any missing block (filled Unknown) it touches; appends each touched block index to `lit` the first time it is seen in the frame; writes Sight into the block cells.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| fog | `src/scripts/Fog/fog.cpp` | `stampCircle` |

### `stampCircle`

Stamps one npc's view field: for each cell row in reach, sets the cells whose center lies inside the circle of radius `viewField` around the feet. Private.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `feet` | `Vector2D<float>` | npc feet in map px |

**Output:** `void`

**Side effects:** calls `stampRow` for each row with cells in the circle; clamps to the map's cell range.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| fog | `src/scripts/Fog/fog.cpp` | `update` |

### `update`

Advances one frame: last frame's Sight becomes Discovered, each team npc's circle is stamped, then the snapshot is refreshed: entries now in sight are dropped, and standing trees and walls in sight are recorded again.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `teamFeet` | `const std::vector<Vector2D<float>>&` | feet of each npc on this team |
| `trees` | `const std::vector<tree>&` | the game's trees list |
| `walls` | `const std::vector<wall>&` | the game's walls list |

**Output:** `void`

**Side effects:** changes Sight cells to Discovered in lit blocks; clears `lit`; stamps circles (allocating blocks); erases `seen` entries whose spot is in Sight; appends a Tree entry for each unfelled tree in sight (with its list index) and a Wall entry for each undestroyed wall in sight (with its index, `built` and `progress`).

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `cellState`

Returns the state of one cell. Cells off the map and cells in a block never made are Unknown.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `cx` | `int` | cell column |
| `cy` | `int` | cell row |

**Output:** `FogState`: Unknown, Discovered or Sight.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `drawVeil` |

### `state`

Returns the state of the cell under a map point.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `point` | `Vector2D<float>` | map px |

**Output:** `FogState`: the state of the cell containing `point`, from `cellState`.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |
| ai | `src/scripts/AI/agent.cpp` | `settle` |
| ai | `src/scripts/AI/agent.cpp` | `find` |

### `getSeen`

Returns the snapshot of trees and walls last seen in sight.

**Input:** none

**Output:** `const std::vector<Seen>&`: the snapshot, valid until the next `update`.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |
| ai | `src/scripts/AI/agent.cpp` | `find` |

### `seenAt`

Finds the snapshot entry of a kind at exact feet.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `kind` | `Seen::Kind` | Tree or Wall |
| `pos` | `Vector2D<float>` | feet of the entry |

**Output:** `const Seen*`: the first matching entry, or `nullptr` if none.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/agent.cpp` | `settle` |
