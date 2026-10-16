#ifdef BZ_TESTS
#include "shared/test.h"
#include "../g_local.h"
#include "jass/jass.h"

void setup_test_world(void);
bool run_test_jass(cstring_t script);
bool run_test_jass_error(cstring_t script, cstring_t expected);
extern player_t *currentplayer;

#define ORDR_SYNC_DECLS \
    "native DzSyncData takes string prefix, string data returns nothing\n" \
    "native DzTriggerRegisterSyncData takes trigger trig, string prefix, boolean server returns nothing\n" \
    "native DzGetTriggerSyncData takes nothing returns string\n" \
    "native DzGetTriggerSyncPlayer takes nothing returns player\n"

/* ORDR routes local UI selections through prefix-filtered sync actions. Drive
 * the normal frame scheduler so queued sends cannot silently disappear. */
TEST(wc3_ordr_sync, local_messages_keep_prefix_order_sender_and_nested_context) {
    player_t *old = currentplayer;
    setup_test_world();
    currentplayer = &game.clients[2].ps;
    T_ASSERT(run_test_jass(ORDR_SYNC_DECLS
        "globals\n"
        " integer hits = 0\n"
        " integer nestedHits = 0\n"
        " integer executeHits = 0\n"
        " integer wrong = 0\n"
        " string received = \"\"\n"
        " trigger nested = null\n"
        "endglobals\n"
        "function filter takes nothing returns boolean\n"
        " if DzGetTriggerSyncData() == \"nested\" then\n"
        "  call BJassAssert(DzGetTriggerSyncPlayer() == Player(1), \"nested send used event player\")\n"
        " else\n"
        "  call BJassAssert(DzGetTriggerSyncPlayer() == Player(2), \"condition sender mismatch\")\n"
        " endif\n"
        " return DzGetTriggerSyncData() != \"reject\"\n"
        "endfunction\n"
        "function onNested takes nothing returns nothing\n"
        " call BJassAssert(DzGetTriggerSyncData() == \"first\", \"nested context lost payload\")\n"
        " call BJassAssert(DzGetTriggerSyncPlayer() == Player(2), \"nested context lost sender\")\n"
        " set nestedHits = nestedHits + 1\n"
        "endfunction\n"
        "function onExecute takes nothing returns nothing\n"
        " call TriggerSleepAction(0.05)\n"
        " call BJassAssert(DzGetTriggerSyncData() == \"first\", \"ExecuteFunc lost sleeping payload\")\n"
        " call BJassAssert(DzGetTriggerSyncPlayer() == Player(2), \"ExecuteFunc lost sleeping sender\")\n"
        " set executeHits = executeHits + 1\n"
        "endfunction\n"
        "function onSync takes nothing returns nothing\n"
        " local string data = DzGetTriggerSyncData()\n"
        " call BJassAssert(GetTriggerPlayer() == DzGetTriggerSyncPlayer(), \"event player mismatch\")\n"
        " set received = received + data + \"|\"\n"
        " set hits = hits + 1\n"
        " if data == \"first\" then\n"
        "  call TriggerExecute(nested)\n"
        "  call ExecuteFunc(\"onExecute\")\n"
        "  call DzSyncData(\"alpha\", \"nested\")\n"
        "  call BJassAssert(DzGetTriggerSyncData() == data, \"send overwrote active payload\")\n"
        " endif\n"
        "endfunction\n"
        "function onWrong takes nothing returns nothing\n"
        " set wrong = wrong + 1\n"
        "endfunction\n"
        "function main takes nothing returns nothing\n"
        " local trigger t = CreateTrigger()\n"
        " local trigger other = CreateTrigger()\n"
        " local trigger disabled = CreateTrigger()\n"
        " set nested = CreateTrigger()\n"
        " call TriggerAddAction(nested, function onNested)\n"
        " call TriggerAddCondition(t, Condition(function filter))\n"
        " call TriggerAddAction(t, function onSync)\n"
        " call TriggerAddAction(other, function onWrong)\n"
        " call TriggerAddAction(disabled, function onWrong)\n"
        " call DzTriggerRegisterSyncData(t, \"alpha\", false)\n"
        " call DzTriggerRegisterSyncData(other, \"alph\", false)\n"
        " call DzTriggerRegisterSyncData(disabled, \"alpha\", false)\n"
        " call DisableTrigger(disabled)\n"
        " call BJassAssert(DzGetTriggerSyncData() == \"\" and DzGetTriggerSyncPlayer() == null, \"ambient sync context\")\n"
        " call DzSyncData(\"alpha\", \"first\")\n"
        " call DzSyncData(\"alpha\", \"second\")\n"
        " call DzSyncData(\"alpha\", \"\")\n"
        " call DzSyncData(\"alpha\", \"reject\")\n"
        " call DzSyncData(\"missing\", \"ignored\")\n"
        " call BJassAssert(hits == 0, \"sync callback ran inline\")\n"
        "endfunction\n"
        "function verify takes nothing returns nothing\n"
        " call BJassAssert(hits == 4 and nestedHits == 1 and executeHits == 1 and wrong == 0, \"dispatch/filter mismatch\")\n"
        " call BJassAssert(received == \"first|second||nested|\", \"payload order mismatch\")\n"
        " call BJassAssert(DzGetTriggerSyncData() == \"\" and DzGetTriggerSyncPlayer() == null, \"sync context leaked\")\n"
        "endfunction\n"));
    /* Presentation selection may change after send; queued messages retain
     * player 2 while a send from their callback uses GetLocalPlayer() (1). */
    currentplayer = &game.clients[1].ps;
    level.started = level.scriptsStarted = true;
    globals.RunFrame();
    globals.RunFrame();
    level.time += 100;
    globals.RunFrame();
    jass_callbyname(level.vm, "verify", false);
    T_ASSERT(!jass_rterror_pending(level.vm));
    currentplayer = old;
}

