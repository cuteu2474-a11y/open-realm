#ifdef BZ_TESTS
#include "test.h"
#include "../g_local.h"
#include "../hud/hud_local.h"
#include "jass/jass.h"

bool run_test_jass(cstring_t source);
bool run_test_jass_error(cstring_t source, cstring_t expected);
void setup_test_world(void);

static struct {
    uiFrame_t frame;
    uint8_t data[4096];
    char text[4096];
} dz_test_frames[256];
static uint32_t dz_test_frame_count, dz_test_packets[MAX_PLAYERS];
static uint32_t dz_test_layer, dz_test_write_stage;
static bool dz_test_in_console;
static void (*dz_test_original_write)(pfWriteType_t, void const *);
static void (*dz_test_original_unicast)(edict_t *);

static void dz_test_write(pfWriteType_t type, void const *data) {
    if (type == PF_BYTE) {
        if (dz_test_write_stage == 1) {
            dz_test_in_console = *(int32_t const *)data == LAYER_CONSOLE;
            if (dz_test_in_console) dz_test_layer = LAYER_CONSOLE;
            dz_test_write_stage = 0;
        } else if (*(int32_t const *)data == svc_layout) dz_test_write_stage = 1;
    }
    if (type != PF_UIFRAME || !dz_test_in_console || dz_test_frame_count >= 256) return;
    uint32_t slot = dz_test_frame_count++;
    uiFrame_t const *frame = data;
    dz_test_frames[slot].frame = *frame;
    strlcpy(dz_test_frames[slot].text, frame->text ? frame->text : "", sizeof(dz_test_frames[slot].text));
    dz_test_frames[slot].frame.text = dz_test_frames[slot].text;
    if (frame->buffer.size <= sizeof(dz_test_frames[slot].data)) {
        memcpy(dz_test_frames[slot].data, frame->buffer.data, frame->buffer.size);
        dz_test_frames[slot].frame.buffer.data = dz_test_frames[slot].data;
    }
}

static void dz_test_unicast(edict_t *ent) {
    if (dz_test_in_console && ent && ent->client && ent->client->ps.number < MAX_PLAYERS)
        dz_test_packets[ent->client->ps.number]++;
    dz_test_in_console = false;
}

static void dz_test_capture_begin(void) {
    dz_test_original_write = gi.Write; dz_test_original_unicast = gi.unicast;
    dz_test_frame_count = dz_test_write_stage = 0;
    dz_test_layer = UINT32_MAX; dz_test_in_console = false;
    memset(dz_test_packets, 0, sizeof(dz_test_packets));
    gi.Write = dz_test_write; gi.unicast = dz_test_unicast;
}

static void dz_test_capture_end(void) {
    gi.Write = dz_test_original_write; gi.unicast = dz_test_original_unicast;
}

static void dz_test_tick(void) {
    level.started = level.scriptsStarted = true;
    globals.RunFrame();
}

static uiFrame_t *dz_test_text(cstring_t text) {
    FOR_LOOP(i, dz_test_frame_count) if (!strcmp(dz_test_frames[i].text, text)) return &dz_test_frames[i].frame;
    return NULL;
}

#define ORDR_FRAME_TEST_NATIVES \
    "native DzGetGameUI takes nothing returns integer\n" \
    "native DzCreateFrameByTagName takes string kind, string name, integer parent, string template, integer context returns integer\n" \
    "native DzCreateFrame takes string name, integer parent, integer context returns integer\n" \
    "native DzFrameFindByName takes string name, integer context returns integer\n" \
    "native DzFrameSetText takes integer frame, string text returns nothing\n" \
    "native DzFrameGetText takes integer frame returns string\n" \
    "native DzFrameSetSize takes integer frame, real width, real height returns nothing\n" \
    "native DzFrameSetPoint takes integer frame, integer point, integer relative, integer relativePoint, real x, real y returns nothing\n" \
    "native DzFrameSetAbsolutePoint takes integer frame, integer point, real x, real y returns nothing\n" \
    "native DzFrameClearAllPoints takes integer frame returns nothing\n" \
    "native DzFrameSetTexture takes integer frame, string path, integer flag returns nothing\n" \
    "native DzFrameSetFont takes integer frame, string path, real height, integer flags returns nothing\n" \
    "native DzFrameSetAlpha takes integer frame, integer alpha returns nothing\n" \
    "native DzFrameSetEnable takes integer frame, boolean enabled returns nothing\n" \
    "native DzFrameShow takes integer frame, boolean shown returns nothing\n"

