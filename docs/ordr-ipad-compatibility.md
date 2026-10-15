# ORDR Season 2 2.323[C] — iPad compatibility checkpoint

## Verified on GitHub Actions

- Branch: `ordr-compat-01`
- iPad build workflow: `ordr-compat-build.yml`
- Successful unsigned device bundle: run 38750966276, artifact `ordr-openrealm-ipad-unsigned`
- Existing iPad workflow: run 38750973245 succeeded
- Generic CI run 38750973368: build and test jobs succeeded; **EOS regressions and live multiplayer checks** failed in the step `Verify SDK and prepare restricted player config`. This is a separate environment/configuration prerequisite and does not demonstrate an ORDR regression.

## Changes in this checkpoint

- Escaped quote handling in Warcraft III JASS lexer
- Initial native aliases: `EXPauseUnit`, `EXSetUnitFacing`

## Not yet verified

- Whether `ORDR_S2_2.323[C].w3x` parses, loads, initializes, or is playable
- ORDR extended native signatures and behavior; aliases must be validated against the map script
- `DzFrame*` user interface functions, JN sync APIs, and other extension dependencies
- Warcraft III asset loading on iPad and code signing

## Reproduction steps

1. Build using `make ipad` on a supported macOS runner.
2. Obtain the unsigned `.app` from GitHub Actions (not directly installable).
3. Supply legitimately obtained Warcraft III classic data assets separately.
4. Run the map on a device/simulator with diagnostics enabled, and collect the **first** missing-native, parser, or runtime error.
5. Implement each native against verified JASS signatures and rerun the test; do not silently map functions with different signatures.

## Release gate

Do not call the port playable until map startup, first wave, unit spawning, UI interactions, save/load (if present), and an extended play session have been exercised on-device. A successful iPad compilation is only a build check.
