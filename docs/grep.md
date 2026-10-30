## History search

arg: `<topic> <latest N>` (default N = 2)

- dir: `histories/`
- file name: `<topic>-<YYYY-MM-DD_HHMM>.md` (see [history.md](/docs/history.md))
- match `<topic>-` as name prefix by glob, not substring (`ui` must not hit `build` or `unit`)
- newest first by modified time, take N, read those only
- no hit -> try the `alias` words of the topic (see [topic.md](/docs/topic.md)), then file content
- still no hit -> same search in the old `.claude/history/*.json` archive (snake_case names)
<!-- history: histories/ md replaced .claude/history json; `<topic>-*` glob picks the topic and its subs, ls -t orders by time across subs -->

topic with all its subs:

```bash
ls -t histories/<topic>-* | head -n 2
```

no name hit:

```bash
grep -il "<topic>" histories/*.md | xargs ls -t | head -n 2
```

latest of any topic:

```bash
ls -t histories/*.md | head -n 2
```

old archive:

```bash
cd .claude/history && ls <topic>[-_]* | sort -t- -k2 | tail -n 2
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
