# iPad custom map startup diagnostics

Custom maps can render terrain even when their JASS startup script did not
complete. Loading a `.w3x` archive is not proof that `config()`, `main()`,
the map's initial unit creation, or scheduled triggers succeeded.

On iPad, the Warcraft III game module now writes
`OpenRealm-JASS.log` to the app's **Documents** directory. Each map load
replaces the preceding log. It records script load phases, a `main` marker,
and the first occurrence of each distinct JASS runtime error encountered
during load, startup, or subsequent frames.

## Collecting a report

1. Launch a custom map on the iPad and wait a few seconds.
2. Open Files and navigate to the Documents folder belonging to the
   OpenWarcraft3 app. If launched inside LiveContainer, find the **virtual app
   data container** selected for OpenWarcraft3 and open its `Documents` folder.
3. Read `OpenRealm-JASS.log`. The error line includes the stage and the
   runtime error message.
4. Report only the relevant log lines and which map was tested. Do not upload
   copyrighted retail MPQs or private signing certificates.

Examples:

```text
map: Maps/example.w3x
stage: war3map.j loaded
stage: config completed
stage: calling main
main: unimplemented native: ExampleNative
```

For ORDR map filenames, the iPad log also includes five `ordr-frame`
checkpoints (frames 1, 5, 20, 60, 120). These record the number of live
entities (`units`), owner counts (`p7`, `p15`), registered triggers, timers,
events, and logical DzFrame instances. If the difficulty-selection NPCs are
missing, compare the counters between checkpoints. Unchanged counts indicate
that the corresponding map callbacks may not have created new entities;
nonzero counts alone do not prove that a model rendered on screen.

**The example above is illustrative, not a verified error from a specific map.**
A successful log without errors does **not** prove that a custom game works:
an event handler may never fire, or an unimplemented native may use an
incomplete placeholder.

## For engine developers

Startup phases are in `games/warcraft-3/game/g_spawn.c` and
`games/warcraft-3/game/g_main.c`; the JASS runtime error API is in
`games/warcraft-3/jass/jass.h`. Missing built-in registrations are listed
in `games/warcraft-3/game/api/api_module.c`.

Do not add generic no-op replacements for extension APIs until their
side effects, event dispatch, multiplayer synchronization, and persistence
requirements are understood. Such replacements can hide the initial error
while silently breaking the gameplay state.
