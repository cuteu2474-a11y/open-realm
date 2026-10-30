#ifdef BZ_TESTS

#include "test.h"
#include "../g_local.h"
#include "common/stb_slk.h"
#include "jass/jass.h"

void setup_test_world(void);
bool run_test_jass(cstring_t src);

/* Models are presentation data, not the authoritative definition of a unit.
 * Missing custom UnitUI rows must not disable the wisp's event/move lifecycle. */
TEST(wc3_beacon, model_less_custom_wisp_is_an_active_unit) {
    uint32_t const id = MAKEFOURCC('e','0','I','X');
    slkTestData_t *rows = parse_slk_string(
        "ID;PWXL;N;E\nB;X3;Y2;D0\n"
        "C;X1;Y1;K\"unitBalanceID\"\nC;X2;K\"HP\"\nC;X3;K\"spd\"\n"
        "C;X1;Y2;K\"e0IX\"\nC;X2;K10\nC;X3;K522\nE\n");
    slkTestData_t *old;
    edict_t *wisp;

    setup_test_world();
    old = G_SetSLKRows("UnitBalance", rows);
    T_NOT_NULL(old);
    T_ASSERT(!G_UnitUI(id)->modelFile || !*G_UnitUI(id)->modelFile);
    wisp = unit_create(1, id, &MAKE(vec2_t, -3600.0f, -9350.0f), 0);
    T_NOT_NULL(wisp);
    T_ASSERT(wisp->s.flags & EF_UNIT);
    T_ASSERT(wisp->svflags & SVF_MONSTER);
    T_ASSERT(G_UnitIsWorldActive(wisp));
    T_FEQ(wisp->health.value, 10.0f, 0.001f);
    G_SetSLKRows("UnitBalance", old);
    free_slk_rows(rows);
}

/* Real CH bounds and real e0IX rawcode, but not the private imported terrain.
 * This covers the native enter-region -> JASS -> consumed wisp -> reward chain
 * after an authoritative position crossing, including the event subject. */
TEST(wc3_beacon, custom_wisp_enters_ordr_beacon_and_grants_once) {
    uint32_t const id = MAKEFOURCC('e','0','I','X');
    slkTestData_t *rows = parse_slk_string(
        "ID;PWXL;N;E\nB;X2;Y2;D0\n"
        "C;X1;Y1;K\"unitBalanceID\"\nC;X2;K\"HP\"\n"
        "C;X1;Y2;K\"e0IX\"\nC;X2;K10\nE\n");
    slkTestData_t *old;
    edict_t *wisp = NULL;
    vec2_t from;

    setup_test_world();
    old = G_SetSLKRows("UnitBalance", rows);
    T_NOT_NULL(old);
    T_ASSERT(run_test_jass(
        "globals\n unit wisp = null\n unit reward = null\n integer grants = 0\n endglobals\n"
        "function grant takes nothing returns nothing\n"
        " local unit entering = GetEnteringUnit()\n"
        " if GetUnitTypeId(entering) != 'e0IX' then\n return\n endif\n"
        " call BJassAssert(GetTriggerUnit() == entering, \"beacon subject\")\n"
        " set reward = CreateUnit(GetOwningPlayer(entering), 'hfoo', 0.0, 0.0, 0.0)\n"
        " call ShowUnit(entering, false)\n call KillUnit(entering)\n"
        " set grants = grants + 1\n"
        "endfunction\n"
        "function main takes nothing returns nothing\n"
        " local region beacon = CreateRegion()\n local trigger t = CreateTrigger()\n"
        " call RegionAddRect(beacon, Rect(-3584.0, -9504.0, -3328.0, -9216.0))\n"
        " call TriggerRegisterEnterRegion(t, beacon, null)\n"
        " call TriggerAddAction(t, function grant)\n"
        " set wisp = CreateUnit(Player(1), 'e0IX', -3600.0, -9350.0, 0.0)\n"
        "endfunction\n"
        "function verify takes nothing returns nothing\n"
        " call BJassAssert(grants == 1, \"exact beacon region did not fire once\")\n"
        " call BJassAssert(GetWidgetLife(wisp) <= 0.405, \"wisp must be consumed\")\n"
        " call BJassAssert(reward != null, \"wisp reward missing\")\n"
        " call BJassAssert(GetOwningPlayer(reward) == Player(1), \"reward owner\")\n"
        "endfunction\n"));
    FOR_LOOP(i, globals.num_edicts) {
        if (g_edicts[i].inuse && g_edicts[i].class_id == id) {
            wisp = &g_edicts[i];
            break;
        }
    }
    T_NOT_NULL(wisp);
    T_ASSERT(wisp->svflags & SVF_MONSTER);
    from = wisp->s.origin2;
    wisp->s.origin2 = MAKE(vec2_t, -3500.0f, -9350.0f);
    G_UnitPositionChanged(wisp, &from);
    G_RunEvents(); jass_runevents(level.vm);
    jass_callbyname(level.vm, "verify", false);
    T_ASSERT(!jass_rterror_pending(level.vm));
    G_RunEvents(); jass_runevents(level.vm);
    jass_callbyname(level.vm, "verify", false);
    T_ASSERT(!jass_rterror_pending(level.vm));
    G_SetSLKRows("UnitBalance", old);
    free_slk_rows(rows);
}

#endif
