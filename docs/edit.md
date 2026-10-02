## Before edit

- read the file, or the range around the edit if the file is large
- after compact, read again, dont trust remembered content
- signature, header, name change -> grep callers first (see [grep.md](/docs/grep.md))
- more than one file -> plan first (see [plan.md](/docs/plan.md))

## During edit

### Scope

- smallest edit that solves the prompt
- one topic per edit
- no unasked refactor, rename, reformat, reorder of includes
- bug seen outside the topic -> leave it, dont fix, dont report unless asked

### Style

- match the file being edited: naming, braces, indent, include order
- no file to match yet -> match the nearest file in the same folder
- comment density same as the file, apart from the After edit comment

### C++

- declaration in header, definition in `.cpp`
- new `.cpp` -> add to `CMakeLists.txt` in the same edit
- header change -> every `.cpp` that includes it still compiles
- no new dependency without ok
- no file delete without ok

## After edit

### Output

- only `<path>: <ln>`, one line per edit
- `<ln>` is the first line of the edit
- no explanation, no summary, no next step
- explain only when user asks what was done

### Comment

format: `<topic> <why> <mechanism after edit>`

- placed on the line under the edit
- one comment per edit block, not per line
- one line, split to two only if over line width
- same topic edited again -> replace the old comment, dont stack
- edit removed code only -> comment on the line where it was

| part | content |
| --- | --- |
| `topic` | what system or feature, 1-3 words, same word as history topic |
| `why` | reason for the edit |
| `mechanism after edit` | how it works now, not how it was |

| file | syntax |
| --- | --- |
| `.cpp` `.h` `.hpp` | `// topic: why; mechanism` |
| `CMakeLists.txt` `.cmake` | `# topic: why; mechanism` |
| `.md` | `<!-- topic: why; mechanism -->` |
| `.json` | none, json has no comment |

example:

```cpp
unit.pos += unit.vel * FIXED_DT;
// movement: frame dt made units drift between clients; position now steps by fixed tick so sim is deterministic
```

### Verify

- build after edit
- fail -> fix, build again, no output about it
- cant fix after 3 tries -> stop, output the error
