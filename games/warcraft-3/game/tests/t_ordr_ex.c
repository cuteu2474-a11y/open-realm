#ifdef BZ_TESTS
#include "shared/test.h"
#include "../g_local.h"

void setup_test_world(void);
bool run_test_jass(cstring_t source);

/* Execute the signatures declared by the private map, including inverse pause
 * and degree-based facing. Registry pointer equality cannot prove VM arguments. */
TEST(wc3_ordr_ex, unit_aliases_execute_and_unpause_through_jass) {
    setup_test_world();
    T_ASSERT(run_test_jass(
        "native EXPauseUnit takes unit u, boolean flag returns nothing\n"
        "native EXSetUnitFacing takes unit u, real angle returns nothing\n"
        "function main takes nothing returns nothing\n"
        "local unit u = CreateUnit(Player(0), 'hfoo', 128.0, 128.0, 0.0)\n"
        "call BJassAssert(u != null, \"unit creation failed\")\n"
        "call EXPauseUnit(u, true)\n"
        "call BJassAssert(IsUnitPaused(u), \"EXPauseUnit did not pause\")\n"
        "call EXPauseUnit(u, false)\n"
        "call BJassAssert(not IsUnitPaused(u), \"EXPauseUnit did not resume\")\n"
        "call EXSetUnitFacing(u, 270.0)\n"
        "call BJassAssert(GetUnitFacing(u) > 269.99 and GetUnitFacing(u) < 270.01, \"EX facing units\")\n"
        "call RemoveUnit(u)\n"
        "endfunction\n"));
}
#endif
