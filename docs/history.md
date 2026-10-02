## Naming File
/`<topic_name>-<YY-MM-DD-TTTT>.json`
<!-- history: search is always by topic; topic is now the name prefix so a glob finds it with no substring false hit -->

- dir: `.claude/history/`
- `topic_name`: `<topic>` or `<topic>_<sub>` from [topic.md](/docs/topic.md), snake_case, only `_` inside, never `-`
- first `-` splits topic from date
- `TTTT`: 24h local time `HHMM` when the summary is written
- example: `ui_minimap-26-10-01-1730.json`

## Content

```json
{
  "topic": "topic_name",
  "date": "YY-MM-DD",
  "time": "TTTT",
  "prompt": "what user asked, one line",
  "spec": ["decision user made", "constraint"],
  "edits": [
    { "path": "src/scripts/x.cpp", "ln": 42, "why": "one line" }
  ],
  "state": "done | wip | blocked",
  "next": ["what is left"],
  "open": ["unanswered question"]
}
```

## Rules

- facts only, no reasoning, no code blocks
- under 60 lines per file
- `edits` lists final state only, not every intermediate edit
- same topic on a later session: new file, dont rewrite old ones