#define ORDR_FRAME_TEST_SCENE \
    ORDR_FRAME_TEST_NATIVES \
    "globals\ninteger DzTestLabel = 0\ninteger DzTestBackground = 0\nendglobals\n" \
    "function main takes nothing returns nothing\n" \
    "local integer root = DzGetGameUI()\n" \
    "set DzTestLabel = DzCreateFrameByTagName(\"TEXT\", \"ORDRTestLabel\", root, \"\", 0)\n" \
    "set DzTestBackground = DzCreateFrameByTagName(\"BACKDROP\", \"ORDRTestPanel\", root, \"\", 0)\n" \
    "call DzFrameSetText(DzTestLabel, \"global frame text\")\n" \
    "call DzFrameSetSize(DzTestBackground, 0.23, 0.17)\n" \
    "call DzFrameSetTexture(DzTestBackground, \"TestUI\\\\Textures\\\\solid_white.blp\", 0)\n" \
    "call DzFrameSetAbsolutePoint(DzTestBackground, 4, 0.4, 0.35)\n" \
    "call DzFrameSetAbsolutePoint(DzTestLabel, 4, 0.2, 0.2)\n" \
    "call DzFrameClearAllPoints(DzTestLabel)\n" \
    "call DzFrameSetPoint(DzTestLabel, 0, DzTestBackground, 0, 0.01, -0.02)\n" \
    "call DzFrameSetFont(DzTestLabel, \"Fonts\\\\FRIZQT__.TTF\", 0.014, 0)\n" \
    "call DzFrameSetAlpha(DzTestLabel, 133)\n" \
    "call DzFrameSetEnable(DzTestLabel, false)\n" \
    "endfunction\n" \
    "function local_changes takes nothing returns nothing\n" \
    "if GetLocalPlayer() == Player(0) then\n" \
    "call DzFrameSetText(DzTestLabel, \"local frame text\")\n" \
    "call DzFrameShow(DzTestBackground, false)\n" \
    "endif\n" \
    "endfunction\n" \
    "function global_changes takes nothing returns nothing\n" \
    "call DzFrameSetText(DzTestLabel, \"updated frame text\")\n" \
    "call DzFrameSetEnable(DzTestLabel, true)\n" \
    "call DzFrameSetAlpha(DzTestLabel, 999)\n" \
    "call DzFrameShow(DzTestBackground, true)\n" \
    "endfunction\n"

TEST(wc3_ordr_frames, jass_creates_real_frame_and_resolves_context) {
    UI_ResetHud();
    T_ASSERT(run_test_jass(ORDR_FRAME_TEST_NATIVES
        "function main takes nothing returns nothing\n"
        "local integer root = DzGetGameUI()\n"
        "local integer label = DzCreateFrameByTagName(\"TEXT\", \"ORDRTestLabel\", root, \"\", 7)\n"
        "call DzFrameSetText(label, \"ORDR frame text\")\n"
        "if root != 0 and label != 0 and label == DzFrameFindByName(\"ORDRTestLabel\", 7) and DzFrameFindByName(\"ORDRTestLabel\", 8) == 0 and DzFrameGetText(label) == \"ORDR frame text\" then\n"
        "call SetPlayerState(Player(0), ConvertPlayerState(1), 23)\n"
        "endif\n"
        "endfunction\n"));
    T_EQ(game.clients[0].ps.stats[1], 23);
    frameDef_t *label = UI_FindFrame("ORDRTestLabel");
    T_NOT_NULL(label);
    if (!label) return;
    T_EQ(label->Type, FT_TEXT);
    T_STREQ(label->Text, "ORDR frame text");
    UI_ResetHud();
}

