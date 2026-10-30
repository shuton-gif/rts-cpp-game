@project.spec.json
<!-- ui-terminal: issue 4's grill-me spec binds the work on it; the import loads project.spec.json into every session -->

## Checking the screen
- `screencapture` is blocked here and SDL's offscreen driver has no OpenGL; to see a frame, run a throwaway copy of main.cpp that calls `SDL_RenderReadPixels` + `SDL_SavePNG`, with `SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software`

## Default model
Opus 5.5 (`claude-opus-5-5`)

- main session: Opus 5.5, dont switch
- subagent `high`: opus, high effort
- subagent `low`: haiku, low effort
- no other model unless user says so
<!-- default model: main session holds the full context so it stays on the strongest model; only subagents drop to haiku for `low` edits, mapping detail in docs/plan.md -->


## Edit

read [edit.md](/docs/edit.md)

### Thinking

- think silent, dont output reasoning, options, or progress
- user doesnt care how, only explain what was done when asked
- read edit target and its callers before edit
- smallest edit that solves the prompt, no unasked refactor
- decide alone if reversible, ask only if spec is unclear and a wrong guess costs a redo
- build/test before finish, if fail fix it silent
<!-- thinking: user wants results not narration; all reasoning stays internal and output is only what After edit allows -->

### Before edit

- read the file, never edit from memory
- grep callers if a signature, header or name changes
- more than one file -> Plan first
<!-- before edit: edits are made against current disk state and known callers, so they dont break files that were not opened -->

### During edit

- match the style of the file being edited
- one topic per edit, no drive-by changes
- new `.cpp` -> add to `CMakeLists.txt` in the same edit
- no new dependency, no file delete without ok
<!-- during edit: each edit stays inside what was asked so the `<path>: <ln>` list is the full picture of what changed -->

### After edit

- dont output explanation the edit, just `<path>: <ln>`
- under edited line add comments of `<topic> <why> <mechanism after edit>`
- one comment per edit block, not per line
- old comment of the same topic is replaced, not stacked
- build after edit, fix fail silent
<!-- after edit: the comment under the edit is the explanation, so chat output stays `<path>: <ln>` and the why lives next to the code -->


## Compact

- autocompact at 650k
- keep: current task and its spec, user decisions, open questions, edited `<path>: <ln>` list, failing build/test output
- drop: file dumps, search results, tool logs, finished reasoning, anything re-readable from repo
- after compact, re-read files before edit, dont trust remembered content
- manual `/compact` follows the same keep/drop
<!-- compact: context after compact should hold only what the repo cant give back; state that is on disk is re-read instead of carried -->

### History
read [history.md](/docs/history.md)
- dir `histories/`, file name `<topic>-<YYYY-MM-DD_HHMM>.md`, timestamp = session start
- one file per topic per session, updated as that topic moves on, split if session had more than one
- write or update it when a task ends (commit, or topic done and built), not before compact
- write it with the `write-histories` skill
- history is already on disk when compact or autocompact runs, so nothing is lost if compact drops it
- session start: read the most recently modified file in `histories/`; older ones only by grep `<topic> <latest 2>`
- old `.claude/history/*.json` stay as read-only archive, grep them too when `histories/` has no hit
<!-- claude: project moved to the global my-claude-set-up conventions; history is now markdown in histories/, one file per topic per session, latest read at session start, write-histories skill writes it at task end -->


## Project files

- `CLAUDE.local.md`: personal, uncommitted rules for this project; read at session start, wins over this file
- `docs/`: lasting rules and knowledge (decisions, how-tos, specs), not only in chat
- `docs/file-defs/<topic>/<file-name>.md`: one per source file, like a header: each function's name, input, output, what it does, where used (topic + path); start from `~/.claude/templates/file-def.md`
- add or change functions in a file, or their callers change -> update its file-def in the same task
- before working in an area, read its file-defs before re-reading every source file
- source file with no file-def yet -> write it when that file is next edited
<!-- claude: global setup adds CLAUDE.local.md and file-defs; file-defs fill in as files are touched so no bulk pass is needed -->


## Plan

### Flow

read [plan.md](/docs/plan.md)

1. read prompt
2. ask spec questions
3. list files to edit
4. per file: content of edit + difficulty (`low` or `high` only)
5. `low` -> fast model, `high` -> slow model high effort
6. propose plan, wait for ok
<!-- plan flow: steps only here so they load every session; what counts as low/high, question rules and plan format are in docs/plan.md -->

### Parallelism

- subagents access level `auto`
- one subagent per file, never two on the same file
- dependent edits run in order, independent edits run parallel
<!-- parallelism: split by file so parallel edits cant collide; ordering and model mapping detail in docs/plan.md -->


## grep

read [grep.md](/docs/grep.md)
- history search arg: `<topic> <latest 2>` -> newest 2 files in histories/ with prefix `<topic>`
- code search scope: `src/` first, widen only on no hit
- filenames first, then content with line numbers on hit files only
- symbol search: headers (`.h` `.hpp`) before `.cpp`
- never search build output, `_deps`, `assets/`
<!-- grep: rules kept short so they load every session; patterns and recipes live in docs/grep.md and are read on demand -->

## Topic

read [topic.md](/docs/topic.md)

- topic word comes from the list in topic.md, never invented on the spot
- same word in history file name, edit comment, plan
- name = `<topic>` or `<topic>-<sub>` in kebab-case, first word always from the list
- nothing fits -> add a new topic to topic.md first, then use it
- edit touches two topics -> topic of the system that owns the file
<!-- topic: names are kebab-case to match the global histories/ convention; one fixed word list shared by history, comments and plan, so grep by topic finds everything -->

## gh issues/branch creation naming format
issue create --title "<issue-index>:<short description max 50 words>"
branch name <issue-index>

ex: issueId = 10 creates branch 10