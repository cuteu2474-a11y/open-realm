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

### 2026-10-20 — physical-touch arbitration and source-asset audit

The original ORDR 2.323[C] map creates 12 neutral-passive stage-scene MDX units in `CUN` (three nonselectable headings, six difficulty choices, two mode choices and the start control). It does **not** follow that the same-stage input is a DzFrame click; keep the original player-unit SELECTED trigger route until map evidence proves otherwise. Source-map imported `Button *.mdl` models are registered, but their actual selectable bounds and full original scene background remain unverified. Do not invent a replacement image.

The iPad direct-finger event is authoritative over SDL's synthetic `SDL_TOUCH_MOUSEID` messages. Previously synthetic mouse-UP could arrive before FINGERUP, set `synthetic_up_seen`, then suppress the physical tap. We now suppress synthetic duplicates after detecting a direct touch, preserving real mouse events and two-finger panning; test both release orders. The first 5-second audit now lists **all 12** original unit HPs and selection gates, not only the three decorative headings. If input still fails, collect the new `CHOICE_MODEL_AUDIT` entries and `CHOICE_SELECT_COMMAND`/ `CHOICE_USER_SELECTION` in an on-device log and compare button HP, FOW, UI status and authoritative server event acceptance. Do not override HP without validating original UnitBalance object data.

### Original ORDR in-game offline mode on first start (2026-10-20)

Original ORDR S2 added the **`-오프라인모드`** command to use accumulated clear rewards where online save/load is unavailable; the **`-offline`** alias was subsequently added. Do not confuse this with the OpenRealm client's internet/network mode or the ORDR difficulty/mode selection UI.

In `feature/ordr-singleplayer-compat`, `G_ORDRTryAutoOfflineMode` runs only when the original ORDR JASS entry points CUO/CUW/CUN exist, the map is single-player, a local human player is connected, the game is still within its first 15 seconds, and a registered player chat trigger matches the authored offline command. It publishes **exactly one** `EVENT_PLAYER_CHAT` for the correct local player, using the existing JASS event dispatcher, without changing map-owned save bonuses, choices, or HP. The event is queued after `jass_runevents` and executed on the next game event pass. The log records `ORDR_OFFLINE_STARTUP` when queued, not proof of successful JASS mode activation; confirm in the game or further script traces.

No generated UI, invented state, or alternate background textures are introduced.

### 2026-10-20: JASS native implementation audit — verified tranche

Implementation and regression are separate from registration. `TriggerRegisterPlayerStateEvent` was a null-handle stub; it now registers the authored player state, LIMITOP and threshold, and `SetPlayerState` publishes threshold *crossings* through the original registered player-state trigger. Other internal writers of player resource stats still require a centralized event-emission audit; do not claim complete state-event coverage.

The original ORDR choice UI uses nine selectable neutral-passive models (`u000..u007,u00C`) and three unselectable heading models (`u009..u00B`), all observed with zero max HP in the physical iPad log. These noncombat models are now eligible for *selection only*, without manufacturing HP or changing damage mechanics, when the original ORDR script is loaded, their exact rawcode matches, and they are explicitly selectable in the authored object flags. Ordinary dead Warcraft units remain unselectable. The next device log will also show UnitBalance row count and embedded source size to determine the underlying object data problem.

**Unfinished function families:** `DzLoadToc` still does not materialize FDF frame templates; `DzFrameSetTexture` still only stores a path without presenting a textured UI element; `DzFrameSetScriptByCode` requires a fully wired render-to-input UI event adapter. The offline JN profile store does not implement the unavailable original online service. No completeness claim for all ORDR natives, unit systems or gameplay is justified.

`GetEventPlayerState` now returns a typed JASS player-state handle sourced from the event's coroutine-local context value. It no longer silently returns null for a valid `TriggerRegisterPlayerStateEvent` callback. As in classic JASS, callers should only use this getter while handling a player-state event; outside that event its value is undefined.
