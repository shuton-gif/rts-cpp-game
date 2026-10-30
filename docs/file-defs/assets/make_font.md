# make_font

- **Path:** `tools/make_font.py`
- **Topic:** assets
- **Purpose:** Builds `assets/ui/font.ttf` from the pixel sheet `assets/ui/font.aseprite`; run by hand after a sheet edit.

## Functions

### `main`

Exports the sheet to a temporary PNG, builds the font and prints the character count.

**Input:** none

**Output:** none

**Side effects:** writes `assets/ui/font.ttf`, prints to stdout

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| assets | `tools/make_font.py` | command line entry |

### `export_sheet`

Runs the Aseprite CLI (`ASEPRITE` env var, or the default macOS path) to flatten the sheet to PNG.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `path` | `Path` | PNG file to write |

**Output:** none

**Side effects:** writes `path`, raises `CalledProcessError` if Aseprite fails

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| assets | `tools/make_font.py` | `main` |

### `build_font`

Cuts each row of the sheet into glyphs in `ROWS` order and saves the TTF; stops with an error if a row's column count does not match `ROWS`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `sheet` | `PIL.Image` | RGBA sheet image |
| `output` | `Path` | TTF file to write |

**Output:** `int`: number of mapped characters, including space

**Side effects:** writes `output`, exits on a column count mismatch

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| assets | `tools/make_font.py` | `main` |

### `columns`

Finds the runs of filled columns in one sheet row.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `alpha` | `PixelAccess` | alpha channel of the sheet |
| `width` | `int` | sheet width in pixels |
| `top` | `int` | first y of the row |
| `bottom` | `int` | last y of the row, inclusive |

**Output:** `list[tuple[int, int]]`: first and last x of each run

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| assets | `tools/make_font.py` | `build_font` |

### `glyph_outline`

Turns a glyph's pixels into a TrueType glyph, with y flipped around the row baseline.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pixels` | `set[tuple[int, int]]` | filled pixels, x relative to the glyph, y in sheet space |
| `baseline` | `int` | sheet y of the row's last pixel above the baseline |

**Output:** `Glyph`: TrueType outline in font units

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| assets | `tools/make_font.py` | `build_font` |

### `trace`

Follows the edges of a pixel set into closed loops and keeps only the corners.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pixels` | `set[tuple[int, int]]` | filled pixels |

**Output:** `list[list[tuple[int, int]]]`: one corner list per closed outline

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| assets | `tools/make_font.py` | `glyph_outline` |
