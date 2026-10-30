# script

- **Path:** `src/scripts/AI/script.h`, `src/scripts/AI/script.cpp`
- **Topic:** ai
- **Purpose:** Parse indented text npc scripts into `Node` trees and load every `.txt` script in a folder.

## Functions

### `isScriptCall`

Checks if a name is one of the fixed script calls: `find`, `holding`, `moveTo`, `pickup`, `target`, `use`, `full`, `build`, `drop`, `stop`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `name` | `const std::string&` | call name to check |

**Output:** `bool`: true when `name` matches a call in the list exactly.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/script.cpp` | `parseCall` |
| ui | `src/scripts/Editor/editor.cpp` | `colorLine` |

### `parseScript`

Parses script text into a `Script`. Builds into a local `Script` and moves it into `out` only on success, keeping `out.name`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `text` | `const std::string&` | full script source text |
| `out` | `Script&` | target script; untouched on failure |
| `error` | `std::string&` | set to `"line N: msg"` on failure |

**Output:** `bool`: true on success, false on a parse error.

**Side effects:** writes `out` on success, `error` on failure.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/script.cpp` | `loadScripts` |
| ui | `src/scripts/Editor/editor.cpp` | `scriptEditor::save` |

### `loadScripts`

Parses every regular `.txt` file in `dir` and stores each one in `scripts` under its file stem. A file that fails to parse keeps its old entry.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `dir` | `const std::string&` | folder to scan |
| `scripts` | `std::map<std::string, Script>&` | script map keyed by file stem |

**Output:** `std::vector<std::string>`: one error per failed file as `"<file>: line N: msg"`, or `{"<dir>: cannot open"}` when the folder cannot be opened.

**Side effects:** reads files from disk; inserts or replaces entries in `scripts` and sets each new script's `name` to the stem.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| core | `src/scripts/GameScene/main.cpp` | `main` (startup load) |
| core | `src/scripts/GameScene/main.cpp` | `main` (reload, line 869) |
| core | `src/scripts/GameScene/main.cpp` | `main` (reload, line 1043) |

### `fail` (static)

Writes a line-tagged parse error and returns false so callers can return it directly.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `line` | `int` | 1-based source line |
| `message` | `const std::string&` | error text |
| `error` | `std::string&` | output error string |

**Output:** `bool`: always false.

**Side effects:** sets `error` to `"line N: msg"`.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/script.cpp` | `splitLines` |
| ai | `src/scripts/AI/script.cpp` | `parseCall` |
| ai | `src/scripts/AI/script.cpp` | `parseBlock` |
| ai | `src/scripts/AI/script.cpp` | `parseScript` |

### `splitLines` (static)

Cuts text on `'\n'` into `ScriptLine`s. Drops `'\r'`, `#` comments and trailing spaces or tabs, skips empty lines, and counts leading spaces as indent.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `text` | `const std::string&` | full script source text |
| `lines` | `std::vector<ScriptLine>&` | cleaned lines are appended here |
| `error` | `std::string&` | set on failure |

**Output:** `bool`: false when a tab appears in the indent ("tabs are not allowed"), true otherwise.

**Side effects:** appends to `lines`; writes `error` on failure.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/script.cpp` | `parseScript` |

### `isNameChar` (static)

Checks if a character is a letter, digit or `_`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `c` | `char` | character to check |

**Output:** `bool`: true for `A-Z`, `a-z`, `0-9` or `_`.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/script.cpp` | `parseCall` |

### `parseCall` (static)

Parses `name(arg)` with no spaces. The name must be a known call. The arg may be empty and holds only letters, digits, `_` or `-`. Nothing may follow `)`.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `line` | `const ScriptLine&` | source line, used for the line number |
| `text` | `const std::string&` | call text to parse |
| `node` | `Node&` | gets `call`, `arg` and `line` |
| `error` | `std::string&` | set on failure |

**Output:** `bool`: true on success; false with "expected name(arg)" or "unknown call 'name'".

**Side effects:** writes `node.call`, `node.arg`, `node.line` on success; writes `error` on failure.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/script.cpp` | `parseBlock` (statement and if condition) |

### `firstWordIs` (static)

Checks if `text` starts with `word` followed by a space or the end of text.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `text` | `const std::string&` | line text |
| `word` | `const std::string&` | keyword such as `if`, `not`, `else` |

**Output:** `bool`: true when `word` is the whole first word.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/script.cpp` | `parseBlock` |

### `skipSpaces` (static)

Steps over spaces starting at an index.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `text` | `const std::string&` | text to scan |
| `at` | `std::size_t` | start index |

**Output:** `std::size_t`: index of the first non-space character at or after `at`, or `text.size()`.

**Side effects:** none

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/script.cpp` | `parseBlock` |

### `parseBlock` (static)

Reads lines at exactly `indent` until a smaller indent. An `if` (with optional `not`) takes the deeper block after it as `then`, and an `else` at the same indent right after takes the next deeper block as `otherwise`. Recurses for nested blocks.

**Input**

| Name | Type | Meaning |
| --- | --- | --- |
| `lines` | `const std::vector<ScriptLine>&` | cleaned lines |
| `pos` | `std::size_t&` | current line index, advanced as lines are read |
| `indent` | `int` | indent of this block |
| `out` | `std::vector<Node>&` | parsed nodes are appended here |
| `error` | `std::string&` | set on failure |

**Output:** `bool`: true when the block ends cleanly; false on "unexpected indent", "else without if", "if needs an indented block", "nothing may follow else", "else needs an indented block", or a `parseCall` error.

**Side effects:** advances `pos`; appends to `out`; writes `error` on failure.

**Used in**

| Topic | File | Where |
| --- | --- | --- |
| ai | `src/scripts/AI/script.cpp` | `parseBlock` (then block) |
| ai | `src/scripts/AI/script.cpp` | `parseBlock` (else block) |
| ai | `src/scripts/AI/script.cpp` | `parseScript` |
