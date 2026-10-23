# ORDR 2.323[C] single-player native compatibility (work in progress)

Branch: `feature/ordr-singleplayer-compat`. This work is intentionally separate from `main`.

## Source implementation (device integration still incomplete)

- All 43 native names explicitly declared by the map: 29 Dz, 12 JN, 2 EX have native registrations.
- The 2 EX natives forward to existing Warcraft III unit APIs.
- DZ local sync transports prefix/payload into locally invoked JASS trigger actions.
- DZ FDF frames: TOC/FDF loading, template child-frame cloning, synthetic tag types, visual property updates, priority-ordered frame serialization and click event 1 via validated `dzframe`. Optional physical keyboard edges travel through `cl_game_keycmd`/`dzkey`; edit focus and live non-click UI events remain incomplete. See [DzAPI contract](ordr-dzapi-compat.md).
- DZ key event registration and dispatch entry points. The platform input path is not yet wired.
- JN login uses local profile identity without network calls; character fields can be saved to an engine-private disk path. JN regex/count/split/replace are implemented.
- Three map-referenced stock natives absent from the original registry: `GetAbilityEffectById`, `SetTextTagPos`, `DecUnitAbilityLevel`.
- Replaced empty stock implementations for unit timed life, queued animation and weighted unit/item pools.
- Increased the stable timer arena to 8192 slots to address an observed ORDR timer-registry limit.

## Remaining integration requirements before claiming a working game

- DZ has a WC3 HUD serializer/OnClick adapter, verified headlessly. The **actual ORDR imported art, iPad rendering and touch event delivery** still require device/map testing.
- DZ key-dispatch function is not wired to actual game input. Registered events are inert without that adapter.
- TOC/FDF parsing and cloned child lookup are implemented; the original map's actual TOC imports, template inheritance and prioritization remain untested.
- Some stock Warcraft III natives outside this map's initial missing-name list remain partial/empty. In particular, audio stop/fade still needs native subsystem work. Animation-by-index now resolves actual model sequences.
- JavaScript/vJASS to Lua translation has not been attempted; current code uses the original JASS runtime.
- Local JN profile serialization is not a replacement for a network account, and was not exercised on an iPad.
- The exact map configuration, renderer, input, persistence and frame lifecycle have not been tested.

## Test policy

The user requested implementation first and independent verification. Run headless regression tests during implementation. Successful compilation is not working iPad gameplay; registration is **not** proof of semantic correctness. See [native handoff](ordr-native-handoff.md) for verified changes, remaining functions and continuation commands.

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

**Unfinished function families:** Dz keyboard event integration, edit-field focus, frame priority/z-order, non-click UI events and on-device texture/model presentation remain to be verified or implemented. Frame templates, backdrop textures and click event 1 now use the existing HUD/UI bridge. The offline JN profile store does not implement the unavailable original online service. No completeness claim for all ORDR natives, unit systems or gameplay is justified.

`GetEventPlayerState` now returns a typed JASS player-state handle sourced from the event's coroutine-local context value. It no longer silently returns null for a valid `TriggerRegisterPlayerStateEvent` callback. As in classic JASS, callers should only use this getter while handling a player-state event; outside that event its value is undefined.

### Warcraft III native fog queries — six real implementations

`IsVisibleToPlayer`, `IsLocationVisibleToPlayer`, `IsFoggedToPlayer`, `IsLocationFoggedToPlayer`, `IsMaskedToPlayer`, and `IsLocationMaskedToPlayer` previously returned a hardcoded false. They now read the canonical gameplay FOW grid (current sight vs previously explored cells), respect out-of-world points and `FogEnable(false)`, and reuse its packed grid when enabled. Their tests check visible, fogged, masked, outside world, and unexplored transitions. This fixes actual JASS-visible world-state behavior rather than adding temporary gameplay balance.

### 2026-10-20 JASS native implementation pass: additional classic game functions

Implemented the six `IssueNeutral{Immediate,Point,Target}Order{,ById}` variants by routing validated neutral-passive shop structures through the existing server order and stock engine. Implemented the string `IssueBuildOrder` variant by decoding the authored four-character unit ID and forwarding it to `G_IssueBuildOrder` (numeric variant already existed). Added real `GetUnitPointValue` and `GetUnitPointValueByType` reads from original `UnitData.points` (`upoi`). Implemented `IsUnitVisible`, `IsUnitFogged`, `IsUnitMasked`, `IsUnitRace` using real fog/race and entity metadata. These are 13 formerly hardcoded false/zero native function bodies in this change (6+1+2+3+1). The numeric/boolean routes still require full live ORDR gameplay testing.

### 2026-10-20 — 네이티브 함수 추가 구현

- 중립 상점 구매 명령은 중립 건물이 아니라 **구매자에게 비용을 청구**하며 기존 상점 재고·조건·아이템 지급·판매 이벤트를 그대로 통과하도록 수정했다.
- 그룹 생성/필터 2종과 숫자 ID 그룹 주문 4종을 실제 게임의 유닛·건물 명령 엔진에 연결했다.
- `GetItemPlayer`, `GetOrderTargetItem`은 아이템 소유자와 JASS 이벤트 대상 아이템을 실제 객체에서 조회한다.
- 전체 함수 등록표는 `api_module.c`의 약 1,289개. 등록과 실제 구현은 분리해 판단한다.

### 2026-10-22 — event routing and native lifecycle

Implemented `GetTriggerEventId`, `GetLearnedSkill`, `GetLearnedSkillLevel`,
`SetTimeOfDayScale`, `GetTimeOfDayScale`, `SetUnitAnimationByIndex`, and
`DestroyTrigger`. Enum `GetHandleId` now returns the scalar value required by
ORDR’s `GE[GetHandleId(GetTriggerEventId())]` router. Conditions, nested calls,
waits and save/load retain the original response context. The client select
command is exercised through the real frame scheduler. Full headless tests pass;
iPad touch and complete ORDR gameplay remain unverified. See the handoff.
