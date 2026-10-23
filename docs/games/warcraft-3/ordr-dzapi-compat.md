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
   `DzFrameSetScriptByCode` click event 1 creates a real `OnClick` command; `dzframe`
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
   (edit-box text changed). The game can deliver any retained event by
   `ORDR_DispatchFrameEvent`; live click 1 and edit text change 9 are wired from
   actual client interactions. Mouse enter/leave, up/down, wheel, checkbox and
   popup selections still lack independent event dispatch.
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
   `OnClick` command (`dzedit <frame> "<value>"`). The game validates the live
   visible/enabled editor, updates `DzFrameGetText`, and invokes registered
   callback event **9** with the actual client-player context. This path is
   single-player only. A keyboard and pointer event fixture proves client
   delivery; no iPad hardware rendering/IME parity claim is made.
5. The client editor has one active field at a time and does not yet implement
   multi-field Tab traversal, pointer caret placement or a complete IME
   composition lifecycle. Server-authored changes to a *currently edited*
   value are not reconciled with a local in-progress edit. Event IDs 2–8 and
   10, scroll bar controls and actual ORDR in-game testing remain pending.

Tests were written first: frame priority order, event 9 callback and `dzkey` JASS action all
failed on the prior implementation, then passed after changes. The generic client test
`client_input.opt_in_game_key_edges_use_existing_string_command` likewise failed first and
now proves both physical key edges and console-mode suppression.

## Remaining Dz compatibility gaps

| Native group | Map functions | Remaining proof or work |
| --- | --- | --- |
| UI lifecycle | `DzGetGameUI`, `DzCreateFrame`, `DzCreateFrameByTagName`, `DzFrameFindByName`, `DzLoadToc` | Exercise the **actual ORDR imported TOC/FDF and asset references** on a loaded map. |
| Visual frames | `DzFrameShow`, `DzFrameSetEnable`, `DzFrameSetText`, `DzFrameGetText`, `DzFrameSetTexture`, `DzFrameSetSize`, `DzFrameSetAlpha`, `DzFrameSetPoint`, `DzFrameSetAbsolutePoint`, `DzFrameClearAllPoints`, `DzFrameSetFont`, `DzFrameSetTextAlignment`, `DzFrameSetModel` | Compare on-device text alignment, layered textures/alpha, model animation and absolute screen positions against the retail UI. |
| Partial properties | `DzFrameSetPriority`, `DzFrameSetFocus` | Priority sets **sibling order on wire**; edit focus drives client text entry, but full focus navigation/IME parity is unverified. |
| UI events | `DzFrameSetScriptByCode`, `DzGetTriggerUIEventFrame`, `DzGetTriggerUIEventPlayer` | Click event 1 and edit event 9 reach JASS through real client commands. Remaining mouse/scroll/check events and actual iPad interaction are unverified. |
| Local single-player sync | `DzTriggerRegisterSyncData`, `DzSyncData`, `DzGetTriggerSyncData`, `DzGetTriggerSyncPlayer` | Existing local dispatch is not a multiplayer transport. Audit async event order and trigger lifecycle on real ORDR. |
| Keyboard | `DzTriggerRegisterKeyEventByCode`, `DzGetTriggerKeyPlayer` | Physical keyboard now travels via opted-in `cl_game_keycmd`/`dzkey` to JASS; verify retail status semantics on imported ORDR. iPad touch keyboard still needs its own control path. |

Headless tests: `wc3_api.dz_edit_focus_and_text_input_round_trip`,
`client_input.persistent_editbox_taps_accept_text_and_send_authored_command`,
`wc3_api.dz_toc_and_frame_creation_find_real_template_children`,
`wc3_api.dz_frames_emit_real_ui_payloads`, and
`wc3_api.dz_click_flows_through_actual_client_command_and_respects_visibility`.
These cover parse/clone, generic wire output, a real `G_ClientCommand` click callback and
hidden-frame rejection. **They do not establish complete iPad ORDR playability.**
