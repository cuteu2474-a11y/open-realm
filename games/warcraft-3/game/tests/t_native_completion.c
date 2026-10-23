#ifdef BZ_TESTS
#include "test.h"
#include "../g_local.h"
#include "jass/jass.h"
void setup_test_world(void);
bool run_test_jass(cstring_t src);

TEST(wc3_native_completion, time_scale_drives_clock_and_survives_save) {
    cstring_t file = Test_TempPath("native-time-scale.sav");
    setup_test_world();
    T_ASSERT(run_test_jass(
        "function main takes nothing returns nothing\n"
        " call BJassAssert(GetTimeOfDayScale() == 1.0, \"default clock scale\")\n"
        " call SetTimeOfDayScale(2.0)\n"
        " call BJassAssert(GetTimeOfDayScale() == 2.0, \"clock scale getter\")\nendfunction\n"
        "function freeze takes nothing returns nothing\n call SetTimeOfDayScale(0.0)\nendfunction\n"));
    float step = game.constants.gameDayHours / game.constants.gameDayLength * FRAMETIME / 1000.0f;
    level.started = true; level.scriptsStarted = true;
    globals.RunFrame();
    T_FEQ(G_GetTimeOfDay(), 2.0f * step, 0.0001f);
    T_ASSERT(WriteGame(file));
    jass_callbyname(level.vm, "freeze", false); G_UpdateTimeOfDay();
    T_FEQ(G_GetTimeOfDay(), 2.0f * step, 0.0001f);
    T_ASSERT(ReadGame(file)); G_UpdateTimeOfDay();
    T_FEQ(G_GetTimeOfDay(), 4.0f * step, 0.0001f);
    G_SuspendTimeOfDay(true); G_UpdateTimeOfDay();
    T_FEQ(G_GetTimeOfDay(), 4.0f * step, 0.0001f);
    G_SuspendTimeOfDay(false); G_UpdateTimeOfDay();
    T_FEQ(G_GetTimeOfDay(), 6.0f * step, 0.0001f); remove(file);
}

TEST(wc3_native_completion, animation_index_uses_model_order_and_advances) {
    setup_test_world();
    T_ASSERT(run_test_jass(
        "globals\n unit indexed = null\nendglobals\n"
        "function main takes nothing returns nothing\n"
        " set indexed = CreateUnit(Player(0), 'hfoo', 64.0, 64.0, 0.0)\nendfunction\n"
        "function choose takes nothing returns nothing\n"
        " call SetUnitAnimationByIndex(indexed, 3)\n call PauseUnit(indexed, true)\nendfunction\n"
        "function invalid takes nothing returns nothing\n"
        " call SetUnitAnimationByIndex(indexed, -1)\n call SetUnitAnimationByIndex(indexed, 999)\n"
        " call SetUnitAnimationByIndex(null, 0)\nendfunction\n"));
    edict_t *unit = NULL;
    FOR_LOOP(i, globals.num_edicts)
        if (globals.edicts[i].inuse && globals.edicts[i].class_id == MAKEFOURCC('h','f','o','o')) unit = globals.edicts + i;
    T_NOT_NULL(unit); if (!unit) return;
    unit->s.model = G_RegisterModel("Units/Creeps/Medivh/Medivh.mdx");
    jass_callbyname(level.vm, "choose", false);
    T_NOT_NULL(unit->animation); if (!unit->animation) return;
    T_STREQ(unit->animation->name, "Morph Alternate"); T_EQ(unit->s.frame, 3000);
    jass_callbyname(level.vm, "invalid", false);
    T_EQ(unit->s.frame, 3000); T_ASSERT(!jass_rterror_pending(level.vm));
    G_RunEntities(); T_ASSERT(unit->s.frame > 3000 && unit->s.frame < 3999);
    cstring_t file = Test_TempPath("native-animation-index.sav"); uint32_t number = unit->s.number;
    T_ASSERT(WriteGame(file)); T_ASSERT(ReadGame(file)); unit = globals.edicts + number;
    T_NOT_NULL(unit->animation);
    if (unit->animation) T_STREQ(unit->animation->name, "Morph Alternate");
    T_EQ(unit->animation_index, 4); remove(file);
}

