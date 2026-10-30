## Where topic is used

| place | form |
| --- | --- |
| history file name | `<topic>-<YYYY-MM-DD_HHMM>.md` (see [history.md](/docs/history.md)) |
| history title | `# <topic>: ...` |
| edit comment | `// topic: why; mechanism` (see [edit.md](/docs/edit.md)) |
| plan `goal` | starts with topic |
| history search | `<topic> <latest 2>` (see [grep.md](/docs/grep.md)) |

## Who names it

- Claude picks the topic from the list, no question to user
- user names a topic in the prompt -> use that word, add to list if missing
- user renames a topic -> update list, old history files keep the old name, add old word to `alias`

## Name form

- `<topic>` or `<topic>-<sub>`, kebab-case
- `<topic>`: one word from the list
- `<sub>`: 1-2 words, kebab-case, free, narrows the topic (`netcode-lockstep`, `ui-minimap`)
- grep by `<topic>` must hit all its subs, so topic is always the first word
- old `.claude/history/` files use snake_case (`ui_minimap`); same topic, older form
<!-- topic: kebab-case to match the global histories/ convention; snake_case names stay valid only in the old archive -->

## List

### Simulation

| topic | covers | alias |
| --- | --- | --- |
| `core` | game loop, fixed tick, time, ecs / entity storage | |
| `unit` | unit data, stats, spawn, death, selection state | |
| `building` | structures, placement, production queue | |
| `movement` | steering, velocity, collision, formation | |
| `pathfinding` | nav grid, A*, flow field | |
| `combat` | attack, damage, projectile, targeting | |
| `economy` | resource, gather, cost, supply | |
| `map` | tile, terrain, map load, map editor | |
| `fog` | vision, fog of war, reveal | |
| `ai` | PVE opponent, build order, unit behavior | |

### Multiplayer

| topic | covers | alias |
| --- | --- | --- |
| `netcode` | lockstep, command sync, determinism, desync | |
| `lobby` | matchmaking, team setup (1v1 .. 4v4), ready state | |
| `steam` | Steamworks, auth, friends, achievements | |
| `replay` | command record, playback | |

### Presentation

| topic | covers | alias |
| --- | --- | --- |
| `render` | SDL3 draw, sprite batch, layers, pixel scaling | |
| `animation` | sprite sheet, frame timing, state to clip | |
| `camera` | scroll, zoom, bounds | |
| `ui` | hud, menu, minimap, command card | |
| `input` | mouse, keyboard, hotkey, box select, command issue | |
| `audio` | sfx, music, mixer | |

### Project

| topic | covers | alias |
| --- | --- | --- |
| `assets` | sprite, sound, font files, asset loading | |
| `save` | save / load, settings file | |
| `build` | CMake, dependency, compiler flag, platform | |
| `test` | unit test, sim test, benchmark | |
| `docs` | README, `docs/` | |
| `claude` | `CLAUDE.md`, `.claude/`, workflow rules | |

## Grouping rules

### Pick

1. file belongs to one system -> that topic
2. edit touches two topics -> topic of the system that owns the edited file
3. cross-system change with no owner -> topic of the reason for the change (desync fix in movement code -> `netcode`)
4. still unsure -> the topic the user's prompt was about

### New topic vs sub

- narrows a list topic -> sub (`ui-minimap`, not `minimap`)
- new topic only if: no list topic covers it, and it will have its own files
- add row to the list first, then use it
- no topic named `misc`, `fix`, `refactor`, `update`: those are actions, not topics

### Split and merge

- sub reaches its own folder under `src/` -> promote to topic, old word goes to `alias` of the new row
- two topics always edited together -> merge, dropped word goes to `alias`
- grep history by topic also greps its `alias` words

### Session with more than one topic

- one history file per topic
- plan table rows grouped by topic, one `goal` line per topic
