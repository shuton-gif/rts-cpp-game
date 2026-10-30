# grass

- **Path:** `src/scripts/Grass/grass.h`, `src/scripts/Grass/grass.cpp`
- **Topic:** map
- **Purpose:** Builds one random grass clump: a set of non-overlapping grass strips inside a 1 : 2 oval bound.

## Functions

### `makeGrassClump`

Builds a clump at a random oval size and fill, places its strips without overlap, and sorts them by y.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `rng` | `std::mt19937&` | Random generator; advanced by every draw. A fixed seed gives the same clump. |
| `pos` | `Vector2D<float>` | Clump center in map px. Stored as `clump.pos`. |
| `spriteCount` | `int` | Number of strip sprites; each strip picks an index in `0..spriteCount-1`. |

**Output:** `GrassClump`: `pos`, `size` (oval bound width and height in sprite px), and `strips` (each with a sprite index and an offset from the center in sprite px, sorted by `offset.y`).

**Side effects:** none beyond advancing `rng`. Uses only local data.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Map/chunk.cpp` | `chunkStore::load` |

### `randomSpot` (file-static)

Returns a random point inside the oval: random direction, random distance from the center.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `rng` | `std::mt19937&` | Random generator; advanced by two draws. |
| `width` | `const int` | Oval bound width in sprite px. |
| `height` | `const int` | Oval bound height in sprite px. |

**Output:** `Vector2D<int>`: offset from the center in sprite px, rounded.

**Side effects:** none beyond advancing `rng`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Grass/grass.cpp` | `placeStrips` |

### `placeStrips` (file-static)

Tries to place `strips` non-overlapping strip boxes, each `stripWidth` x `stripHeight`, inside the bound. Each strip gets `triesPerStrip` random spots.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `rng` | `std::mt19937&` | Random generator; advanced by the spot draws. |
| `strips` | `const int` | Number of strips to place. |
| `width` | `const int` | Oval bound width in sprite px. |
| `height` | `const int` | Oval bound height in sprite px. |
| `placed` | `std::vector<Vector2D<int>>&` | Output list of placed spots. Cleared first. |

**Output:** `bool`: `true` if all strips were placed; `false` if some strip found no free spot within its tries.

**Side effects:** clears and refills `placed`; advances `rng`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| map | `src/scripts/Grass/grass.cpp` | `makeGrassClump` |
