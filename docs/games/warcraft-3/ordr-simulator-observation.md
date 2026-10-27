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
