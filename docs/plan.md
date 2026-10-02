## When to plan

- plan: more than one file, new system, change of public header, build/dependency change
- no plan, just edit: one file, typo, rename, comment, value tweak

## Flow

### 1. Read prompt

- restate the goal in one line (internal, not output)
- grep history `<topic> <latest 2>` if the topic was worked on before (see [grep.md](/docs/grep.md))

### 2. Spec questions

- ask only what changes the edit and cant be found in repo or history
- max 4 questions, all in one message, each with options and a recommended one first
- no question if a wrong guess is cheap to redo, pick the default and go
- always ask: new dependency, file delete, public header/API change, save format or netcode change

### 3. List files

- every file to create, edit, delete, as `<path>`
- include headers, `CMakeLists.txt`, and callers that break from the change
- find callers by grep before listing, dont guess

### 4. Allocate content and difficulty

per file: what changes (one line) + `low` or `high`. no middle.

| `low` | `high` |
| --- | --- |
| one file, no other file depends on the change | header or interface used by other files |
| follows a pattern already in repo | new system, no pattern in repo yet |
| boilerplate, rename, constants, comments, docs | simulation tick, pathfinding, netcode, determinism |
| `CMakeLists.txt` add source file | memory ownership, threading, lifetime |
| wrong edit is visible at compile | wrong edit compiles but breaks at runtime |
| | performance hot path (per unit, per frame) |

- unsure -> `high`
- any one `high` row matches -> `high`

### 5. Model

| difficulty | model | effort |
| --- | --- | --- |
| `low` | haiku | low |
| `high` | opus | high |

- main session stays on default model, only subagents switch

### 6. Propose plan

format:

```
goal: <one line>

| # | path | edit | difficulty | model | after |
| - | ---- | ---- | ---------- | ----- | ----- |
| 1 | src/scripts/x.h | add struct Unit | high | opus | - |
| 2 | src/scripts/x.cpp | implement Unit::tick | high | opus | 1 |
| 3 | CMakeLists.txt | add x.cpp | low | haiku | - |

verify: <build / test command>
```

- `after`: `#` of the edit that must finish first, `-` if none
- no reasoning in the plan, table only
- wait for ok, dont edit before it
- user changes the plan -> update table, propose again

## Parallelism

- subagents access level `auto`
- one subagent per file, never two on the same file
- same `after` value -> run parallel in one batch
- header before its `.cpp`, interface before its callers
- prompt to subagent holds: path, the edit, the spec decisions it needs, After edit rules from CLAUDE.md
- subagent returns only `<path>: <ln>`
- after all finish: build once in main session, fix fails there, dont respawn

## After plan runs

- output edited `<path>: <ln>` list only
- write history if session will compact (see [history.md](/docs/history.md))