/* ORDR routes through GE[GetHandleId(GetTriggerEventId())]. Two ranks are
 * queued before dispatch: learned rank must reflect the event, not latest state. */
TEST(wc3_native_completion, learned_skill_context_survives_yield_and_save) {
    cstring_t file = Test_TempPath("native-learned-skill.sav"); setup_test_world();
    T_ASSERT(run_test_jass(
        "globals\n unit learner = null\n integer completed = 0\n trigger nested = null\n"
        " trigger array routes\n integer routed = 0\n"
        " constant playerunitevent EVENT_PLAYER_HERO_SKILL = ConvertPlayerUnitEvent(42)\nendglobals\n"
        "function nestedCheck takes nothing returns boolean\n"
        " call BJassAssert(GetTriggerEventId() == EVENT_PLAYER_HERO_SKILL, \"nested event id\")\n"
        " call BJassAssert(GetLearnedSkill() == 'AHhb', \"nested skill id\")\n return true\nendfunction\n"
        "function routeCheck takes nothing returns boolean\n"
        " set routed = routed + 1\n return TriggerEvaluate(nested)\nendfunction\n"
        "function learned takes nothing returns nothing\n local integer rank = GetLearnedSkillLevel()\n"
        " call BJassAssert(GetTriggerEventId() == EVENT_PLAYER_HERO_SKILL, \"skill event id\")\n"
        " call BJassAssert(GetLearnedSkill() == 'AHhb', \"skill rawcode\")\n"
        " call BJassAssert(GetLearningUnit() == learner and GetTriggerPlayer() == Player(0), \"learner context\")\n"
        " call BJassAssert(rank == 1 or rank == 2, \"rank snapshot\")\n"
        " call TriggerEvaluate(routes[GetHandleId(GetTriggerEventId())])\n call TriggerSleepAction(0.10)\n"
        " call BJassAssert(GetTriggerEventId() == EVENT_PLAYER_HERO_SKILL, \"resumed event id\")\n"
        " call BJassAssert(GetLearnedSkillLevel() == rank, \"resumed rank\")\n"
        " set completed = completed + rank\nendfunction\n"
        "function main takes nothing returns nothing\n local trigger t = CreateTrigger()\n"
        " set nested = CreateTrigger()\n call TriggerAddCondition(nested, Condition(function nestedCheck))\n"
        " call BJassAssert(GetHandleId(ConvertGameEvent(0)) == 0, \"zero enum handle id\")\n"
        " call BJassAssert(GetHandleId(EVENT_PLAYER_HERO_SKILL) == 42, \"event enum handle id\")\n"
        " set routes[GetHandleId(EVENT_PLAYER_HERO_SKILL)] = CreateTrigger()\n"
        " call TriggerAddCondition(routes[GetHandleId(EVENT_PLAYER_HERO_SKILL)], Condition(function routeCheck))\n"
        " call TriggerRegisterPlayerUnitEvent(t, Player(0), EVENT_PLAYER_HERO_SKILL, null)\n"
        " call TriggerAddAction(t, function learned)\n"
        " set learner = CreateUnit(Player(0), 'Hpal', 0.0, 0.0, 0.0)\n call SetHeroLevel(learner, 5, false)\n"
        " call SelectHeroSkill(learner, 'AHhb')\n call SelectHeroSkill(learner, 'AHhb')\nendfunction\n"
        "function verify takes nothing returns nothing\n"
        " call BJassAssert(completed == 3, \"both ranks must complete once\")\n"
        " call BJassAssert(routed == 2, \"event id array routing\")\n"
        " call BJassAssert(GetTriggerEventId() == null, \"event leaked to root\")\nendfunction\n"));
    level.started = true; level.scriptsStarted = true; globals.RunFrame();
    T_ASSERT(!jass_rterror_pending(level.vm)); T_ASSERT(WriteGame(file)); T_ASSERT(ReadGame(file));
    level.time += 200; globals.RunFrame(); jass_callbyname(level.vm, "verify", false);
    if (jass_rterror_pending(level.vm)) fprintf(stderr, "%s\n", jass_rterror_message(level.vm));
    T_ASSERT(!jass_rterror_pending(level.vm)); remove(file);
}

