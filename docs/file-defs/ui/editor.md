# editor

- **Path:** `src/scripts/Editor/editor.h`, `src/scripts/Editor/editor.cpp`
- **Topic:** ui
- **Purpose:** In-game pop-up Terminal (`scriptEditor`), drawn as pixel art, with a tab bar of buttons (close, -, +) and one tab per npc script, numbered rows, syntax colors, autocomplete and save with parse check.

## Functions

### `colorLine`

Cuts one script line into colored runs: `#` to end green, `if`/`not`/`else` pink, known calls blue, text inside `(` `)` orange, the rest black.

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

Opens the editor. Keeps buffers of names it already had, takes the buffers of names parked by `-` from the closed lists, reads `<buildDir>/<name>.txt` for other new names, sorts and dedups the tabs, stays on the current tab if it still exists, then empties the closed lists.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `window` | `SDL_Window*` | window that gets text input |
| `names` | `const std::vector<std::string>&` | script names, one tab each |
| `sourceDir` | `const std::string&` | source script dir (save target) |
| `buildDir` | `const std::string&` | build script dir (read and save target) |

**Output:** `void`

**Side effects:** reads script files; replaces `names`, `buffers`; empties `closedNames`, `closedBuffers`; sets `tab`, `row`, `col` (or calls `selectTab(0)`), `window`, `sourceDir`, `buildDir`, `opened = true`; calls `SDL_StartTextInput` when `window` is set

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` |

### `scriptEditor::close`

Closes the editor, the autocomplete popup, a name being typed and any drag, and clears the hovered button and the resize cursor. Buffers and the panel's size are kept.

**Input:** none

**Output:** `void`

**Side effects:** calls `SDL_StopTextInput` when `window` is set; sets `opened = false`, `completing = false`, `naming = false`, `hover = none`, `dragging = none`; calls `setEdgeCursor(0)`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |

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

Routes one SDL event while open. While a name is being typed, text input goes to `typedName` (letters, digits, `_`, `-`, 32 at most). Otherwise text is inserted and reruns suggestions. Keys go to `handleKey`, mouse events to `handleMouse`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `event` | `const SDL_Event&` | the event |
| `density` | `float` | pixel density of the window |

**Output:** `bool`: true when the editor used the event (text, every key, mouse inside the panel); false when closed or other events

**Side effects:** edits `typedName`, or the current buffer, cursor, scroll and popup state through the helpers

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

Sets `artScale` (the largest whole art scale, at least 1, that fits 128x96 art pixels inside 80% of the viewport) and `viewportSize`. Unless the panel was resized, it starts at 128x96 art pixels centered, with its corner on a whole text pixel. After a resize it keeps its rect, pulled back on screen. Then it draws the panel and restores clip, render scale and blend mode.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | renderer |
| `viewport` | `Vector2D<float>` | viewport size in real pixels |
| `density` | `float` | pixel density of the window |

**Output:** `bool`: false on any SDL failure; true when closed or drawn

**Side effects:** sets `artScale`, `viewportSize`, `panel`; draws to the renderer; resets clip rect and restores the caller's render scale and blend mode

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

Computes the art scale (`artScale`, set by `draw`), the text cell, the row height (one glyph plus half the gap a 4 art px row leaves, never under one glyph plus 2 text px), the popup row height, the status, gutter and text positions, with the text rows starting 4 art px below the band. Also computes the line count, the widest line, the line-number digits (at least 2), the text columns, which scrollbars show (two passes, since one bar takes room from the text and can make the other needed), their track and thumb rects (thumbs sized and placed from the lines or columns on screen and `scroll` or `scrollX`), and the close, minus, plus and tab rects, all in real pixels.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `scale` | `float` | text render scale |

**Output:** `editorLayout`: the positions, sizes, counts and scrollbar rects

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawContents` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleEvent` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::dragTo` |

### `scriptEditor::keepCursorVisible`

Moves `scroll` and `scrollX` just enough to keep the cursor row inside the visible rows and the cursor column inside the visible columns.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `area` | `const editorLayout&` | layout from `layout`, for `visible` and `columns` |

**Output:** `void`

**Side effects:** sets `scroll` and `scrollX`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleEvent` |

### `scriptEditor::selectTab`

Switches to a tab (clamped) and resets cursor, scroll, sideways scroll, error line, status and popup.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `index` | `int` | tab index |

**Output:** `void`

**Side effects:** sets `tab`, `row = 0`, `col = 0`, `scroll = 0`, `scrollX = 0`, `errorLine = -1`; clears `status`; sets `completing = false`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::open` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::closeTab` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::createScript` |

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

### `scriptEditor::closeTab`

Parks the current tab for the next open: moves its name and buffer to `closedNames` and `closedBuffers`, removes the tab, then selects the tab now in its place. Does nothing with no tabs.

**Input:** none

**Output:** `void`

