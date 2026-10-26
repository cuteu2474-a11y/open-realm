# ORDR S2 2.323[C] — DzAPI compatibility contract

Target: `feature/ordr-singleplayer-compat`. The map statically references **29 Dz natives**;
the count is from `ordr-native-audit.json`, not a claim that all implementations work.

## Source and tools

- **`tools/fdfbindgen.c`** generates compile-time C bindings for known *stock* FDF frame trees.
  Dynamic map-defined Dz frames do not have a fixed schema; load their `.toc`/`.fdf`
  from the mounted MPQ using `UI_EnsureFDF`, `UI_CloneFrameTree`, and the existing
  `UI_WriteLayout` gameplay serializer. Do not generate project-owned substitute FDF art.
- DzAPI signature and name-context behavior: [JapiNative.j](https://github.com/yefq/JapiNative/blob/master/JapiNative.j)
  and [h-lua DzAPI bindings](https://github.com/h-lua/h-lua/blob/main/depend/h-lua/lib/japi.lua).
- This is a *standalone reimplementation* of exposed API semantics, not the binary JAPI plugin
  (which is tied to retail Warcraft/Windows). Ghidra can help inspect retail semantics where
  documentation is insufficient; it does not compile or port the plugin to iOS.

## Implemented in this tranche

1. `DzLoadToc`: read a TOC file from the mounted map/filesystem; parse its FDF path lines,
   strip blank lines/comments, and load each FDF through the existing parser. Report missing
   TOC/FDF resources. `DzCreateFrame` now clones the named loaded template including nested
   children, and the child names resolve under the creator's context ID.
2. `DzCreateFrameByTagName`: use parameter **2** for the runtime name and parameter **1**
   for the frame kind. The old code incorrectly used the frame kind as the name. Instantiate
   supported types with `UI_Spawn`; inherit a loaded template when supplied. Look up the
   latest frame in repeated name/context combinations.
3. Properties (`Show`, `SetEnable`, `SetText`, `SetSize`, `SetAlpha`, `SetPoint`,
   `SetAbsolutePoint`, `ClearAllPoints`, `SetFont`, `SetTextAlignment`, `SetTexture`,
   `SetModel`) now mutate FDF-backed frames as well as JASS compatibility state.
   `DzFrameSetTexture` drives `Backdrop.Background` for BACKDROP frames.
4. Changed frames are sent through `LAYER_GAME_3` using the existing game-authored
   `svc_layout`/generic client UI protocol, *once per dirty simulation frame*.
   `DzFrameSetScriptByCode` click event 1 creates a real input descriptor; `dzframe` (legacy)
   validates a live, visible, enabled single-player frame before delivering the
   corresponding JASS UI event with the correct player context.
5. Reset Dz frame handles/callbacks, sync registrations and key registrations at map
   replacement to prevent dangling pointers into an old map's JASS VM.

## Follow-up: frame ordering and keyboard bridge

1. `DzFrameSetPriority` is now used when writing `LAYER_GAME_3`. The game walks its registered
   FDF handles in ascending **sibling** priority, breaking ties by creation order; higher
   priority comes last for both client painting and reverse-order pointer hit testing.
   It uses the same `UI_WriteFrame`/`UI_WriteStart`/`UI_WriteEnd` packet writer as the normal HUD.
   The Dz walker does **not** call the stock UI scroll-bar preparation helper; imported Dz
   scrolling controls still need a dedicated integration test.
2. `DzFrameSetScriptByCode` retains event IDs 0–31 instead of silently dropping ID 9
   (edit-box text changed). Live click 1 and edit text change 9, plus mouse enter 2,
   leave 3, release 4, press 5, wheel 6 and checkbox checked 7/unchecked 8, now
   traverse an actual pointer event → authenticated `uievent` game command → JASS
   coroutine. Event 7/8 is selected from **server-owned** checkbox toggle state,
   independent of the client's requested edge. The game validates that every
   ancestor is currently visible/enabled. `LISTBOX` and FDF-authored
   `POPUPMENU`/`GLUEPOPUPMENU` now forward **event 10** with the selected item
   index, validated against server-owned menu contents. Duplicate selection and
   invalid or hidden frame input cannot fire callbacks.
3. The generic client now forwards non-repeating physical keyboard down/up edges through
   its **opt-in** `cl_game_keycmd` (configured to `dzkey` by the Warcraft III share config).
   This reuses `clc_stringcmd` and does not change the wire schema. The game validates
   local single-player ownership, maps shared `common/game_keys.h` codes to Dz/Windows VK
   IDs (ASCII letters, arrows, F1–F12, Page/Home/End, modifiers), then invokes registered
   Dz key callbacks and triggers. Mouse buttons and keyboard text-edit mode are excluded.
   This is **physical keyboard coverage**, not an on-screen iPad virtual-keyboard solution.
4. `DzFrameSetFocus(frame, true)` now keeps one game-owned focused frame and sends
   a focus hint through the existing `uiFrame_t.value` field for EDITBOX types;
   `false` clears it. The universal client retains typed text/caret state across
   ordinary layout redraws, activates SDL text input on tap or a focus hint,
   and sends an escaped quoted value through the editor's server-authored
   `OnClick` command (`uievent <frame> 9 "<value>"` for subscribed editor frames;
   the earlier `dzedit` command is still accepted). The game validates the live
   visible/enabled editor, updates `DzFrameGetText`, and invokes registered
   callback event **9** with the actual client-player context. This path is
   single-player only. A keyboard and pointer event fixture proves client
   delivery; no iPad hardware rendering/IME parity claim is made.
5. The client editor has one active field at a time and does not yet implement
   multi-field Tab traversal, pointer caret placement or a complete IME
   composition lifecycle. Server-authored changes to a *currently edited*
   value are not reconciled with a local in-progress edit. Scripted selection of
   popup/list entries has headless tests, but actual ORDR FDF layering, scroll
   bar controls and on-device interaction remain unverified.

## Event 10: list and popup item selection

- `FT_LISTBOX` mouse release hit-tests the same content inset, item height and
  vertical scroll offset as the renderer. A changed selectable row emits
  `uievent <frame-handle> 10 <row-index>`; a client cannot force arbitrary
  JASS callbacks with a spoofed item index.
- The WC3 FDF serializer projects `Popup.MenuFrame`'s authored `MENU` items
  into the existing frame text wire field as separate rows. Its selected index
  travels in the existing `value` field. The generic client offers a transient
  selectable popup overlay; no new `uiFrame_t` field or protocol bit is used.
- The game looks up the authored `MENU` for popup types, validates the index,
  stores the selection, then runs JASS event 10 in the current player context.
  Listbox selection updates the existing `uiListBox_t.selectedIndex`.
- Retail appearance, keyboard navigation, actual imported ORDR assets and
  iPad touch interaction still need real-device verification.

## Generic UI event input contract (current)

- A frame that subscribes to UI events publishes the existing `OnClick`
  metadata string `#uievent <frame-handle> <bitmask>`. This is a client-generic
  descriptor; **no uiFrame_t wire field, stat width, or protocol flag changed**.
  The `stat` field is only one byte on the wire and must not be repurposed for
  32-bit event masks/handles. The generic client parses the descriptor and sends
  `uievent <handle> <event-id>` as a `clc_stringcmd`. For an editor, it appends
  the quoted full text to event 9. Ordinary non-script `OnClick` commands and
  the legacy direct `dzframe`/`dzedit` game commands remain valid.
- Touch taps and mouse clicks share the existing `SCR_LayoutMouseEvent` path.
  Each real hover boundary emits at most one 2/3 edge; mouse presses/releases
  emit 5/4 only for subscribed frames; wheel motion emits 6; checkbox release
  asks the server to toggle, which reports 7 or 8 according to its state.
- `CHECKBOX`, `GLUECHECKBOX`, `SIMPLECHECKBOX`, `POPUPMENU`,
  `GLUEPOPUPMENU` and `LISTBOX` names can now be spawned by tag. Menu item
  selection event 10 has test coverage for click → validated index → JASS.

Tests were written first: frame priority order, event 9 callback and `dzkey` JASS action all
failed on the prior implementation, then passed after changes. The generic client test
`client_input.opt_in_game_key_edges_use_existing_string_command` likewise failed first and
now proves both physical key edges and console-mode suppression.

## Remaining Dz compatibility gaps

### Partial-implementation correctness pass (2026-10-23)

Regression tests first exposed and then verified fixes for these native paths:

- `DzCreateFrame`/`DzCreateFrameByTagName` reject a missing root template,
  missing named inheritance template, or unsupported frame type with handle
  `0` instead of returning an invisible or incorrectly styled frame that
  consumes a registry slot.
- `DzCreateFrameByTagName` initializes a new `LISTBOX`/popup with **no**
  selected item (`-1`). The first click on row index `0` now fires event 10,
  while a repeated click still does not.
- Frames cloned from FDF inherit the template's initial hidden/disabled state;
  event validation checks these actual UI flags throughout the parent chain.
- Dynamically created `SCROLLBAR` frames use the normal HUD scrollbar layout
  preparation when their Dz parent tree is serialized.
- `DzFrameSetTextAlignment` maps the frame point IDs `0` through `8` to the
  correct 3×3 horizontal/vertical FDF font justification grid; previously
  centered text (ID `4`) and bottom-right text (ID `8`) mapped incorrectly.
- `DzFrameSetFocus` now accepts only live, visible, enabled editable controls.
  Hiding or disabling a parent clears its focused descendant, preventing stale
  text input from reactivating when a hidden menu is reopened.
- Nested `DzSyncData` callbacks preserve the outer payload. Without a
  `currentplayer` during map initialization, the sync event now resolves the
  connected solo client as its sender; a destroyed trigger cannot fire again.
- `DzFrameSetFont` now updates the live font resource **and** the FDF font
  name/size, with stable rounding from UI coordinates to glyph size. A
  subsequent font-configstring change replaces the client-side font handle
  instead of retaining an old glyph atlas in the same numeric slot.
- `DzFrameSetModel` is tested from JASS resource path through game-side model
  index and `FT_MODEL` wire payload. The client now uses the base asset and
  `Stand` by default for a general `FT_MODEL` widget, reserving `_Portrait`
  companion models and `Portrait` defaults for `FT_PORTRAIT` widgets.

These are engine-level corrections, **not** proof that every Dz feature is
fully identical to the Windows plugin. Rendering and input still require an
end-to-end run of the actual ORDR map on the target iPad.

| Native group | Map functions | Remaining proof or work |
| --- | --- | --- |
| UI lifecycle | `DzGetGameUI`, `DzCreateFrame`, `DzCreateFrameByTagName`, `DzFrameFindByName`, `DzLoadToc` | Exercise the **actual ORDR imported TOC/FDF and asset references** on a loaded map. |
| Visual frames | `DzFrameShow`, `DzFrameSetEnable`, `DzFrameSetText`, `DzFrameGetText`, `DzFrameSetTexture`, `DzFrameSetSize`, `DzFrameSetAlpha`, `DzFrameSetPoint`, `DzFrameSetAbsolutePoint`, `DzFrameClearAllPoints`, `DzFrameSetFont`, `DzFrameSetTextAlignment`, `DzFrameSetModel` | Font/model asset paths now have game→wire and client-renderer regression coverage. The map's numeric model-type/font-flag semantics and exact camera/font styling are not yet established by the public native declarations; compare imported assets, layers, and absolute positions on device. |
| Partial properties | `DzFrameSetPriority`, `DzFrameSetFocus` | Priority sets **sibling order on wire**; edit focus drives client text entry, but full focus navigation/IME parity is unverified. |
| UI events | `DzFrameSetScriptByCode`, `DzGetTriggerUIEventFrame`, `DzGetTriggerUIEventPlayer` | Events 1–10 have headless callback/input-path coverage, including item selection 10. Additional unenumerated events, retail frame behavior and actual iPad interaction remain unverified. |
| Local single-player sync | `DzTriggerRegisterSyncData`, `DzSyncData`, `DzGetTriggerSyncData`, `DzGetTriggerSyncPlayer` | Existing local dispatch is not a multiplayer transport. Audit async event order and trigger lifecycle on real ORDR. |
| Keyboard | `DzTriggerRegisterKeyEventByCode`, `DzGetTriggerKeyPlayer` | Physical keyboard now travels via opted-in `cl_game_keycmd`/`dzkey` to JASS; verify retail status semantics on imported ORDR. iPad touch keyboard still needs its own control path. |

Headless tests: `wc3_api.dz_edit_focus_and_text_input_round_trip`,
`client_input.persistent_editbox_taps_accept_text_and_send_authored_command`,
`wc3_api.dz_toc_and_frame_creation_find_real_template_children`,
`wc3_api.dz_frames_emit_real_ui_payloads`, and
`wc3_api.dz_click_flows_through_actual_client_command_and_respects_visibility`,
`wc3_api.dz_script_frame_mouse_events_and_checkbox_toggle_are_authoritative`,
`client_input.scripted_layout_reports_real_pointer_edges_and_checkbox_toggles`,
`client_input.scripted_editbox_text_uses_generic_event_command`,
`client_input.scripted_listbox_selection_reports_changed_row`,
`client_input.scripted_popup_selection_only_after_opening_menu`,
`wc3_api.dz_listbox_selection_changes_only_for_valid_items`, and
`wc3_api.dz_popup_selection_uses_authored_menu_items`.
These cover parse/clone, generic wire output, a real `G_ClientCommand` click callback and
hidden-frame rejection. **They do not establish complete iPad ORDR playability.**

Additional focused tests: `wc3_api.dz_font_updates_fdf_and_serialized_glyph_face`,
`wc3_api.dz_model_path_reaches_the_wire_render_widget`,
`client_layout.model_widget_uses_base_model_and_stand_animation`, and
`client_layout.updated_font_configstring_rebinds_renderer_face`.

## 2026-10-23 iPad follow-up: frame pool, local mode, difficulty backdrop

- The FDF frame pool was still capped at 2048 on `89f3d282`. The ORDR
  `FDF_ParseFrame -> DzLoadToc` crash path has two independent protections:
  capacity 4096, and a null-safe recursive declaration skip when `UI_Spawn`
  exhausts the pool. The latter has a regression covering nested braces and
  parser recovery after a failed allocation; raising the limit alone would
  only postpone the same crash. The 4096 limit remains finite and further
  resource exhaustion must be handled as a recoverable load failure.
- **Correction:** It is the ORDR **map's** online status that the user wants
  disabled, **not** Warcraft III's Battle.net/LAN/transport features. The
  `BZ_IPAD_OFFLINE_ONLY` build flag, menu restrictions and remote-connection
  rejection have been removed. In ORDR only, the single-player predicate
  uses the actual connected player count rather than its authored lobby
  slots. Other Warcraft maps and multiplayer sessions are unaffected.
- Inspection of the **original** `ORDR_S2_2.323[C]` map found that the broad
  difficulty selection background is **not** an FDF BACKDROP: `CUN` creates
  destructable ID `B100`, whose W3B `bfil` is
  `war3mapImported\\Background.mdl`. The map contains the matching
  `war3mapImported\\Background.mdx` with a GEOS textured quad and a root-level
  `Background.tga` (960x540, 32-bit). The source's destructable path rewrite
  already produces `.mdx`, and the renderer has a TGA loader. The background
  failure therefore still needs a real-device model registration / visibility /
  texture-resolution trace; FDF texture changes alone cannot establish parity.

The desktop test suite and `BZ_IPAD_OFFLINE_ONLY` syntax checks do **not**
prove iOS rendering or that gameplay after difficulty selection is stable.

## Follow-up from the 2026-10-23 post-start iPad test

- The new JASS log continues beyond the previous crash (to roughly 51 s) and
  shows the previous `ORDR_OFFLINE_STARTUP ... command=-offline` at 12.1 s.
  **The root cause of the early "현재 온라인입니다." report was the stubbed
  Warcraft `Cheat("greedisgood")` native.** ORDR's original `BJK -> BJJ`
  initialization calls this cheat at approximately 1 s; at 2 s, `BJJ`
  checks whether the single-player resource cheat worked. `Lw=true`
  denotes ORDR's online detection and enables its login UI, independently
  of the `jK[player]` offline-command flag. Implementing real single-player
  `greedisgood` resource increments makes `BJJ` set `Lw=false` and clear the
  probe resources itself. Multiplayer and unrelated cheats remain unchanged.
- ORDR's own `CIh` chat handler is registered as soon as the map is ready.
  Its actual `-offline` command is queued **after 2.1 s** (and before the
  difficulty chooser at approximately 4 s) because the map's `CUX` trigger
  resets `Sw[player]` at 2 s; invoking `CIg` earlier would erase the
  offline clear-count bonus of 50. No map state is faked from the engine.
  The two independent flags now follow the original map's real JASS logic,
  and the UI label correction is only a fallback. iPad verification still
  requires `ORDR_SINGLEPLAYER_PROBE`, `ORDR_OFFLINE_STARTUP` and
  `ORDR_OFFLINE_CONFIRMED` in the new device log.
- iOS native `SDL_FINGER*` events previously recognized **two** fingers as a
  camera pan, but delivered a **one**-finger contact only as a release-time
  click. One-finger swipes in the game viewport now perform stock Warcraft
  **marquee unit selection** after 12 px of movement (instead of panning
  the camera). Stationary taps still select, two-finger dragging pans, and
  long-press/two-finger taps issue stock smart commands such as move/attack.
  The existing precise entity ray trace now has a constrained, nearest-player-
  unit screen-space fallback for small MDX meshes; decorative no-op layouts
  may no longer swallow a world-unit tap. Full-screen decorative FDF
  textures no longer block hit-testing, but interactive windows still have
  priority. Device-level selection and movement remain to be verified.
- The original `Background.mdx` and `Background.tga` have now rendered together
  in the desktop MDX preview renderer, proving the *asset/texture decoder*
  path. This is **not** proof of in-map placement or visibility. A targeted
  `ORDR_BACKDROP_SPAWN` JASS log records the actual B100 model, flags, ground
  height, and authored model path; `ORDR_WORLD_SELECT_COMMAND` distinguishes
  server-accepted taps from client-side touch dispatch failures. Request an
  iPad screenshot to pin down the unrelated UI layout offset. Do not claim
  the invisible background or layout has been fixed until a device screenshot
  and the new trace confirm them.


## 2026-10-27 — 엔진 오류 일괄 수정 / iPad 시뮬레이터 자동 실행 중단

원본 \`ORDR_S2_2.323[C].w3x\` / \`war3map.j\`는 수정하지 않는다.
사용자 지시에 따라 원랜디에 드러난 오류들을 먼저 한 번에 수정하고,
시뮬레이터 및 IPA 워크플로는 **수동 실행(workflow_dispatch)**으로 유지한다.
일반 브랜치 커밋, PR 갱신만으로 iPad 시뮬을 실행하지 않는다.

이번 엔진 변경 사항 (런타임 결과 검증 **전**):
- **DzAPI 아이콘과 점수판 레이어 충돌:** 기존 \`LAYER_GAME_3\`을
  Dz/FDF에만 사용하고, 점수판/팀 자원 UI를 새 \`LAYER_GAME_4\`로 분리.
  별도 레이어의 점수판 셀 텍스트만 한 줄/클리핑 처리한다.
- **Unicode emoji 잘못된 글리프 캐시:** 폰트 캐시에서
  \`(codepoint >> 8) % 256\` 때문에 보충 평면이 BMP 페이지와 충돌하던
  문제를 제거. 글꼴 자체에 해당 emoji 글리프가 없으면 여전히 표시되지
  않을 수 있으며 별도 폰트 지원이 필요하다.
- **게임 시작 이후 B100 선택 장면 배경:** 전장 카메라와 난이도 장면
  카메라를 구분하여 배경 표시 상태만 바꾼다.
- **물/지면 표시:** 물이 있는 타일 안에서도 수면 위로 솟은 지면을
  삼각형 단위로 가린다. W3E 지형/텍스처 파일은 그대로 유지한다.
- **줄바꿈과 점수판:** \`\\n\`, \`|n\`, 실제 LF 표기와 점수판 셀
  클리핑을 클라이언트 렌더링에서 수정한다.
- **JASS 0 나누기:** \`CV5\`의 \`360 / (vP * EK)\` 피연산자를
  기록하지만 원본 수치나 계산식은 임의로 대체하지 않는다.
  JASS의 divide-by-zero는 원래 스레드 중단을 일으키므로, 원인
  값을 고치기 전에는 해결 상태로 표시하지 않는다.

**여전히 미해결 / 실기기 또는 엔진 이벤트 검증 필요**:
\`CV5\` 영(0) 피연산자의 실제 출처, 위습 선택/이동,
\`e0IX\`의 \`CH\` 비콘 진입과 \`CRy\` 유닛 생성,
적 유닛의 경로 명령/이동, 플레이어 수에 따른 파티 모드 판정,
폰트에 없는 이모티콘 및 성능 문제.
실제 iPad 자동 터치/비콘/전투를 검증한 것으로 보고하지 않는다.


### 2026-10-27: 런타임 전 추가 원인 수정 (시뮬레이터 미실행)

- **파티 모드:** 원본 W3I는 여러 human-capable 슬롯을 갖지만 로컬 실제 접속자는 하나뿐이다.
  싱글 ORDR에서 이미 접속자가 존재할 때만, 미접속 human 슬롯의
  `GetPlayerController` 결과를 `MAP_CONTROL_NONE`으로 매핑한다.
  `GetPlayerSlotState`는 기존 접속 판정을 유지한다.
- **JASS의 변수 스코프:** `VM_EvalIdentifier`, 배열 접근, `SET`가
  전역을 지역보다 먼저 검색하던 문제 수정. JASS의 지역변수/인수 우선
  규칙으로 통일했다. `CV5`의 `vP=10, EK=0` 오류가 동일 이름의 전역
  변수로 인한 것인지 확인 가능한 제한된 스코프 추적 추가.
  **사전 코드 분석만으로 EK가 양수로 회복되는지는 확인되지 않았다.**
- **적 이동/위습 경로:** `IsTerrainPathable`은 항상 `true`를 반환하던
  미구현 native였다. 원본 WPM 경로 플래그와 JASS pathingtype enum을
  구분하여 지형 경로·비행·수영·건설·황폐화 가능 여부를 질의하도록 변경.
  `SetTerrainPathable`의 동적 경로 변경은 별도 연동이 필요하다.
- **지형과 물:** 물은 원본 W3E의 물 비트가 있는 꼭짓점에서만 그리도록
  삼각형을 자르며, 지면 높이는 지형 렌더러가 실제 그리는 램프/변형
  지형 높이를 그대로 사용한다.
- **위습/전투 미확인:** `e0IX` → `CH` 구역 → `CRy` 이벤트,
  적 유닛 실제 이동과 공격, 실기기 한 손가락 선택은 아직
  플레이 검증하지 않았다. 임의의 유닛 생성이나 공격 로직을 추가하지 않는다.

테스트 운영 규칙: 작은 커밋마다 iPad 시뮬레이터/IPA를 돌리지 않는다.
현재 두 GitHub 워크플로는 `workflow_dispatch`만 허용한다. 런타임
메커니즘 분석과 엔진 코드·기본 CI 단계만 우선 수행하고, 원인 분석이
완료되지 않은 사항은 `미검증`/ `미해결`로 명시한다.