TEST(wc3_ordr_frames, console_snapshot_contains_art_fonts_forward_anchors_and_hidden_geometry) {
    setup_test_world();
    UI_ResetHud(); UI_LoadHudConsole();
    T_ASSERT(run_test_jass(ORDR_FRAME_TEST_SCENE));
    game.clients[0].connected = true;
    dz_test_capture_begin();
    dz_test_tick();
    dz_test_capture_end();
    T_EQ(dz_test_layer, LAYER_CONSOLE);
    T_EQ(dz_test_packets[0], 1);
    uiFrame_t *label = dz_test_text("global frame text");
    T_NOT_NULL(label);
    if (!label) { UI_ResetHud(); return; }
    uint32_t label_number = label->number;
    uint32_t panel_number = label->points.x[FPP_MIN].relativeTo;
    T_ASSERT(panel_number > label_number); /* late-created frame is a valid anchor */
    T_EQ(label->points.y[FPP_MIN].relativeTo, panel_number);
    T_ASSERT(!label->points.x[FPP_MID].used && !label->points.y[FPP_MID].used);
    T_FEQ(label->points.x[FPP_MIN].offset / UI_FRAMEPOINT_SCALE, 0.01f, 0.0001f);
    T_FEQ(label->points.y[FPP_MIN].offset / UI_FRAMEPOINT_SCALE, -0.02f, 0.0001f);
    T_EQ(label->color.a, 133);
    T_ASSERT(label->flagsvalue & UIFRAME_DISABLED);
    T_EQ(label->buffer.size, sizeof(uiLabel_t));
    T_ASSERT(((uiLabel_t *)label->buffer.data)->font != 0);
    uiFrame_t *panel = &dz_test_frames[panel_number - 1].frame;
    T_EQ(panel->number, panel_number);
    T_EQ(panel->flags.type, FT_BACKDROP);
    T_FEQ(panel->size.width, 0.23f, 0.0001f);
    T_FEQ(panel->size.height, 0.17f, 0.0001f);
    T_FEQ(panel->points.x[FPP_MID].offset / UI_FRAMEPOINT_SCALE, 0.4f, 0.0001f);
    T_FEQ(panel->points.y[FPP_MID].offset / UI_FRAMEPOINT_SCALE, 0.35f, 0.0001f);
    T_EQ(panel->points.y[FPP_MID].targetPos, FPP_MAX); /* absolute WC3 origin is bottom-left */
    T_ASSERT(((uiBackdrop_t *)panel->buffer.data)->Background != 0);
    T_ASSERT(!((uiBackdrop_t *)panel->buffer.data)->Opaque);

    /* The production resource cache must not resend an unchanged snapshot. */
    dz_test_capture_begin(); dz_test_tick(); dz_test_capture_end();
    T_EQ(dz_test_frame_count, 0);
    jass_callbyname(level.vm, "local_changes", true); jass_runevents(level.vm);
    dz_test_capture_begin(); dz_test_tick(); dz_test_capture_end();
    label = dz_test_text("local frame text");
    T_NOT_NULL(label);
    panel = &dz_test_frames[panel_number - 1].frame;
    T_EQ(panel->flags.type, FT_FRAME);
    T_FEQ(panel->size.width, 0.23f, 0.0001f);
    T_EQ(panel->buffer.size, 0);
    T_EQ(dz_test_packets[0], 1);
    T_EQ(dz_test_packets[1], 0);

    /* Reconnecting another recipient rebuilds the stock console and global
     * custom properties; player 0's hidden art/text never leaks. */
    game.clients[1].connected = true; game.clients[1].resourcebar.gold = -1;
    dz_test_capture_begin(); G_RefreshResourceBar(g_edicts + 1); dz_test_capture_end();
    T_NOT_NULL(dz_test_text("global frame text"));
    T_NULL(dz_test_text("local frame text"));
    panel = &dz_test_frames[panel_number - 1].frame;
    T_EQ(panel->flags.type, FT_BACKDROP);

    jass_callbyname(level.vm, "global_changes", true); jass_runevents(level.vm);
    dz_test_capture_begin(); dz_test_tick(); dz_test_capture_end();
    T_EQ(dz_test_packets[0], 1); T_EQ(dz_test_packets[1], 1);
    label = dz_test_text("updated frame text");
    T_NOT_NULL(label);
    if (label) { T_EQ(label->color.a, 255); T_ASSERT(!(label->flagsvalue & UIFRAME_DISABLED)); }
    T_EQ(dz_test_frames[panel_number - 1].frame.flags.type, FT_BACKDROP);
    UI_ResetHud();
    T_ASSERT(!G_DzFramesActive());
}