**Side effects:** moves `names[tab]` and `buffers[tab]` into the closed lists; erases them from `names`, `buffers`; calls `selectTab`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |

### `scriptEditor::startNaming`

Opens the name tab for a new script: clears the popup, the status and `typedName`, then sets `naming`.

**Input:** none

**Output:** `void`

**Side effects:** sets `completing = false`, `naming = true`; clears `status` and `typedName`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |

### `scriptEditor::createScript`

Makes a new script named by `typedName`. Refuses an empty name, or one already open, parked or on disk (sets `status`, the name tab stays). Otherwise writes an empty `<name>.txt` to `sourceDir` and `buildDir`, appends the tab, selects it, sets `status = "created"` and `saved` so `main.cpp` loads it.

**Input:** none

**Output:** `void`

**Side effects:** writes up to two files; on success appends to `names`, `buffers`, clears `naming`, calls `selectTab`, sets `status` and `saved`; on failure sets `status` only

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleKey` |

### `scriptEditor::drawSuggestions`

Draws the autocomplete list (up to 8 rows) under the cursor line, or above it when it would pass the text area bottom, clamped inside the panel: a 1 art px navy edge and a white body, with the picked row on light grey. Its rows use `popupLine`, and its x accounts for `scrollX`.

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

Handles a key down. While a name is being typed, Escape cancels it, Enter (or keypad Enter) calls `createScript`, Backspace removes the last char of the name, and every other key is ignored. Otherwise Escape closes the popup, else the editor. Ctrl or Cmd+S saves, Ctrl or Cmd+Space opens the popup. Up/Down/Tab drive the popup when open. Otherwise arrows, Home/End, PageUp/PageDown move the cursor, and Backspace, Delete, Return, Tab edit.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `key` | `const SDL_KeyboardEvent&` | the key event |
| `density` | `float` | pixel density of the window |

**Output:** `bool`: always true (every key is consumed)

**Side effects:** edits the name being typed, or the current buffer, cursor, scroll and popup state; may save files, create a script or close the editor

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleEvent` |

### `scriptEditor::handleMouse`

Handles mouse events. While a drag runs, every mouse event goes to it: motion moves it, and the left button's release ends it. Otherwise an edge of the panel (within 1 art px, inside or outside it) shows a resize cursor, and a left press there starts a resize. Outside the panel an event only clears the hovered button. Inside, each event records the button under the mouse (close, minus, plus) as hovered. The wheel scrolls 3 rows per notch up/down and 3 columns per notch sideways, each stopping where its content ends. A left click drops a name being typed and closes the popup. A press on a scrollbar track or thumb starts a bar drag. On the tab bar, red closes the editor, - calls `closeTab`, + calls `startNaming`, and a tab switches to it. In the text it moves the cursor to the nearest row and column, counting from `scrollX`. A left press on the dark strip, off the red button, starts moving the panel.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `event` | `const SDL_Event&` | mouse motion, wheel or button event |
| `density` | `float` | pixel density of the window |

**Output:** `bool`: true when a drag runs, when the event is on a panel edge or inside the panel; false otherwise

**Side effects:** sets `hover`, `wheel`, `wheelX`, `scroll`, `scrollX`, `completing`, `naming`, `row`, `col`, `dragging`, `dragEdges`, `dragStart`, `panelStart`, `dragGrab`; sets the resize cursor through `setEdgeCursor`; calls `close`, `closeTab`, `startNaming`, `selectTab` or `dragTo`

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleEvent` |

### `scriptEditor::edgesAt`

Tells which panel edges a real-pixel point is on. A point counts when it is within 1 art px of the panel, inside or outside it.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pixel` | `Vector2D<float>` | point in real pixels |

**Output:** `int`: bits `edgeLeft` (1), `edgeRight` (2), `edgeTop` (4), `edgeBottom` (8) combined, or 0 when the point is off the edges

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |

### `scriptEditor::setEdgeCursor`

Shows the resize cursor matching the edge bits: diagonal for a corner, horizontal or vertical for one edge, the default arrow for 0. Only acts when the bits change; keeps one system cursor per shape for the whole run.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `edges` | `int` | edge bits from `edgesAt`, 0 for none |

**Output:** `void`

**Side effects:** sets `cursorEdges` when it changes; calls `SDL_SetCursor` with a cached cursor

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::close` |

### `scriptEditor::dragTo`

Applies a drag to the pointer's real-pixel position. A resize moves the dragged edges from where the drag began, keeps the opposite edges and the panel between 64x40 art px and the screen, and sets `resized`. A thumb drag turns the thumb's place on its track into `scroll` (vertical) or `scrollX` (horizontal), from 0 at the track's start to the last row or column that still fills the text at its end. With all four edge bits set, it moves the panel instead, keeping its size and staying fully on screen.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pixel` | `Vector2D<float>` | pointer position in real pixels |
| `density` | `float` | pixel density of the window |

**Output:** `void`

