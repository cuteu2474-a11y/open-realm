# ORDR 2.323[C] single-player native compatibility (work in progress)

Branch: `feature/ordr-singleplayer-compat`. This work is intentionally separate from `main`.

## Added in source (NOT run or built yet)

- All 43 native names explicitly declared by the map: 29 Dz, 12 JN, 2 EX have native registrations.
- The 2 EX natives forward to existing Warcraft III unit APIs.
- DZ local sync transports prefix/payload into locally invoked JASS trigger actions.
- DZ frame state: create/find/show, text, geometry/anchors, alpha, texture, font, model and callback registration. A callback-dispatch entry point is available.
- DZ key event registration and dispatch entry points. The platform input path is not yet wired.
- JN login uses local profile identity without network calls; character fields can be saved to an engine-private disk path. JN regex/count/split/replace are implemented.
- Three map-referenced stock natives absent from the original registry: `GetAbilityEffectById`, `SetTextTagPos`, `DecUnitAbilityLevel`.
- Replaced empty stock implementations for unit timed life, queued animation and weighted unit/item pools.
- Increased the stable timer arena to 8192 slots to address an observed ORDR timer-registry limit.

## Remaining integration requirements before claiming a working game

- DZ frame state has **no WC3 HUD renderer adapter**. Creating frames in JASS does not yet display them. The client UI surface and touch/click callback routing must be connected.
- DZ key-dispatch function is not wired to actual game input. Registered events are inert without that adapter.
- TOC/FDF template loading retains file references but does not yet instantiate template child frames. `DzFrameFindByName` can only find frames explicitly created in the local registry.
- Some stock Warcraft III natives outside this map's initial missing-name list remain partial/empty. In particular, audio stop/fade and animation-by-index need native subsystem work.
- JavaScript/vJASS to Lua translation has not been attempted; current code uses the original JASS runtime.
- Local JN profile serialization is not a replacement for a network account, and was not exercised on an iPad.
- The exact map configuration, renderer, input, persistence and frame lifecycle have not been tested.

## Test policy

Per the user's request, do not build, run, or device-test until the function-implementation pass is complete. Registration is **not** proof of semantic correctness.