/* Persist both queued transport payloads and a sleeping recipient. Neither
 * may retain a pointer to a reused queue slot or a previous VM. */
TEST(wc3_ordr_sync, queued_and_sleeping_messages_survive_save_load) {
    cstring_t file = Test_TempPath("wc3-ordr-sync.sav");
    player_t *old = currentplayer;
    setup_test_world();
    currentplayer = &game.clients[2].ps;
    T_ASSERT(run_test_jass(ORDR_SYNC_DECLS
        "globals\n integer hits = 0\n integer resumed = 0\nendglobals\n"
        "function onSync takes nothing returns nothing\n"
        " local string data = DzGetTriggerSyncData()\n"
        " call BJassAssert(data == \"saved payload\", \"queued payload lost\")\n"
        " set hits = hits + 1\n"
        " call TriggerSleepAction(0.05)\n"
        " call BJassAssert(DzGetTriggerSyncData() == data, \"sleeping payload lost\")\n"
        " call BJassAssert(DzGetTriggerSyncPlayer() == Player(2), \"sleeping sender lost\")\n"
        " set resumed = resumed + 1\n"
        "endfunction\n"
        "function main takes nothing returns nothing\n"
        " local trigger t = CreateTrigger()\n"
        " call TriggerAddAction(t, function onSync)\n"
        " call DzTriggerRegisterSyncData(t, \"save\", false)\n"
        " call DzSyncData(\"save\", \"saved payload\")\n"
        "endfunction\n"
        "function sendAgain takes nothing returns nothing\n"
        " call DzSyncData(\"save\", \"saved payload\")\n"
        "endfunction\n"
        "function verify takes nothing returns nothing\n"
        " call BJassAssert(hits == 2 and resumed == 2, \"saved sync callback or registration lost\")\n"
        " call BJassAssert(DzGetTriggerSyncPlayer() == null, \"loaded context leaked\")\n"
        "endfunction\n"));
    T_ASSERT(WriteGame(file));
    T_ASSERT(ReadGame(file));
    level.started = level.scriptsStarted = true;
    globals.RunFrame();
    T_ASSERT(WriteGame(file));
    T_ASSERT(ReadGame(file));
    level.time += 100;
    globals.RunFrame();
    jass_callbyname(level.vm, "sendAgain", false);
    globals.RunFrame();
    level.time += 100;
    globals.RunFrame();
    jass_callbyname(level.vm, "verify", false);
    T_ASSERT(!jass_rterror_pending(level.vm));
    remove(file);
    currentplayer = old;
}

/* A failed restore after allocating the context payload must leave the live
 * sleeping recipient intact and permit a subsequent complete restoration. */
typedef struct {
    FILE *file;
    bool write, cut_after_payload, cut;
} ordrSyncSnapshot_t;

static bool ordr_sync_snapshot_io(void *context, void *data, uint32_t size) {
    ordrSyncSnapshot_t *stream = context;
    static char const payload[] = "snapshot payload";
    if (stream->cut) return false;
    if (stream->write) return fwrite(data, 1, size, stream->file) == size;
    if (fread(data, 1, size, stream->file) != size) return false;
    if (stream->cut_after_payload && size == sizeof(payload) && !memcmp(data, payload, size))
        stream->cut = true;
    return true;
}

