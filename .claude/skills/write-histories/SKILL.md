---
name: write-histories
description: Write the session's history JSON files into .claude/history/, one per topic. Use when the user says "write histories" / "write history", and when a task ends (after a commit, or a topic is done and builds clean) per CLAUDE.md.
---

# write-histories

Format and rules live in [docs/history.md](/docs/history.md), topic words in [docs/topic.md](/docs/topic.md). Read both first; this skill is only the steps.

## Steps

1. **Pick topics.** List every topic this session edited since its last history file. Name each `<topic>` or `<topic>_<sub>`, first word from the docs/topic.md list (add a row there first if nothing fits). One file per topic.
2. **Skip done ones.** `ls .claude/history/` and drop topics that already have a file written this session after their last edit. A topic changed again after its file gets a new file; never rewrite an old one.
3. **Get the time.** `date +%y-%m-%d-%H%M` -> `YY-MM-DD-TTTT` for the name and the `date` / `time` fields.
4. **Collect edits from the repo, not memory.** `git diff` / `git show --stat` / `grep -n` the topic's comment (`// <topic>:`) to get each edited block's current `<path>` and `ln`. `edits` lists the final state only.
5. **Write** `.claude/history/<topic_name>-<YY-MM-DD-TTTT>.json` with the fields from docs/history.md:
   - `topic`, `date`, `time`
   - `prompt`: what the user asked, one line, in their words
   - `spec`: decisions the user made and constraints (asked answers, interpretations told to the user)
   - `edits`: `{ "path", "ln", "why" }`, `why` one line
   - `state`: `done` | `wip` | `blocked`
   - `next`: what is left; `open`: unanswered questions, things not checked (e.g. "not yet checked on screen by the user")
   - facts only, no reasoning, no code, under 60 lines
6. **Validate** each file: `python3 -m json.tool <file> >/dev/null`.
7. **Report** only the written paths as markdown links. `.claude/history/` is gitignored, so nothing to commit.
