---
name: write-histories
description: Write or update the session's history notes in histories/, one markdown file per topic. Use when the user says "write histories" / "write history", and when a task ends (after a commit, or a topic is done and builds clean) per CLAUDE.md.
---

# write-histories

Format and rules live in [docs/history.md](/docs/history.md), topic words in [docs/topic.md](/docs/topic.md). Read both first; this skill is only the steps.

## Steps

1. **Pick topics.** List every topic this session edited. Name each `<topic>` or `<topic>-<sub>` in kebab-case, first word from the docs/topic.md list (add a row there first if nothing fits). One file per topic.
2. **Find this session's file.** The timestamp is when the session started (`YYYY-MM-DD_HHMM`). If `histories/<topic>-<that timestamp>.md` exists, update it; otherwise create it. Never rewrite a file from an earlier session.
3. **Get the time** if the session start is unknown: `date +%Y-%m-%d_%H%M` at the first write of the session, then reuse it for every topic.
4. **Collect edits from the repo, not memory.** `git diff` / `git show --stat` / `grep -n` the topic's comment (`// <topic>:`) to get each edited block's current `<path>:<ln>`. `Edits` lists the final state only.
5. **Write** `histories/<topic>-<YYYY-MM-DD_HHMM>.md` with the sections from docs/history.md:
   - `# <topic>: <what the user asked, one line, in their words>`
   - `- state: done | wip | blocked`
   - `## Done`, `## Decisions` (answers the user gave, interpretations told to the user), `## Edits` (`` `path:ln` ``: why), `## Next`, `## Open` (unanswered questions, things not checked, e.g. "not yet checked on screen by the user")
   - facts only, no reasoning, no code, under 60 lines
6. **Report** only the written paths as markdown links. `histories/` is gitignored, so nothing to commit.
<!-- claude: history moved from .claude/history json to histories/ markdown per the global setup; one file per topic per session, updated in place -->
