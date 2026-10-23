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