**Side effects:** sets `panel`, `resized` (resize) or `scroll`, `scrollX` (bar drag)

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |

### `scriptEditor::drawTabBar`

Draws the tab bar at render scale 1: 7 pixel grey bar, the red square with its darker center in the top strip's left corner (dim while hovered), each tab (selected: blue edge with cut corners and 3 white rows; others: grey edge with a dark right side), the - tile after the selected tab and the + glyph, both glyphs white at rest and light grey while hovered.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | renderer |
| `area` | `const editorLayout&` | layout from `layout` |

**Output:** `bool`: false on any SDL failure, else true

**Side effects:** sets the render draw color; fills rects on the renderer

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawContents` |

### `scriptEditor::drawScrollbars`

Draws each needed scrollbar at the art scale: a light track with a grey thumb, vertical on the right and horizontal at the bottom. When both show, the corner square where they meet is darker.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | renderer |
| `area` | `const editorLayout&` | layout from `layout`, with the track and thumb rects |

**Output:** `bool`: false on any SDL failure, else true

**Side effects:** sets the render draw color; fills rects on the renderer

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawContents` |

### `scriptEditor::drawContents`

Draws the panel at its real size, clipped to it: opaque white panel, dark strip on top, the light band under the bar, the tab bar through `drawTabBar`, and the needed scrollbars through `drawScrollbars`. Then the tab labels clipped to their tab (a name being typed shows a black caret; selected black, others #858585), 0-based `00:` line numbers in black (error row red), the white status in the strip, colored text from column `scrollX` clipped to the text area, the black cursor bar (hidden when it is left of `scrollX`, or above, below or clipped off the text area), then the popup. Clamps `scroll` and `scrollX` first.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | renderer |
| `density` | `float` | pixel density of the window |

**Output:** `bool`: false on any SDL failure, else true

**Side effects:** clamps `scroll` and `scrollX`; changes render scale, blend mode, draw color and clip rect; draws to the renderer

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
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleEvent` |

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
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::dragTo` |

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

### `lineCells` (static)

Returns the width of a line's first `col` chars in cells. A space after the first word is half a cell; every other char, including indent spaces, is one cell.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `line` | `const std::string&` | the line |
| `col` | `int` | how many chars to measure (clamped to the line) |

**Output:** `float`: width in cells

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::layout` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::keepCursorVisible` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawContents` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawSuggestions` |

### `colAtCells` (static)

Returns the column nearest a position in cells, the inverse of `lineCells`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `line` | `const std::string&` | the line |
| `target` | `float` | position in cells |

**Output:** `int`: the nearest column

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |

### `charCells` (static)

Returns one char's width in cells: 0.5 for a space after the line's first word, else 1.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `line` | `const std::string&` | the line |
| `at` | `std::size_t` | index of the char |

**Output:** `float`: width in cells

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `lineCells` |
| ui | `src/scripts/Editor/editor.cpp` | `colAtCells` |

### `snapTo` (static)

Rounds a value down to a whole multiple of `scale`, so text lands on whole text pixels.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `value` | `float` | value in real pixels |
| `scale` | `float` | text render scale |

**Output:** `float`: `floor(value / scale) * scale`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::layout` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::draw` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawContents` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawSuggestions` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::dragTo` |

### `fillArt` (static)

Sets the draw color and fills a rect given in art pixels at render scale 1. Its top-left is `x`, `y` plus `pixelX`, `pixelY` times `art`, and its size is `wide` by `high` times `art`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `renderer` | `SDL_Renderer*` | renderer |
| `x` | `float` | panel or button x in real pixels |
| `y` | `float` | panel or button y in real pixels |
| `art` | `float` | art scale (real pixels per art pixel) |
| `pixelX` | `float` | left edge in art pixels, from `x` |
| `pixelY` | `float` | top edge in art pixels, from `y` |
| `wide` | `float` | width in art pixels |
| `high` | `float` | height in art pixels |
| `color` | `SDL_Color` | fill color |

**Output:** `bool`: false on SDL failure, else true

**Side effects:** sets render draw color; fills a rect on the renderer

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawContents` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawTabBar` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawScrollbars` |

### `inRect` (static)

Tells if a real-pixel point is inside a rect. The left and top edges are inside, the right and bottom edges are outside.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `pixel` | `Vector2D<float>` | point in real pixels |
| `box` | `const SDL_FRect&` | rect in real pixels |

**Output:** `bool`: true when the point is inside `box`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::handleMouse` |

### `scriptEditor::tabLabel`

Returns the name of tab `index`, or `typedName` for the tab past the last one (the name being typed).

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `index` | `std::size_t` | tab index |

**Output:** `const std::string&`: the tab's name, or `typedName`

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::layout` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::drawContents` |

<!-- ui-terminal: the Terminal gained scrollbars and a resizable panel; this file-def lists its tab bar, button, naming, scrollbar and resize functions -->