TEST(wc3_ordr_frames, fdf_template_children_are_owned_instances_with_context_lookup) {
    setup_test_world();
    UI_ResetHud();
    UI_ParseFDF("TestUI\\Frames\\ordr_compat.fdf");
    T_ASSERT(run_test_jass(ORDR_FRAME_TEST_NATIVES
        "function main takes nothing returns nothing\n"
        "local integer root = DzGetGameUI()\n"
        "local integer a = DzCreateFrame(\"ORDRCompatPanel\", root, 3)\n"
        "local integer b = DzCreateFrame(\"ORDRCompatPanel\", root, 4)\n"
        "call DzFrameSetText(DzFrameFindByName(\"ORDRCompatLabel\", 3), \"first instance\")\n"
        "call DzFrameSetText(DzFrameFindByName(\"ORDRCompatLabel\", 4), \"second instance\")\n"
        "if a != b and DzFrameGetText(DzFrameFindByName(\"ORDRCompatLabel\", 3)) == \"first instance\" then\n"
        "call SetPlayerState(Player(0), ConvertPlayerState(1), 37)\n"
        "endif\nendfunction\n"));
    T_EQ(game.clients[0].ps.stats[1], 37);
    frameDef_t *template_label = UI_FindFrame("ORDRCompatLabel");
    T_NOT_NULL(template_label);
    if (template_label) T_STREQ(template_label->Text, "template text");
    game.clients[0].connected = true;
    dz_test_capture_begin(); dz_test_tick(); dz_test_capture_end();
    uiFrame_t *a = dz_test_text("first instance"), *b = dz_test_text("second instance");
    T_NOT_NULL(a); T_NOT_NULL(b);
    if (a && b) { T_ASSERT(a->parent != b->parent); T_EQ(a->points.x[FPP_MIN].relativeTo, a->parent); }
    UI_ResetHud();
}

TEST(wc3_ordr_frames, local_font_and_anchor_changes_preserve_other_properties) {
    setup_test_world();
    UI_ResetHud();
    T_ASSERT(run_test_jass(ORDR_FRAME_TEST_NATIVES
        "function main takes nothing returns nothing\n"
        "local integer root = DzGetGameUI()\n"
        "local integer label = DzCreateFrameByTagName(\"TEXT\", \"LocalPropertyLabel\", root, \"\", 0)\n"
        "call DzFrameSetText(label, \"local property regression\")\n"
        "call DzFrameSetSize(label, 0.2, 0.1)\n"
        "call DzFrameSetPoint(label, 0, root, 0, 0.01, -0.02)\n"
        "if GetLocalPlayer() == Player(0) then\n"
        "call DzFrameSetFont(label, \"Fonts\\\\FRIZQT__.TTF\", 0.014, 0)\n"
        "call DzFrameSetPoint(label, 8, root, 8, -0.01, 0.02)\n"
        "endif\n"
        "call DzFrameSetAlpha(label, 0)\n"
        "endfunction\n"
        "function resize takes nothing returns nothing\n"
        "call DzFrameSetSize(DzFrameFindByName(\"LocalPropertyLabel\", 0), 0.3, 0.15)\n"
        "endfunction\n"));
    game.clients[0].connected = true;
    dz_test_capture_begin(); G_DzFramesUpdate(); dz_test_capture_end();
    uiFrame_t *label = dz_test_text("local property regression");
    T_NOT_NULL(label);
    if (label) {
        T_EQ(label->color.a, 0);
        T_ASSERT(label->points.x[FPP_MIN].used && label->points.x[FPP_MAX].used);
        T_ASSERT(label->points.y[FPP_MIN].used && label->points.y[FPP_MAX].used);
        T_FEQ(label->size.width, 0.0f, 0.0001f);
        /* Auto-sized single-line text uses its font line box on the wire. */
        T_FEQ(label->size.height, 0.014f, 0.0001f);
    }
    game.clients[1].connected = true; game.clients[1].resourcebar.gold = -1;
    dz_test_capture_begin(); G_RefreshResourceBar(g_edicts + 1); dz_test_capture_end();
    label = dz_test_text("local property regression");
    T_NOT_NULL(label);
    if (label) {
        T_EQ(label->color.a, 0);
        T_ASSERT(label->points.x[FPP_MIN].used && !label->points.x[FPP_MAX].used);
        T_FEQ(label->size.width, 0.2f, 0.0001f);
        T_FEQ(label->size.height, 0.1f, 0.0001f);
    }
    jass_callbyname(level.vm, "resize", true); jass_runevents(level.vm);
    game.clients[1].connected = false;
    dz_test_capture_begin(); G_DzFramesUpdate(); dz_test_capture_end();
    label = dz_test_text("local property regression");
    T_NOT_NULL(label);
    if (label) {
        T_FEQ(label->size.width, 0.3f, 0.0001f);
        T_FEQ(label->size.height, 0.15f, 0.0001f);
    }
    UI_ResetHud();
}

