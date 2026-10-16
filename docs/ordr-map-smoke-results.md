# ORDR 2.323[C] private-map smoke test — 2026-10-15

## Reproducible observation

The original user-supplied `ORDR_S2_2.323[C].w3x` was available in a local private workspace. Its `war3map.j` was extracted with OpenRealm's MPQ reader (not copied into GitHub).

- Private W3X SHA-256: `0794996c9bc73eaa3020f20ebc45fb37cfa64a6e98ffe8aa0dfa3b356e77bc5c`
- Extracted `war3map.j`: 2,503,677 bytes, 63,651 lines
- Custom JASS declarations: **43** (29 Dz, 12 JN, 2 EX)
- Extension native registry entries at checkpoint 01: **7 of 43** (two EX aliases, `DzLoadToc`, and four JN string functions). Name registration alone does not establish behavior.

### Baseline unmodified engine JASS interpreter

Command: `build/bin/jass war3map.j` (local standalone interpreter).

Result:

```text
error parsing function at line 7494 near '모코코와'
Parser Error
jass: error loading 'war3map.j'
```

### Escaped-quote lexer + JASS comment-preprocessor fixes

Same private input passed to the locally compiled patched interpreter.

```text
JASS runtime error: unknown function: InitHashtable
jass: error loading 'war3map.j'
```

**Interpretation:** the parse error is resolved and execution reached a Warcraft III game native. The standalone JASS CLI intentionally does **not** provide game host natives; this is **not** proof of full engine initialization, successful map start, UI rendering, or playable gameplay.

The source changes corresponding to these fixes have been committed to `ordr-compat-01`. The exact latest GitHub branch still needs its own automated compile/test results.

## Checkpoint 02: repeated with the supplied map

The newly downloaded W3X matches the SHA-256 above. The original `war3map.j`
again parses and stops at `InitHashtable` in the standalone interpreter.
`tools/ordr_extract_native_usage.py` now exports the declared parameter types,
return types, and source line numbers alongside call frequencies. All **43**
extension declarations were recovered; the call counts match the existing
manifest exactly. Only ABI metadata is stored in Git, without script bodies or
string values.

The other supplied archive was OpenRealm source code, not retail Warcraft III
data. To distinguish engine loading from full data availability, the actual W3X
was also run against the generated **synthetic test MPQ** for 100 server frames.
This is an incomplete-data diagnostic, not a retail-data gameplay test:

- The engine opened the map archive and reached script startup.
- Required base SLKs, models/UI files, and full `Scripts/Blizzard.j` were absent.
- Script errors included `unknown function: SetPlayerAllianceStateAllyBJ` and
  `unknown function: SetAmbientDaySound`.
- The process nevertheless returned zero at the frame limit. No battle was verified.

`tools/wc3_map_audit.py --fail-on-errors` now preserves reports and returns
nonzero for collected runtime/asset diagnostics even when the process exits
zero. The same 100-frame actual-map run correctly fails this gate. A separate
temporary game-host diagnostic also stopped in real `config` at the missing
`SetPlayerAllianceStateAllyBJ`; synthetic fixture helpers must not be mistaken
for a complete retail script environment.

With legitimate full Warcraft III data and the map under that data directory:

```sh
python3 tools/wc3_map_audit.py --data '/path/to/Warcraft III' \
  --binary build/bin/openwarcraft3 --mpqtool build/bin/mpqtool \
  --loose-map '/path/to/Warcraft III/Maps/ORDR_S2_2.323[C].w3x' \
  --frames 600 --timeout 120 --jobs 1 --fail-on-errors \
  --output-dir build/ordr-map-audit
```

Even a clean bounded server run needs separate first-wave/combat assertions and
rendered iPad input verification before claiming playability.

### Runtime diagnosis with private stock helper scripts — 2026-10-16

The W3X does not contain `common.j`, `Blizzard.j`, or their `Scripts\\` paths.
Direct `mpqtool cat` lookup confirms the absence; the map's protected archive
still permits extracting `war3map.j`. (`mpqtool info` did not find that script,
so use extraction rather than metadata lookup for this archive.)

To isolate script execution from the missing helper library, a private MPQ
overlay supplied the unchanged legacy `Common.j` and `Blizzard.j` from the
[YDWE public mirror at commit c1c75149](https://github.com/actboy168/YDWE/tree/c1c75149a41df44aa3c7a4de5d9dd490bb2c058e/Development/Component/share/zh-CN/mpq/Scripts).
Their Git blob IDs are `2672d216fec4eb738f6f890bba7d18d77f875baa` and
`38feb9623b2428b6d88ae09b3815e7534a62e6d6`, respectively. They were diagnostic
inputs only; neither script nor the overlay is committed. This does not supply
the missing base game data or establish the map's exact retail patch environment.

The 100-frame run initially crashed with SIGSEGV. A private preload diagnostic
captured `TriggerEvaluate(NULL)` from `ConditionalTriggerExecute <- main`.
The map retains a null trigger without any subsequent assignment. Separately,
`Warning: Passing version to rect type` exposed a stock BJ local variable
colliding with a short map global: the VM resolved globals before locals.

Both failures now have synthetic regressions in `t_jass_map.c`:

- `wc3_jass_map.trigger_evaluate_null_returns_false` reproduced SIGSEGV before
  the fix and passes after null evaluation returns false.
- `wc3_jass_map.locals_and_parameters_shadow_globals` failed before the fix
  and passes after identifier reads and assignments resolve local scope first.

Repeating the real-map diagnostic with those fixes returned process exit zero
at 100 frames, without the SIGSEGV or the version/rect warning. The strict audit
still correctly returned **one** because runtime errors remained:

- `CreateTimer: timer registry is full` twice (`MAX_TIMERS` is 1,024 slots).
- `unimplemented native: GetAbilityEffectById`.

Required base assets also remain absent. No first wave, combat, rendered UI,
touch interaction, or iPad execution was verified. The temporary test that read
a private absolute map path was removed from automatic test discovery.

## Real UI assets in the private map

MPQ extraction confirmed:

- `war3mapImported\\ORDRTemplates.toc` includes `war3mapImported\\ORDR.fdf`.
- `UI\\LoginDialog.toc` includes `UI\\LoginDialog.fdf` and `UI\\SaveSelectDialog.fdf`.

The map uses dynamic BACKDROP, BUTTON, TEXT and SPRITE frames and custom frame mouse events. Their FDF templates are available **inside the map**. The tested frame rendering subset now exists; frame input callbacks and the touch-to-JASS event bridge remain unimplemented.

## Single-player scope / ordering

1. Preserve script parser correctness (confirmed by local standalone smoke test).
2. Hook map-contained FDF/TOC definitions into the game's existing HUD frame loader (initial `DzLoadToc` native implemented, unverified in live game).
3. Frame creation/text/texture/anchors/show-hide now have synthetic rendering tests. Implement and verify callback delivery against the same UI system.
4. **Local** `DzSyncData` dispatch now has scheduler and persistence tests. Verify its integration with actual frame input; no matchmaking is required.
5. Confirm game `config`/`main`, first wave, combat, and input on iPad with separately supplied legitimate Warcraft III assets.

### Do not distribute

Do not check in or publish the original `.w3x`, extracted `war3map.j`, FDFs, game resources, or Warcraft III base asset archives. Reproduce locally with `tools/ordr_map_smoke.py` and the user's own files.
