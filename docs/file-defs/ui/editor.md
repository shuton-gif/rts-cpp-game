# editor

- **Path:** `src/scripts/Editor/editor.h`, `src/scripts/Editor/editor.cpp`
- **Topic:** ui
- **Purpose:** In-game pop-up script editor (`scriptEditor`) with one tab per npc script, syntax colors, line numbers, autocomplete and save with parse check.

## Functions

### `colorLine`

Cuts one script line into colored runs: `#` to end green, `if`/`not`/`else` pink, known calls blue, text inside `(` `)` orange, the rest white.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `line` | `const std::string&` | one script line |

**Output:** `std::vector<EditorToken>`: text runs with their color, same-color runs merged

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawContents` |

### `suggest`

Finds autocomplete matches for the `[A-Za-z0-9_-]` word before `col`. Inside an unclosed `(` it lists args, else keywords then calls. Matches are case-sensitive prefix matches.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `line` | `const std::string&` | the line being edited |
| `col` | `int` | cursor column (clamped to the line) |
| `force` | `bool` | list matches even for an empty word |
| `wordStart` | `int&` | out: column where the typed word starts |

**Output:** `std::vector<Suggestion>`: matches with insert text (keyword + space, call + `()`, arg as is), cursor step back and color; empty after `#` or for an empty word without `force`

**Side effects:** writes `wordStart`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::refreshSuggestions` |

### `scriptEditor::open`

Opens the editor. Keeps buffers of names it already had, reads `<buildDir>/<name>.txt` for new names, sorts and dedups the tabs, stays on the current tab if it still exists.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `window` | `SDL_Window*` | window that gets text input |
| `names` | `const std::vector<std::string>&` | script names, one tab each |
| `sourceDir` | `const std::string&` | source script dir (save target) |
| `buildDir` | `const std::string&` | build script dir (read and save target) |

**Output:** `void`

**Side effects:** reads script files; replaces `names`, `buffers`; sets `tab`, `row`, `col` (or calls `selectTab(0)`), `window`, `sourceDir`, `buildDir`, `opened = true`; calls `SDL_StartTextInput` when `window` is set

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `scriptEditor::close`

Closes the editor and the autocomplete popup. Buffers are kept.

**Input:** none

**Output:** `void`

**Side effects:** calls `SDL_StopTextInput` when `window` is set; sets `opened = false`, `completing = false`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |

### `scriptEditor::isOpen`

Tells if the editor is open.

**Input:** none

**Output:** `bool`: `opened`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `scriptEditor::handleEvent`

Routes one SDL event while open. Text input is inserted and reruns suggestions. Keys go to `handleKey`, mouse events to `handleMouse`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `event` | `const SDL_Event&` | the event |
| `density` | `float` | pixel density of the window |

**Output:** `bool`: true when the editor used the event (text, every key, mouse inside the panel); false when closed or other events

**Side effects:** edits the current buffer, cursor, scroll and popup state through the helpers

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `scriptEditor::takeSaved`

Returns the saved flag and clears it.

**Input:** none

**Output:** `bool`: true if a save happened since the last call

**Side effects:** sets `saved = false`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `scriptEditor::contains`

Tests a real-pixel point against the panel rect from the last draw.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pixel` | `Vector2D<float>` | point in real pixels |

**Output:** `bool`: true when open and the point is inside `panel`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |

### `scriptEditor::draw`

Sets the panel to 80% of the viewport, centered (corner on a whole text pixel), draws it, then restores clip, render scale and blend mode.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | renderer |
| `viewport` | `Vector2D<float>` | viewport size in real pixels |
| `density` | `float` | pixel density of the window |

**Output:** `bool`: false on any SDL failure; true when closed or drawn

**Side effects:** sets `panel`; draws to the renderer; resets clip rect and restores the caller's render scale and blend mode

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `scriptEditor::lines`

Returns the line buffer of the current tab.

**Input:** none

**Output:** `std::vector<std::string>&`: `buffers[tab]`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::insertText` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::newLine` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::backspace` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::deleteForward` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::save` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::refreshSuggestions` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::acceptSuggestion` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawContents` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::open` |

### `scriptEditor::layout`

Computes cell size, line height, tab row, gutter, text area, status row, visible row count and line-number digits inside the panel, in real pixels.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `scale` | `float` | text render scale |

**Output:** `editorLayout`: the positions and sizes

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawContents` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleEvent` |

### `scriptEditor::keepCursorVisible`

Moves the scroll just enough to keep the cursor row inside the visible rows.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `visible` | `int` | number of visible text rows |

**Output:** `void`

**Side effects:** sets `scroll`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleEvent` |

### `scriptEditor::selectTab`

Switches to a tab (clamped) and resets cursor, scroll, error line, status and popup.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `index` | `int` | tab index |

**Output:** `void`

**Side effects:** sets `tab`, `row = 0`, `col = 0`, `scroll = 0`, `errorLine = -1`; clears `status`; sets `completing = false`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::open` |

### `scriptEditor::insertText`

Inserts only the printable ASCII chars of `text` at the cursor and moves the cursor past them.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `text` | `const std::string&` | text to insert |

**Output:** `void`

**Side effects:** edits the current line; advances `col`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleEvent` |

### `scriptEditor::newLine`

Splits the line at the cursor. The new line starts with the current line's leading spaces, no more than the cursor column.

**Input:** none

**Output:** `void`

**Side effects:** edits the current buffer; moves `row` down one and sets `col` to the indent

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |

### `scriptEditor::backspace`

Removes the char before the cursor, or at column 0 joins the line onto the previous one.

**Input:** none

**Output:** `void`

**Side effects:** edits the current buffer; moves `col` and maybe `row`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |

### `scriptEditor::deleteForward`

Removes the char under the cursor, or at line end pulls up the next line.

**Input:** none

**Output:** `void`

**Side effects:** edits the current buffer

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |

### `scriptEditor::save`

Checks the current buffer with `parseScript` and writes it to `<sourceDir>/<name>.txt` and `<buildDir>/<name>.txt`, even when the parse fails.

**Input:** none

**Output:** `void`

**Side effects:** writes two files; sets `status` (`"saved"`, the parse error, or `"cannot write <path>"`), `errorLine` (from `"line N: ..."`, else -1) and `saved = true`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |

### `scriptEditor::refreshSuggestions`

Reruns `suggest` at the cursor and opens the popup only on a match.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `force` | `bool` | list matches even for an empty word |

**Output:** `void`

**Side effects:** sets `suggestions`, `wordStart`, `picked = 0`, `pickedScroll = 0`, `completing`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleEvent` |

### `scriptEditor::acceptSuggestion`

Replaces the typed word with the picked suggestion's insert text and steps the cursor back by its `cursorBack`.

**Input:** none

**Output:** `void`

**Side effects:** sets `completing = false`; edits the current line; sets `col`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |

### `scriptEditor::drawSuggestions`

Draws the autocomplete popup (up to 8 rows) under the cursor line, or above it when it would pass the text area bottom, clamped inside the panel, with the picked row highlighted.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | renderer |
| `scale` | `float` | text render scale |
| `area` | `const editorLayout&` | layout from `layout` |

**Output:** `bool`: false on any SDL failure; true when nothing to draw or drawn

**Side effects:** clamps `picked` and `pickedScroll`; sets and clears the render clip rect; draws to the renderer

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawContents` |

### `scriptEditor::handleKey`

Handles a key down. Escape closes the popup, else the editor. Ctrl or Cmd+S saves, Ctrl or Cmd+Space opens the popup. Up/Down/Tab drive the popup when open. Otherwise arrows, Home/End, PageUp/PageDown move the cursor, and Backspace, Delete, Return, Tab edit.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `key` | `const SDL_KeyboardEvent&` | the key event |
| `density` | `float` | pixel density of the window |

**Output:** `bool`: always true (every key is consumed)

**Side effects:** edits the current buffer, cursor, scroll and popup state; may save files or close the editor

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleEvent` |

### `scriptEditor::handleMouse`

Handles mouse events inside the panel. The wheel scrolls 3 rows per notch. A left click on a tab switches to it; in the text it moves the cursor to the nearest row and column and closes the popup.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `event` | `const SDL_Event&` | mouse motion, wheel or button event |
| `density` | `float` | pixel density of the window |

**Output:** `bool`: true when the event is inside the panel, else false

**Side effects:** sets `wheel`, `scroll`, `completing`, `row`, `col`, or switches tab

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleEvent` |

### `scriptEditor::drawContents`

Draws the panel: blended black background, current tab box, tab names, line numbers (error row red), status, colored text clipped to the text area, the cursor bar, then the popup.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | renderer |
| `density` | `float` | pixel density of the window |

**Output:** `bool`: false on any SDL failure, else true

**Side effects:** clamps `scroll`; changes render scale, blend mode, draw color and clip rect; draws to the renderer

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::draw` |

### `isWordChar` (static)

Tells if a char is `[A-Za-z0-9_]`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `c` | `char` | the char |

**Output:** `bool`: true for a word char

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `colorLine` |
| ui | `src/scripts/Editor/editor.cpp` | `suggest` |

### `pushToken` (static)

Appends a colored run, skipping empty text and merging into the previous token when the color is the same.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `tokens` | `std::vector<EditorToken>&` | token list to append to |
| `text` | `const std::string&` | run text |
| `color` | `SDL_Color` | run color |

**Output:** `void`

**Side effects:** modifies `tokens`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `colorLine` |

### `textScale` (static)

Returns the text render scale: `round(2 * density)`, at least 1.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `density` | `float` | pixel density of the window |

**Output:** `float`: whole render scale

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawContents` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleEvent` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::draw` |

### `readLines` (static)

Reads a file and splits it on `\n`, stripping `\r`. An empty or missing file gives one empty line.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `path` | `const std::string&` | file path |

**Output:** `std::vector<std::string>`: the lines, at least one

**Side effects:** reads the file

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::open` |

### `drawText` (static)

Sets the draw color and draws debug text at a real-pixel position divided by `scale`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | renderer |
| `x` | `float` | x in real pixels |
| `y` | `float` | y in real pixels |
| `scale` | `float` | current render scale |
| `text` | `const std::string&` | text to draw |
| `color` | `SDL_Color` | text color |

**Output:** `bool`: false on SDL failure, else true

**Side effects:** sets render draw color; calls `SDL_RenderDebugText`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawContents` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawSuggestions` |