TEST(wc3_ordr_frames, stock_and_custom_frames_share_bounded_anchor_namespace) {
    setup_test_world();
    UI_ResetHud(); UI_LoadHudConsole();
    T_ASSERT(run_test_jass_error(ORDR_FRAME_TEST_NATIVES
        "function main takes nothing returns nothing\n"
        "local integer root = DzGetGameUI()\n"
        "local integer i = 0\n"
        "local integer frame = 0\n"
        "loop\nexitwhen i == 253\n"
        "set frame = DzCreateFrameByTagName(\"TEXT\", \"BudgetLabel\", root, \"\", 0)\n"
        "call DzFrameSetText(frame, \"budget frame\")\n"
        "set i = i + 1\nendloop\nendfunction\n",
        "DzCreateFrame: console plus custom frames exceed HUD wire limit of 254"));
    game.clients[0].connected = true;
    dz_test_capture_begin(); G_DzFramesUpdate(); dz_test_capture_end();
    T_EQ(dz_test_packets[0], 1);
    T_NOT_NULL(dz_test_text("budget frame"));
    T_ASSERT(dz_test_frame_count > 200 && dz_test_frame_count < UI_PARENT);
    FOR_LOOP(i, dz_test_frame_count) {
        T_ASSERT(dz_test_frames[i].frame.number > 0 && dz_test_frames[i].frame.number < UI_PARENT);
        FOR_LOOP(p, FPP_COUNT) {
            uiFramePoint_t x = dz_test_frames[i].frame.points.x[p], y = dz_test_frames[i].frame.points.y[p];
            T_ASSERT(!x.used || x.relativeTo == UI_PARENT || x.relativeTo <= dz_test_frame_count);
            T_ASSERT(!y.used || y.relativeTo == UI_PARENT || y.relativeTo <= dz_test_frame_count);
        }
    }
    UI_ResetHud();
}

TEST(wc3_ordr_frames, save_rejects_live_ui_before_overwriting_existing_file) {
    cstring_t path = Test_TempPath("ordr-live-ui-save.bin");
    UI_ResetHud();
    FILE *file = fopen(path, "wb"); T_NOT_NULL(file);
    if (!file) return;
    fputs("previous save contents", file); fclose(file);
    T_ASSERT(run_test_jass(ORDR_FRAME_TEST_SCENE));
    T_ASSERT(!WriteGame(path));
    file = fopen(path, "rb"); T_NOT_NULL(file);
    if (file) {
        char contents[64] = {0}; fread(contents, 1, sizeof(contents) - 1, file); fclose(file);
        T_STREQ(contents, "previous save contents");
    }
    UI_ResetHud();
    T_ASSERT(!G_DzFramesActive());
}

TEST(wc3_ordr_frames, invalid_anchor_reports_error_and_runtime_reset_expires_handles) {
    UI_ResetHud();
    T_ASSERT(run_test_jass_error(ORDR_FRAME_TEST_NATIVES
        "function main takes nothing returns nothing\n"
        "call DzFrameSetAbsolutePoint(DzGetGameUI(), 9, 0.2, 0.3)\nendfunction\n",
        "DzFrame: anchor must be in range 0..8"));
    T_ASSERT(run_test_jass(ORDR_FRAME_TEST_NATIVES
        "function main takes nothing returns nothing\n"
        "call SetPlayerState(Player(0), ConvertPlayerState(1), DzGetGameUI())\nendfunction\n"));
    int32_t expired = game.clients[0].ps.stats[1];
    T_ASSERT(expired > 0);
    UI_ResetHud();
    char source[4096];
    snprintf(source, sizeof(source), ORDR_FRAME_TEST_NATIVES
        "function main takes nothing returns nothing\n"
        "local integer fresh = DzGetGameUI()\n"
        "call SetPlayerState(Player(0), ConvertPlayerState(1), fresh)\n"
        "call DzFrameSetText(%d, \"expired\")\nendfunction\n", expired);
    T_ASSERT(run_test_jass_error(source, "DzFrame: invalid or expired frame handle"));
    T_ASSERT(game.clients[0].ps.stats[1] != expired);
    UI_ResetHud();
}
#endif