TEST(wc3_ordr_sync, failed_snapshot_restore_retains_live_sleeping_payload) {
    cstring_t file = Test_TempPath("wc3-ordr-sync.snapshot");
    player_t *old = currentplayer;
    setup_test_world();
    currentplayer = &game.clients[2].ps;
    T_ASSERT(run_test_jass(ORDR_SYNC_DECLS
        "globals\n integer resumed = 0\nendglobals\n"
        "function onSync takes nothing returns nothing\n"
        " call TriggerSleepAction(0.05)\n"
        " call BJassAssert(DzGetTriggerSyncData() == \"snapshot payload\", \"restored payload lost\")\n"
        " call BJassAssert(DzGetTriggerSyncPlayer() == Player(2), \"restored sender lost\")\n"
        " set resumed = resumed + 1\n"
        "endfunction\n"
        "function main takes nothing returns nothing\n"
        " local trigger t = CreateTrigger()\n"
        " call TriggerAddAction(t, function onSync)\n"
        " call DzTriggerRegisterSyncData(t, \"save\", false)\n"
        " call DzSyncData(\"save\", \"snapshot payload\")\n"
        "endfunction\n"
        "function verify takes nothing returns nothing\n"
        " call BJassAssert(resumed == 1, \"failed restore mutated live coroutine\")\n"
        "endfunction\n"));
    G_RunEvents();
    jass_runevents(level.vm);
    ordrSyncSnapshot_t stream = { .file = fopen(file, "w+b"), .write = true };
    T_NOT_NULL(stream.file);
    if (!stream.file) { currentplayer = old; return; }
    jassSnapshot_t snapshot = { .context = &stream, .transfer = ordr_sync_snapshot_io };
    T_ASSERT(jass_writesnapshot(level.vm, &snapshot));
    rewind(stream.file);
    stream.write = false;
    stream.cut_after_payload = true;
    T_ASSERT(!jass_readsnapshot(level.vm, &snapshot));
    T_ASSERT(stream.cut);
    /* The original sleeping coroutine survives the failed staged load. */
    level.time += 100;
    jass_runevents(level.vm);
    jass_callbyname(level.vm, "verify", false);
    T_ASSERT(!jass_rterror_pending(level.vm));
    /* Loading the complete snapshot also frees the old context safely. */
    rewind(stream.file);
    stream.cut_after_payload = stream.cut = false;
    T_ASSERT(jass_readsnapshot(level.vm, &snapshot));
    level.time += 100;
    jass_runevents(level.vm);
    jass_callbyname(level.vm, "verify", false);
    T_ASSERT(!jass_rterror_pending(level.vm));
    fclose(stream.file);
    remove(file);
    currentplayer = old;
}

/* ORDR uses server=false. Reject unsupported remote/server channels explicitly. */
TEST(wc3_ordr_sync, server_channels_report_unsupported_semantics) {
    setup_test_world();
    T_ASSERT(run_test_jass_error(ORDR_SYNC_DECLS
        "function main takes nothing returns nothing\n"
        " call DzTriggerRegisterSyncData(CreateTrigger(), \"remote\", true)\n"
        "endfunction\n", "DzTriggerRegisterSyncData: server channels are not supported"));
}

TEST(wc3_ordr_sync, send_without_local_player_reports_invalid_context) {
    player_t *old = currentplayer;
    setup_test_world();
    currentplayer = NULL;
    T_ASSERT(run_test_jass_error(ORDR_SYNC_DECLS
        "function main takes nothing returns nothing\n"
        " call DzSyncData(\"local\", \"data\")\n"
        "endfunction\n", "DzSyncData: no local player selected"));
    currentplayer = old;
}

TEST(wc3_ordr_sync, queue_exhaustion_preserves_pending_payloads_and_reset_releases_strings) {
    player_t *old = currentplayer;
    setup_test_world();
    currentplayer = &game.clients[0].ps;
    T_ASSERT(run_test_jass_error(ORDR_SYNC_DECLS
        "function main takes nothing returns nothing\n"
        " local trigger t = CreateTrigger()\n"
        " call DzTriggerRegisterSyncData(t, \"queue\", false)\n"
        " loop\n"
        "  call DzSyncData(\"queue\", \"pending\")\n"
        " endloop\n"
        "endfunction\n", "DzSyncData: event queue is full"));
    T_EQ(level.events.write - level.events.read, (uint32_t)MAX_EVENT_QUEUE);
    T_STREQ(level.events.queue[0].sync_data, "pending");
    T_STREQ(level.events.queue[MAX_EVENT_QUEUE - 1].sync_data, "pending");
    G_ClearSyncEvents();
    FOR_LOOP(i, MAX_EVENT_QUEUE) T_NULL(level.events.queue[i].sync_data);
    FOR_LOOP(i, MAX_EVENTS) if (level.events.handlers[i].type == EVENT_CUSTOM_SYNC_DATA)
        T_NULL(level.events.handlers[i].variable);
    currentplayer = old;
}
#endif
