# file-defs

One `.md` per source file, like a header file: each function's name, input, output, what it does, and where it's used.

- path: `docs/file-defs/<topic>/<file-name>.md`
- `<topic>`: kebab-case word from [topic.md](/docs/topic.md), the system that owns the file
- `<file-name>`: source file name without extension (`chunk.h` + `chunk.cpp` -> one `chunk.md`)
- template: `~/.claude/templates/file-def.md`
- add or change functions, or their callers change -> update the file-def in the same task
- source file with no file-def yet -> write it when that file is next edited
<!-- docs: file-defs added from the global setup; they fill in as files are touched, no bulk pass -->
