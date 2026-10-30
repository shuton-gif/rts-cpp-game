# chunk

- **Path:** `src/scripts/Map/chunk.h`, `src/scripts/Map/chunk.cpp`
- **Topic:** map
- **Purpose:** Streams grass clumps and trees in `chunkSize` squares near the view or an npc, rebuilding each chunk the same from the world seed and its coords.

## Functions

### `chunkStore`

Constructor. Stores the seed and look; the store starts with no chunks loaded.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `seed` | `std::uint32_t` | world seed used to seed every chunk's generator |
| `look` | `const ChunkLook&` | map size, tree size, tile size, clump scale and margin, grass sprite count |

**Output:** none (constructor)

**Side effects:** sets the `seed` and `look` members.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `update`

Frees chunks outside the keep area, compacts the trees list, then loads missing chunks of the load area. Load area is one chunk out from the view and `npcReach` around each npc. Keep area is one chunk further out.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `viewMin` | `Vector2D<float>` | top left of the view in map px |
| `viewMax` | `Vector2D<float>` | bottom right of the view in map px |
| `npcFeet` | `const std::vector<Vector2D<float>>&` | feet position of each npc |
| `npcReach` | `float` | distance around each npc whose chunks are loaded |
| `trees` | `std::vector<tree>&` | the game's trees list, edited in place |
| `walls` | `const std::vector<wall>&` | walls; new trees are not made on a standing wall post |

**Output:** `std::vector<int>`: old tree index to new index (-1 erased) when trees were erased, else empty.

**Side effects:** resizes `owner` and `local` to `trees.size()` (new entries -1); erases trees and grass of freed chunks; moves kept trees down in order; appends trees of newly loaded chunks; changes `grass` and `damaged` through `unload` and `load`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `clumps`

Lists pointers to the grass clumps of every loaded chunk.

**Input:** none

**Output:** `std::vector<const GrassClump*>`: pointers into each chunk's clumps, valid until the next `update`.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `inMap`

Tells whether chunk coords lie on the map. Coords run from 0 to `ceil(map size / chunkSize) - 1` on each axis. Private.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `cx` | `int` | chunk x coord |
| `cy` | `int` | chunk y coord |

**Output:** `bool`: true when the chunk is on the map.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::update` |

### `unload`

Frees one chunk. Stores hp of its trees below max hp by chunk and slot, marks all its trees -1 in `remap`, and drops its grass. Private.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `key` | `std::int64_t` | packed chunk key |
| `trees` | `std::vector<tree>&` | the game's trees list (read only here) |
| `remap` | `std::vector<int>&` | old index to new index; this chunk's trees are set to -1 |

**Output:** `void`

**Side effects:** writes or erases entries in `damaged`; sets entries of `remap` to -1; erases the chunk from `grass`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::update` |

### `load`

Regenerates one chunk from a `std::mt19937` seeded with the world seed and chunk coords. Draws grass clumps first, then tree spots, then makes trees. Private.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `cx` | `int` | chunk x coord |
| `cy` | `int` | chunk y coord |
| `trees` | `std::vector<tree>&` | the game's trees list; new trees are appended |
| `walls` | `const std::vector<wall>&` | spots over a standing wall post are skipped |

**Output:** `void`

**Side effects:** adds up to `clumpsPerChunk` clumps to `grass[key]`; appends trees (`"tree.png"`) to `trees` and their chunk and slot to `owner` and `local`; skips spots within `spawnClear` of the map center, over a standing wall post, or over a fixed tree (owner -1); calls `tree::chop` to restore a stored hp from `damaged`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::update` |

### `chunkKey`

Packs chunk coords into one int64. File-static.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `x` | `const int` | chunk x coord (high 32 bits) |
| `y` | `const int` | chunk y coord (low 32 bits) |

**Output:** `std::int64_t`: `(x << 32) ^ uint32(y)`.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Map/chunk.cpp` | `addArea` |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::load` |

### `overlaps`

Box overlap test; touching edges count as overlap. File-static.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `a` | `const ChunkRect&` | first box |
| `b` | `const ChunkRect&` | second box |

**Output:** `bool`: true when the boxes overlap or touch.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::load` |

### `contains`

Tells whether one box lies wholly inside another, edges included. File-static.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `outer` | `const ChunkRect&` | containing box |
| `inner` | `const ChunkRect&` | box to test |

**Output:** `bool`: true when `inner` is inside `outer`.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::load` |

### `clumpBound`

Builds a clump's bound: sprite size times `clumpScale` plus `clumpMargin`, centered on the clump. File-static.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `clump` | `const GrassClump&` | the clump |
| `look` | `const ChunkLook&` | supplies `clumpScale` and `clumpMargin` |

**Output:** `ChunkRect`: the clump bound in map px.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::load` |

### `chunkSpan`

Finds the run of chunk coords on one axis under `lo` to `hi`, clamped to the map. `first > last` means none. File-static.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `lo` | `const float` | low edge in map px |
| `hi` | `const float` | high edge in map px |
| `mapSize` | `const float` | map size on this axis |
| `first` | `int&` | out: first chunk coord |
| `last` | `int&` | out: last chunk coord |

**Output:** `void` (results in `first` and `last`)

**Side effects:** writes `first` and `last`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Map/chunk.cpp` | `addArea` |

### `addArea`

Inserts the key of every map chunk a box overlaps into a set. File-static.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `keys` | `std::set<std::int64_t>&` | set to add chunk keys to |
| `x0` | `const float` | box left in map px |
| `y0` | `const float` | box top in map px |
| `x1` | `const float` | box right in map px |
| `y1` | `const float` | box bottom in map px |
| `mapSize` | `const Vector2D<float>` | map size in map px |

**Output:** `void`

**Side effects:** inserts keys into `keys`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::update` |
