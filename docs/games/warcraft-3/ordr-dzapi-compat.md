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

## Remaining Dz compatibility gaps

| Native group | Map functions | Remaining proof or work |
| --- | --- | --- |
| UI lifecycle | `DzGetGameUI`, `DzCreateFrame`, `DzCreateFrameByTagName`, `DzFrameFindByName`, `DzLoadToc` | Exercise the **actual ORDR imported TOC/FDF and asset references** on a loaded map. |
| Visual frames | `DzFrameShow`, `DzFrameSetEnable`, `DzFrameSetText`, `DzFrameGetText`, `DzFrameSetTexture`, `DzFrameSetSize`, `DzFrameSetAlpha`, `DzFrameSetPoint`, `DzFrameSetAbsolutePoint`, `DzFrameClearAllPoints`, `DzFrameSetFont`, `DzFrameSetTextAlignment`, `DzFrameSetModel` | Compare on-device text alignment, layered textures/alpha, model animation and absolute screen positions against the retail UI. |
| Partial properties | `DzFrameSetPriority`, `DzFrameSetFocus` | Priority is only retained; no FDF/client z-order mapping. Focus accepts a valid frame but does not request client edit focus. |
| UI events | `DzFrameSetScriptByCode`, `DzGetTriggerUIEventFrame`, `DzGetTriggerUIEventPlayer` | Verified click event 1 over real client command. Other Dz event IDs (edit, enter/leave, wheel, etc.) and physical iPad taps still need client/device coverage. |
| Local single-player sync | `DzTriggerRegisterSyncData`, `DzSyncData`, `DzGetTriggerSyncData`, `DzGetTriggerSyncPlayer` | Existing local dispatch is not a multiplayer transport. Audit async event order and trigger lifecycle on real ORDR. |
| Keyboard | `DzTriggerRegisterKeyEventByCode`, `DzGetTriggerKeyPlayer` | Registration/explicit dispatch exists; physical-key and touch-keyboard input are **not** wired. |

Headless tests: `wc3_api.dz_toc_and_frame_creation_find_real_template_children`,
`wc3_api.dz_frames_emit_real_ui_payloads`, and
`wc3_api.dz_click_flows_through_actual_client_command_and_respects_visibility`.
These cover parse/clone, generic wire output, a real `G_ClientCommand` click callback and
hidden-frame rejection. **They do not establish complete iPad ORDR playability.**
