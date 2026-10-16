# ORDR local Dz sync dispatch

## Verified contract

The private `ORDR_S2_2.323[C].w3x` script declares:

```jass
native DzSyncData takes string prefix, string data returns nothing
native DzTriggerRegisterSyncData takes trigger trig, string prefix, boolean server returns nothing
native DzGetTriggerSyncData takes nothing returns string
native DzGetTriggerSyncPlayer takes nothing returns player
```

All direct registrations and the map's registration wrapper use `server=false`.
Local UI handlers send selections, login text and save/load messages. Recipients
read the payload and sender inside trigger actions; some execute other triggers.
Consequently these calls are needed for offline gameplay even though their names
refer to synchronization. See the public signature/call metadata in
[`tools/ordr_native_usage.json`](../../../tools/ordr_native_usage.json); the map
script and resources must stay private.

This implementation provides local dispatch only. It adds no multiplayer
transport, remote identity, account login or server channel. `server=true` raises
an explicit JASS runtime error rather than appearing to register a remote channel.

## Ownership and event order

`DzTriggerRegisterSyncData` creates an ordinary `event_t` registration of internal
type `EVENT_CUSTOM_SYNC_DATA`. Its existing `variable` string slot owns the exact
prefix. `DzSyncData` selects matching registrations, queues one `gameEvent_t` per
registration and copies the payload. The sender is the current `GetLocalPlayer()`
selection at send time, independently of the triggering event's player.

The normal `G_RunFrame` → `G_RunEvents` → JASS coroutine path evaluates conditions
and respects disabled triggers. Sends do not call actions inline. Messages retain
send order; sends made by actions enter the subsequent event pass. A missing
prefix has no recipient. Empty payloads remain valid messages. Registry/queue
exhaustion and sends with no local player raise runtime errors.

Conditions borrow the current event payload. Each action coroutine owns a separate
copy, including nested `TriggerExecute`/`ExecuteFunc` contexts, and releases it on
completion or VM shutdown. Thus later sends, queue-slot reuse and sleeps cannot
overwrite a recipient's response. Outside sync context the getters return the
empty string and a null player. Queue strings are released after dispatch, or by
`G_ClearSyncEvents` at map replacement, game shutdown, save restoration and test
reset. Prefix strings are released at the same lifecycle boundaries.

## Persistence and verification

Game save format **90** stores queued payloads/senders and the existing registration
prefix. JASS snapshot format **9** stores each suspended recipient's payload and
sender. Older formats are rejected; there is no save migration.

`games/warcraft-3/game/tests/t_ordr_sync.c` drives production native dispatch and the
real frame scheduler. The initial regression failed with
`unimplemented native: DzTriggerRegisterSyncData`. Coverage includes exact prefix
matching, FIFO delivery, conditions, disabled triggers, empty payloads, nested
execution, sender/presentation separation, context clearing, queued-message and
sleeping-coroutine save/load, registration reuse after restoration and explicit
unsupported/invalid-context errors. It also verifies `ExecuteFunc` inheritance
across sleeps, failed snapshot staging immediately after payload allocation,
successful restoration after that failure, queue exhaustion without overwriting
pending payloads and explicit queue/prefix string cleanup. Run with:

```sh
make test-wc3-engine WC3_PATTERN='wc3_ordr_sync.*'
```

These tests establish the local sync behavior with synthetic scripts. They do not
establish that ORDR's full UI, map initialization, combat or iPad input works.
