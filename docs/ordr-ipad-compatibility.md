# ORDR Season 2 2.323[C] — iPad offline compatibility

## Status

Work branch: `ordr-compat-02`, based on `ordr-compat-01` (`6ce77e11`).
Review: [Draft PR #3](https://github.com/cuteu2474-a11y/open-realm/pull/3).
This is a compatibility checkpoint, **not a verified playable port**.

The supplied original W3X was extracted and parsed. With synthetic base assets
and privately supplied legacy stock JASS helpers, a bounded actual-map run now
passes the previous null-trigger crash and local/global variable corruption.
It still fails the strict audit at timer exhaustion and `GetAbilityEffectById`.
See [actual-map evidence and provenance](ordr-map-smoke-results.md).

The other supplied archive contained OpenRealm source, not Warcraft III MPQs.
Full base data, first wave, combat, rendered custom UI, touch input, and on-device
iPad execution remain unverified. Private maps/scripts/assets are excluded from Git.

## Implemented and tested

- JASS/Galaxy source escapes decode once at literal creation; native string
  values retain their bytes and bounded copies do not read past their length.
- ORDR JN string patterns retain start/end anchors and complete UTF-8 final
  characters. General JN regex dialect parity is still unverified; other patterns
  use POSIX ERE behavior.
- JASS local variables and parameters shadow globals for reads, writes, and
  array access. `TriggerEvaluate(null)` returns false instead of crashing.
- `EXPauseUnit` and `EXSetUnitFacing` execute with the actual map signatures;
  tests cover pause/resume and facing in degrees.
- Four [local Dz sync natives](games/warcraft-3/ordr-sync.md) deliver queued
  payloads through real trigger conditions/actions, including nested and sleeping
  recipients and save/load.
- Fifteen [Dz frame natives](games/warcraft-3/ordr-frames.md) author actual FDF
  frames in existing console snapshots, preserving per-player state, geometry,
  anchor references, cache behavior, and reset lifetime.
- Static coverage is **26 of 43** referenced extension names. Registration is
  not a runtime compatibility guarantee. The manifest contains exact declaration
  types and line numbers, without script bodies or private strings.

## Remaining blockers

- Actual-map diagnostic: `CreateTimer: timer registry is full` (1,024 slots),
  then `GetAbilityEffectById` is unimplemented.
- Seventeen extension names remain absent, including frame input callbacks,
  focus, key events, model/priority/alignment, and JN account/storage APIs.
- Local `DzSyncData` is implemented; the touch-to-JASS frame event bridge is not.
- Full Warcraft III base MPQs are required for meaningful map/combat/render tests.
- Runtime Dz frames do not yet serialize. Saving with live custom frames is
  explicitly rejected before the destination file is opened.

Network fields, layouts, widths, flags, and protocol version are unchanged.
Game save format is **90** and JASS snapshot format **9** for sync payloads;
older formats are rejected, with no migration.

## Verification commands

```sh
python3 -m unittest discover -s tools -p 'test_ordr_*.py'
python3 tests/test_wc3_map_audit.py
make test TEST_JOBS=4
make test-wc3-engine WC3_PATTERN='wc3_ordr_sync.*'
make test-wc3-engine WC3_PATTERN='wc3_ordr_frames.*'
make test-wc3-engine WC3_PATTERN='wc3_ordr_ex.*'
python3 tools/ordr_native_audit.py tools/ordr_native_usage.json
```

The C test matcher accepts exact names and trailing-prefix `*`, not a leading
wildcard. `WC3_PATTERN='*ordr*'` selects zero tests and supplies no evidence.
The full suite includes UDP socket tests and requires a network-enabled sandbox.

## iPad build and release gate

`ordr-ipad-build.yml` runs Linux regressions and a macOS `make ipad` check for
this PR. Treat a CI run as evidence only for its exact tested commit. Earlier
successful unsigned iPad builds on checkpoint 01 do not verify the latest code.
The artifact contains `Warcraft3-iphoneos-arm64-unsigned.tar.gz`; extract it to
preserve executable permissions. Signing/provisioning and full game data are
still required before installation.

Do not call the port playable until map startup, first wave, unit spawning,
combat, UI interactions, save/load if present, and an extended play session
have been exercised on-device. Compilation alone is a build check.