TEST(wc3_native_completion, destroy_trigger_unsubscribes_and_allows_self_destruction) {
    cstring_t file = Test_TempPath("native-destroy-trigger.sav"); setup_test_world();
    T_ASSERT(run_test_jass(
        "globals\n trigger doomed = null\n trigger self = null\n integer calls = 0\n timer external = null\nendglobals\n"
        "function action takes nothing returns nothing\n set calls = calls + 1\nendfunction\n"
        "function destroyCondition takes nothing returns boolean\n"
        " call DestroyTrigger(GetTriggeringTrigger())\n return true\nendfunction\n"
        "function main takes nothing returns nothing\n"
        " set doomed = CreateTrigger()\n set self = CreateTrigger()\n set external = CreateTimer()\n"
        " call TimerStart(external, 1.0, true, null)\n call TriggerRegisterTimerEvent(doomed, 0.01, true)\n"
        " call TriggerRegisterTimerExpireEvent(doomed, external)\n"
        " call TriggerAddAction(doomed, function action)\n call TriggerAddAction(self, function action)\n"
        " call TriggerAddCondition(self, Condition(function destroyCondition))\n call TriggerEvaluate(self)\n"
        " call DestroyTrigger(doomed)\n call DestroyTrigger(doomed)\n call DestroyTrigger(null)\n"
        " call EnableTrigger(doomed)\n call TriggerExecute(doomed)\n call TriggerExecute(self)\n"
        " call BJassAssert(not IsTriggerEnabled(doomed), \"destroyed trigger enabled\")\n"
        " call BJassAssert(calls == 0, \"destroyed trigger executed\")\nendfunction\n"
        "function verify takes nothing returns nothing\n"
        " call TriggerExecute(doomed)\n call BJassAssert(calls == 0, \"destroyed trigger revived after load\")\nendfunction\n"));
    T_EQ(level.num_timers, 2); T_ASSERT(level.timers[0].running); T_ASSERT(!level.timers[1].running);
    FOR_EACH_EVENT(reg) T_ASSERT(reg->trigger != &level.triggers[0]);
    T_ASSERT(WriteGame(file)); T_ASSERT(ReadGame(file)); jass_callbyname(level.vm, "verify", false);
    T_ASSERT(!jass_rterror_pending(level.vm)); remove(file);
}

