# ORDR Dz runtime frames

`game/api/api_ordr_frames.h` implements a rendering subset of Dz frames through
the existing game-owned FDF pool. The registered natives are `DzGetGameUI`,
`DzCreateFrame`, `DzCreateFrameByTagName`, `DzFrameFindByName`, `DzFrameSetText`,
`DzFrameGetText`, `DzFrameSetSize`, `DzFrameSetPoint`,
`DzFrameSetAbsolutePoint`, `DzFrameClearAllPoints`, `DzFrameSetTexture`,
`DzFrameSetFont`, `DzFrameSetAlpha`, `DzFrameSetEnable`, and `DzFrameShow`.
Input scripts, model assignment, priority, and other Dz natives remain
unregistered and produce the existing unimplemented-native error.

Frame handles are integers scoped to a HUD lifetime. Handles increase across
resets, so a stale integer cannot address a newly created frame. Template
creation clones the full FDF tree; children have independent state and are
discoverable by name plus creation context. Supported tag names are `FRAME`,
`TEXT`, `BACKDROP`, `BUTTON`, and `SPRITE`. Missing templates, unsupported tags,
invalid handles, out-of-range anchors/coordinates, unsupported font/texture
flags, and local-only frame creation raise runtime errors.

Each frame has synchronized properties and optional per-player overrides.
Assignments made inside the VM's `GetLocalPlayer` branch affect only that
player's text, art, dimensions, anchors, font, alpha, enabled state, or
visibility. A later synchronized assignment replaces earlier local overrides
for the assigned property. There are no client-side gameplay rules or new
network fields.

`G_RunFrame` drains dirty recipients after JASS events. It invalidates the
existing resource-bar cache and calls `G_RefreshResourceBar`, which appends the
custom frames after stock console/minimap frames in `LAYER_CONSOLE`.
Unchanged frames do not resend the console. Reconnect/resource refresh rebuilds
the complete recipient-specific snapshot. Hidden frames and descendants retain
geometry-only disabled entries, preserving anchor targets without drawing art.
Font overrides preserve independent alpha/color properties. Local anchor changes retain prior anchors and apply the same stretched-axis sizing as synchronized changes.
The snapshot remaps all custom parents and anchors, including forward references.
The WC3 bottom-left absolute origin is encoded through the custom root.

Anchor references retain the existing byte representation: 0 is the screen,
1 through 254 are frames, and 255 is the parent sentinel. Creation reserves the
entire stock console FDF tree, including hidden/control art, plus five possible
native proxy frames (clock, clock listener, quest attention, tooltip, minimap).
Custom creation fails before that combined conservative budget exceeds 254.
The write path also checks the actual combined count before emitting custom
frames. Protocol flags, layouts, widths, and versions remain unchanged.

`UI_ResetHud` releases local property allocations before clearing FDF storage.
Runtime frames have no save serializer yet. `WriteGame` rejects a live Dz frame
registry before opening the destination file, preserving an existing save.

`wc3_ordr_frames.*` drives native dispatch with synthetic JASS and
`TestUI/Frames/ordr_compat.fdf`; it captures production `PF_UIFRAME` payloads.
It covers context lookup and template independence, stock/custom console
composition, art/fonts, forward anchors and cleared anchors, local visibility
and text, global inverse assignments, scheduler-driven cache miss/hit,
recipient refresh, shared frame bounds, save rejection, and expired handles.
Run after `make test-assets` using:

```
build/bin/openwarcraft3-tests -data build/tests +dedicated 1 +test 'wc3_ordr_frames.*'
```

These fixture tests establish the rendering contract at the game import
boundary. They do not establish ORDR map initialization, input callbacks,
combat, client framebuffer output, or iPad execution.
