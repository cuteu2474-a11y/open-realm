# ORDR iPad Simulator prolonged map observation

This workflow captures real iPad Simulator screenshots after 8, 16, 45, 90, 150, and 240 seconds following a map launch request. It also records verified input asset presence, a screenshot capture index, an engine JASS log if present, and aggregate (non-raw) OS diagnostic counters.

This is a **loading and stability observation**, not a gameplay end-to-end pass:
- A green job means all commands and screenshots completed, not that the game reached difficulty selection or gameplay.
- Screenshots can show loading, a crash/home screen, or gameplay. Human review must classify them.
- The OS log signature count is heuristic and limited by available Simulator logging, not proof of crash-free operation.
- No scripted physical taps, selection, beacon, unit spawning, terrain combat, or skill behavior are implemented.
- Original MPQ game files and ORDR map remain only in private Drive and a temporary Runner Simulator container, never in the public repository or uploaded artifacts.
- Later engine fixes must not edit the original ORDR map to evade failures.

Run policy: a manual `workflow_dispatch` or an explicitly marked batch commit
containing `[ordr-simulator]` on `feature/ordr-singleplayer-compat` starts the
macOS iPad job. Unmarked incremental pushes skip the simulator job. A marked
batch commit should come only after all identified engine fixes and focused
regressions have been prepared; neither a workflow start nor a green job is
proof of actual ORDR gameplay.

## 2026-10-27: gameplay-selection observation and next renderer corrections

Run [40434262239](https://github.com/cuteu2474-a11y/open-realm/actions/runs/40434262239)
reached the original choice screen, dispatched the scripted difficulty/mode/start
selections and reached the game view without an iPad crash report. The game
view still displayed the original party-player message and substantial water
over the arena. JASS offline confirmation is distinct from the map's party
count. Movement, beacon rewards and combat remain unverified. The latest
server stage advanced only to game frame 200 while 960 wall seconds elapsed;
the longest observed client draw was 17,452 ms. The green simulator workflow
is not a gameplay-complete signal.

Code review found that `SetWaterBaseColor` invalidation unnecessarily took
the full terrain deformation rebuild path, replacing ground and cliff buffers
as well as water. It now refreshes water layers alone; the renderer's
ground-support pass also pre-collects actual walkable surfaces once per view
instead of rescanning all entities per unit. Both changes have focused
headless regressions. They have not yet been measured in the iPad simulator.
The water/terrain mismatch, party-player count, full progression and unit
controls must not be marked fixed on that basis alone. Follow the existing
policy: address and verify the known engine issues before the next single,
explicitly requested simulator run.
