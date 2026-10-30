## Naming File
`<topic>-<YYYY-MM-DD_HHMM>.md`
<!-- history: moved to the global my-claude-set-up form; topic first so a `<topic>-*` glob finds it and its subs, timestamp fixed at session start so the session keeps updating one file -->

- dir: `histories/`
- `topic`: `<topic>` or `<topic>-<sub>` from [topic.md](/docs/topic.md), kebab-case
- timestamp: 24h local `YYYY-MM-DD_HHMM` when the session started
- example: `ui-minimap-2026-10-29_1730.md`
- old `.claude/history/*.json` (snake_case, `YY-MM-DD-TTTT`) stay as read-only archive, never rewritten

## Content

```markdown
# <topic>: <one line of what user asked>

- state: done | wip | blocked

## Done
- <what changed, one line each>

## Decisions
- <decision user made, constraint>

## Edits
- `src/scripts/x.cpp:42`: <why, one line>

## Next
- <what is left>

## Open
- <unanswered question, not checked yet>
```

## Rules

- facts only, no reasoning, no code blocks
- under 60 lines per file
- `Edits` lists final state only, not every intermediate edit
- same topic, same session: update its file
- same topic, later session: new file, dont rewrite old ones

## When

- session start: read the most recently modified file in `histories/`
- task ends: after a commit, or when a topic is done and builds clean
- not "before compact": `/compact` and autocompact run with no turn before them
- user says "write histories": write now for every topic not yet written
<!-- claude: history written before compact never happened since compact gives no turn first; writing at task end keeps it on disk before any compact -->
