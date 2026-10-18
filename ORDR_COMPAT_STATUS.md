# ORDR Season 2 2.323[C] → OpenRealm iPad port

Checkpoint: **2026-10-18**. Continue on **fix/ordr-compat-jass-logging**, existing
[PR #5](https://github.com/cuteu2474-a11y/open-realm/pull/5).

**Start with [the handoff](docs/games/warcraft-3/ordr-handoff.md).**
This is not yet a verified playable iPad release.

- Prior work from `8b14d6d6` is retained: parser/native fixes, offline sync,
  logical DzFrame state, chat routing, timer safety and iPad diagnostics.
- Event capacity is now 2048. The private map needs 1054 registrations during
  startup and reaches 1066 after early timers. Save format is now 90.
- High-slot events survive save/load and run through the actual scheduler;
  both ROC and TFT regression suites cover this.
- Failed FDF parses return failure, stay out of the successful-load cache,
  and their paths are recorded in DzLoadToc diagnostics.
- LayerStyle click passthrough and SpriteScale parsing/local model rendering now
  have ROC/TFT regression coverage. The private ORDR FDF's five templates parse,
  and its 0.0005 scale reaches the local sprite draw call.
- Next: dynamic DzFrame template materialization, HUD rendering,
  touch/callback delivery, complete retail TFT data and actual iPad testing.

Keep private W3X, extracted scripts/FDFs and commercial MPQs outside Git.
Use the latest PR workflow for diagnostic IPA artifacts; old links expire.
