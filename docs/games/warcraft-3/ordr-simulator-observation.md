# ORDR iPad Simulator prolonged map observation

This workflow captures real iPad Simulator screenshots after 8, 16, 45, 90, 150, and 240 seconds following a map launch request. It also records verified input asset presence, a screenshot capture index, an engine JASS log if present, and aggregate (non-raw) OS diagnostic counters.

This is a **loading and stability observation**, not a gameplay end-to-end pass:
- A green job means all commands and screenshots completed, not that the game reached difficulty selection or gameplay.
- Screenshots can show loading, a crash/home screen, or gameplay. Human review must classify them.
- The OS log signature count is heuristic and limited by available Simulator logging, not proof of crash-free operation.
- No scripted physical taps, selection, beacon, unit spawning, terrain combat, or skill behavior are implemented.
- Original MPQ game files and ORDR map remain only in private Drive and a temporary Runner Simulator container, never in the public repository or uploaded artifacts.
- Later engine fixes must not edit the original ORDR map to evade failures.

Run policy: only a manual `workflow_dispatch` starts the macOS iPad job.
Neither ordinary nor explicitly marked pushes start a simulator. Dispatch the
single final run only after all identified engine fixes and focused regressions
have been prepared; neither a workflow start nor a green job proves actual
ORDR gameplay.

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

## 2026-10-27: original MPQ source and grouped pre-simulator fixes

The original private ORDR map was inspected **read-only**, without modifying
`war3map.j` or adding the MPQ/its imported art to the repository. All 43
custom-native declarations have registered OpenRealm API entries. Among the
standard `common.txt` natives, the original script references 317 names,
and all 317 are registered as well. Registration is not a behavioral parity
guarantee, particularly for map UI, combat, and movement.

- `BJF` runs at game time 0.02 seconds and sets `Ji[player]` only for a
  `PLAYER_SLOT_STATE_PLAYING` / `MAP_CONTROL_USER` pair. `CUn` shows the
  *party* message for **any count other than exactly one**, including zero.
  ORDR's authored W3I has four human-capable slots, so pre-handshake fallback
  to the W3I slot count disabled the local single-player helper. The ORDR
  compatibility path now resolves the actual bootstrap local human during the
  first frame, while a later disconnected slot remains vacant. Other maps
  retain normal multiplayer occupancy behavior.
- `CIh` is the map's original `-offline` chat registration. Register it as
  soon as the actual local player connects, even if slow loading missed the
  first game-time second. The original `CIg` still receives a genuine player
  chat event after the `CUX` resource reset; no map global is overwritten.
- `CUN` calls `SetWaterBaseColor(255,255,255,0)` during difficulty selection;
  `CUB` restores `(255,255,255,255)` after the choice. Depth-only water draws
  must stop when override alpha is zero. Furthermore, white/255 is the
  identity tint over the Water.slk shallow/deep color and alpha bands, not an
  instruction to replace them with fully opaque white. The W3E terrain under
  the ORDR starting lanes is only slightly below its water surface, so an
  opaque replacement obscures that otherwise intact terrain.
- The original `CRy` random-wisp beacon is registered with `BDQ(CH, CRy)`;
  `CH` is `Rect(-3584,-9504,-3328,-9216)` and `YR` is `e0IX`.
  `CRy` consumes the entering wisp, draws the original random type and
  creates its reward. Existing game tests exercise region entry, wisp
  consumption, and reward creation, but movement into **this exact** beacon
  and the actual imported unit table are still unverified in a device run.
- The map's `war3mapImported\\Background.mdx`, its root `Background.tga`,
  `war3mapImported\\ORDRTemplates.toc` and several Discord/dashboard TGAs
  are present in the original archive. Their presence alone does not confirm
  that the iPad model/UI texture binding renders them correctly.

New pre-simulator regression cases cover early player census, water alpha
and tint, and visible multiboard name-width measurement (including Warcraft
color codes). The user requested one final test/simulator batch, **after**
all known source issues are addressed; do not infer successful gameplay from
source review or a green workflow alone.

The single local `make test` attempt on 2026-10-27 did **not** complete the
native suite: this execution environment lacks `SDL2/SDL.h`, `SDL2/SDL_keycode.h`,
and linkable `libSDL2`, `EGL`, and `GL`. JASS parser fixtures and several Python
checks ran successfully before build failure, but no C/renderer unit pass is
claimed. The repository's normal dependency-provisioned CI and the final
explicitly dispatched iPad simulator must independently verify this batch.
