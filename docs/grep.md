## History search

arg: `<topic> <latest N>` (default N = 2)

- dir: `.claude/history/`
- file name: `<topic_name>-<YY-MM-DD-TTTT>.json` (see [history.md](/docs/history.md))
- match `<topic>` as name prefix by glob, not substring (`ui` must not hit `build`)
- sort by the date part (after first `-`), take the last N, read those only
- no hit -> try the `alias` words of the topic (see [topic.md](/docs/topic.md)), then file content
<!-- history: topic-first names; prefix glob picks the topic and its subs, sort on field 2 orders them by time across subs -->

topic with all its subs:

```bash
cd .claude/history && ls <topic>[-_]* | sort -t- -k2 | tail -n 2
```

one exact topic or sub:

```bash
cd .claude/history && ls <topic_name>-* | tail -n 2
```

no name hit:

```bash
cd .claude/history && grep -il "<topic>" *.json | sort -t- -k2 | tail -n 2
```

latest of any topic:

```bash
cd .claude/history && ls | sort -t- -k2 | tail -n 2
```

## Code search

### Scope order

1. `src/scripts/`
2. `src/scenes/`
3. `src/`
4. repo root (`CMakeLists.txt`, `docs/`)

stop at the first scope with a hit.

### Steps

1. filenames only (which files have it)
2. content with line numbers, on hit files only
3. read only the range around the hit, not the whole file

### File types

| looking for | glob |
| --- | --- |
| declaration, class, struct, enum | `*.h` `*.hpp` |
| definition, logic | `*.cpp` |
| build, dependency (SDL3, Steam) | `CMakeLists.txt` `*.cmake` |

### Patterns

| target | pattern |
| --- | --- |
| class / struct | `(class\|struct)\s+Name\b` |
| function definition | `\bName\s*\(` in `*.cpp` |
| member function | `\bClass::Name\s*\(` |
| include users | `#include\s+[<"].*Name` |
| enum value | `\bName\b` in `*.h` `*.hpp` |

- whole word (`\b`) by default, loosen only on no hit
- case sensitive by default, `-i` only for history topic

### Exclude

- `build/` `cmake-build-*/` `CMakeFiles/` `_deps/`
- `assets/` (binary)
- `.git/`