/* Real client select command -> real frame scheduler -> original map router. */
TEST(wc3_native_completion, neutral_selection_routes_by_event_id) {
    setup_test_world();
    T_ASSERT(run_test_jass(
        "globals\n unit button = null\n trigger array routes\n integer clicks = 0\nendglobals\n"
        "function picked takes nothing returns boolean\n"
        " call BJassAssert(GetTriggerEventId() == EVENT_PLAYER_UNIT_SELECTED, \"nested selection id\")\n"
        " call BJassAssert(GetTriggerPlayer() == Player(0), \"neutral button clicker\")\n"
        " call BJassAssert(GetTriggerUnit() == button, \"selected button\")\n"
        " set clicks = clicks + 1\n return true\nendfunction\n"
        "function route takes nothing returns nothing\n"
        " call TriggerEvaluate(routes[GetHandleId(GetTriggerEventId())])\nendfunction\n"
        "function main takes nothing returns nothing\n local trigger t = CreateTrigger()\n"
        " set button = CreateUnit(Player(15), 'hfoo', 0.0, 0.0, 0.0)\n"
        " set routes[GetHandleId(EVENT_PLAYER_UNIT_SELECTED)] = CreateTrigger()\n"
        " call TriggerAddCondition(routes[GetHandleId(EVENT_PLAYER_UNIT_SELECTED)], Condition(function picked))\n"
        " call TriggerRegisterPlayerUnitEvent(t, Player(0), EVENT_PLAYER_UNIT_SELECTED, null)\n"
        " call TriggerAddAction(t, function route)\nendfunction\n"
        "function verify takes nothing returns nothing\n"
        " call BJassAssert(clicks == 1, \"selection routed exactly once\")\nendfunction\n"));
    edict_t *button = NULL;
    FOR_LOOP(i, globals.num_edicts)
        if (g_edicts[i].inuse && g_edicts[i].class_id == MAKEFOURCC('h','f','o','o') &&
            g_edicts[i].s.player == 15) { button = &g_edicts[i]; break; }
    T_NOT_NULL(button); if (!button) return;
    game.clients[0].ps.rdflags |= RDF_NOFOG; game.clients[0].connected = true;
    g_edicts[0].inuse = true; g_edicts[0].client = &game.clients[0];
    T_ASSERT(G_UnitCanBeSelected(&game.clients[0], button));
    char number[24]; snprintf(number, sizeof(number), "%u", (unsigned)button->s.number);
    cstring_t command[] = { "select", number }; G_ClientCommand(&g_edicts[0], 2, command);
    level.started = true; level.scriptsStarted = true; globals.RunFrame();
    jass_callbyname(level.vm, "verify", false);
    if (jass_rterror_pending(level.vm)) fprintf(stderr, "%s\n", jass_rterror_message(level.vm));
    T_ASSERT(!jass_rterror_pending(level.vm));
}
TEST(wc3_native_completion, selection_natives_change_authoritative_flags) {
    setup_test_world();
    T_ASSERT(run_test_jass(
        "function main takes nothing returns nothing\n"
        " call EnableSelect(false, true)\n"
        " call EnableDragSelect(false, true)\n"
        " call EnablePreSelect(false, true)\n"
        "endfunction\n"
        "function restore takes nothing returns nothing\n"
        " call EnableSelect(true, true)\n"
        " call EnableDragSelect(true, true)\n"
        " call EnablePreSelect(true, true)\n"
        "endfunction\n"));
    T_ASSERT(level.selection_disabled[0]);
    T_ASSERT(level.drag_selection_disabled[0]);
    T_ASSERT(level.preselection_disabled[0]);
    jass_callbyname(level.vm, "restore", false);
    T_ASSERT(!level.selection_disabled[0]);
    T_ASSERT(!level.drag_selection_disabled[0]);
    T_ASSERT(!level.preselection_disabled[0]);
}
/* Count, dispel and corpse suspension must mutate the real unit lifecycle,
 * not just return an empty JASS placeholder. */
TEST(wc3_native_completion, buff_dispersal_and_decay_suspend_natives) {
    setup_test_world();
    T_ASSERT(run_test_jass(
        "globals\n"
        " unit tested = null\n"
        "endglobals\n"
        "function main takes nothing returns nothing\n"
        " set tested = CreateUnit(Player(0), 'hfoo', 0.0, 0.0, 0.0)\n"
        " call UnitApplyTimedLife(tested, 'BTLF', 10.0)\n"
        " call BJassAssert(UnitCountBuffsEx(tested, true, true, true, true, true, false, false) >= 1, \"timed life exists\")\n"
        " call UnitRemoveBuffs(tested, true, true)\n"
        " call BJassAssert(UnitCountBuffsEx(tested, true, true, true, true, true, false, false) >= 1, \"ordinary dispel preserves timed life\")\n"
        " call UnitRemoveBuffsEx(tested, true, true, true, true, true, false, false)\n"
        " call BJassAssert(UnitCountBuffsEx(tested, true, true, true, true, true, false, false) == 0, \"timed life removed\")\n"
        " call UnitSuspendDecay(tested, true)\n"
        "endfunction\n"
        "function release takes nothing returns nothing\n"
        " call UnitSuspendDecay(tested, false)\n"
        "endfunction\n"));
    edict_t *unit = NULL;
    FOR_LOOP(i, globals.num_edicts)
        if (g_edicts[i].inuse && g_edicts[i].class_id == MAKEFOURCC('h','f','o','o') &&
            g_edicts[i].s.player == 0) { unit = &g_edicts[i]; break; }
    T_NOT_NULL(unit);
    if (!unit) return;
    T_ASSERT((unit->aiflags & AI_CORPSE_DECAY_SUSPENDED) != 0);
    jass_callbyname(level.vm, "release", false);
    T_ASSERT((unit->aiflags & AI_CORPSE_DECAY_SUSPENDED) == 0);
    T_ASSERT(!jass_rterror_pending(level.vm));
}
#endif
