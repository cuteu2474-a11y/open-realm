# ORDR 2.323[C] private-map smoke test — 2026-10-15

## Reproducible observation

The original user-supplied `ORDR_S2_2.323[C].w3x` was available in a local private workspace. Its `war3map.j` was extracted with OpenRealm's MPQ reader (not copied into GitHub).

- Private W3X SHA-256: `0794996c9bc73eaa3020f20ebc45fb37cfa64a6e98ffe8aa0dfa3b356e77bc5c`
- Extracted `war3map.j`: 2,503,677 bytes, 63,651 lines
- Custom JASS declarations: **43** (29 Dz, 12 JN, 2 EX)
- Extension native registry entries in this checkpoint: **2** (EXPauseUnit and EXSetUnitFacing); the remaining 41 require evaluation or implementation.

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

## Real UI assets in the private map

MPQ extraction confirmed:

- `war3mapImported\\ORDRTemplates.toc` includes `war3mapImported\\ORDR.fdf`.
- `UI\\LoginDialog.toc` includes `UI\\LoginDialog.fdf` and `UI\\SaveSelectDialog.fdf`.

The map uses dynamic BACKDROP, BUTTON, TEXT and SPRITE frames and custom frame mouse events. Their FDF templates are available **inside the map**, but OpenRealm still needs a compatible frame runtime and a touch-to-JASS event bridge.

## Single-player scope / ordering

1. Preserve script parser correctness (confirmed by local standalone smoke test).
2. Hook map-contained FDF/TOC definitions into the game's existing HUD frame loader.
3. Implement frame creation, text, texture, point, show/hide, and callback delivery against that UI system.
4. Implement **local** `DzSyncData` dispatch: the map uses it after local button events even in single-player; no networking/matchmaking is necessary. Do not just drop the events.
5. Confirm game `config`/`main`, first wave, combat, and input on iPad with separately supplied legitimate Warcraft III assets.

### Do not distribute

Do not check in or publish the original `.w3x`, extracted `war3map.j`, FDFs, game resources, or Warcraft III base asset archives. Reproduce locally with `tools/ordr_map_smoke.py` and the user's own files.
