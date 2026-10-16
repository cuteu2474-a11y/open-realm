/* Dz runtime frames use the game-owned FDF pool and the existing HUD wire
 * format. UI scripts/input are deliberately unregistered until their event
 * contracts are implemented. This is a rendering subset, not a DzAPI shim. */
#ifndef WC3_API_ORDR_FRAMES_H
#define WC3_API_ORDR_FRAMES_H

#include "../hud/hud_local.h"
#include <limits.h>

/* Frame-point references are bytes; 255 means parent. Keep every frame,
 * including hidden geometry and inherited control art, addressable. */
#define DZ_MAX_FRAMES (UI_PARENT - 1u)

enum {
    DZ_TEXT = 1u << 0, DZ_TEXTURE = 1u << 1, DZ_WIDTH = 1u << 2,
    DZ_POINTS = 1u << 3, DZ_FONT = 1u << 4, DZ_ALPHA = 1u << 5,
    DZ_ENABLED = 1u << 6, DZ_SHOWN = 1u << 7, DZ_HEIGHT = 1u << 8,
    DZ_SIZE = DZ_WIDTH | DZ_HEIGHT,
};

typedef struct {
    frameDef_t value;
    uint32_t fields;
} dzFrameLocal_t;

typedef struct {
    frameDef_t *frame;
    int32_t handle, context;
    dzFrameLocal_t *local[MAX_PLAYERS];
} dzFrame_t;

static dzFrame_t dz_frames[DZ_MAX_FRAMES];
static uint32_t dz_frame_count, dz_dirty_clients;
static int32_t dz_next_handle = 1;

void G_DzFramesReset(void) {
    FOR_LOOP(i, dz_frame_count) {
        FOR_LOOP(p, MAX_PLAYERS) {
            dzFrameLocal_t *local = dz_frames[i].local[p];
            if (!local) continue;
            /* Only DynamicText belongs to a local property record. Template
             * menu lists, parents, textures, and anchor pointers are borrowed. */
            if (local->value.DynamicText) gi.MemFree(local->value.DynamicText);
            gi.MemFree(local);
        }
    }
    memset(dz_frames, 0, sizeof(dz_frames));
    dz_frame_count = dz_dirty_clients = 0;
}

bool G_DzFramesActive(void) { return dz_frame_count != 0; }

static uint32_t DzFrameRecipients(void) {
    return currentplayer && PLAYER_NUM(currentplayer) < MAX_PLAYERS
        ? 1u << PLAYER_NUM(currentplayer) : UINT32_MAX;
}

static dzFrame_t *DzFindHandle(int32_t handle) {
    FOR_LOOP(i, dz_frame_count) if (dz_frames[i].handle == handle) return dz_frames + i;
    return NULL;
}

static dzFrame_t *DzCheckFrame(jass_t *j, int argument) {
    dzFrame_t *record = DzFindHandle(jass_checkinteger(j, argument));
    if (!record) jass_rterror(j, "DzFrame: invalid or expired frame handle");
    return record;
}

static frameDef_t DzFrameValue(dzFrame_t const *record, uint32_t player) {
    frameDef_t value = *record->frame;
    dzFrameLocal_t const *local = player < MAX_PLAYERS ? record->local[player] : NULL;
    uint32_t fields = local ? local->fields : 0;
    if (fields & DZ_TEXT) value.Text = local->value.Text;
    if (fields & DZ_TEXTURE) { value.Texture = local->value.Texture; value.Backdrop = local->value.Backdrop; }
    if (fields & DZ_WIDTH) value.Width = local->value.Width;
    if (fields & DZ_HEIGHT) value.Height = local->value.Height;
    if (fields & DZ_POINTS) { value.Points = local->value.Points; value.AnyPointsSet = true; }
    if (fields & DZ_FONT) {
        /* SetFont changes these three fields; color belongs to SetAlpha. */
        memcpy(value.Font.Name, local->value.Font.Name, sizeof(value.Font.Name));
        value.Font.Size = local->value.Font.Size;
        value.Font.Index = local->value.Font.Index;
    }
    if (fields & DZ_ALPHA) { value.Color.a = local->value.Color.a; value.Font.Color.a = local->value.Color.a; }
    if (fields & DZ_ENABLED) value.disabled = local->value.disabled;
    if (fields & DZ_SHOWN) value.hidden = local->value.hidden;
    return value;
}

static frameDef_t *DzMutableFrame(jass_t *j, dzFrame_t *record, uint32_t field) {
    dz_dirty_clients |= DzFrameRecipients();
    if (!currentplayer) {
        /* A synchronized assignment overrides earlier local assignments of
         * this property, just as it does on each retail player's UI. */
        FOR_LOOP(p, MAX_PLAYERS) if (record->local[p]) record->local[p]->fields &= ~field;
        return record->frame;
    }
    uint32_t player = PLAYER_NUM(currentplayer);
    if (player >= MAX_PLAYERS) {
        jass_rterror(j, "DzFrame: invalid local player");
        return NULL;
    }
    dzFrameLocal_t *local = record->local[player];
    if (!local) {
        local = gi.MemAlloc(sizeof(*local));
        if (!local) { jass_rterror(j, "DzFrame: local property allocation failed"); return NULL; }
        memset(local, 0, sizeof(*local));
        record->local[player] = local;
    }
    if (!(local->fields & field)) {
        if (field == DZ_POINTS) {
            local->value.Points = record->frame->Points;
            local->value.AnyPointsSet = record->frame->AnyPointsSet;
        }
        if (field == DZ_FONT) local->value.Font = record->frame->Font;
        if (field == DZ_TEXTURE) {
            local->value.Type = record->frame->Type;
            local->value.Texture = record->frame->Texture;
            local->value.Backdrop = record->frame->Backdrop;
        }
    }
    local->fields |= field;
    return &local->value;
}

static uint32_t DzFrameTreeCount(frameDef_t const *root) {
    uint32_t count = 1;
    FOR_LOOP(i, MAX_UI_CLASSES) if (frames[i].inuse && frames[i].Parent == root)
        count += DzFrameTreeCount(frames + i);
    return count;
}

/* Reserve the complete stock tree, including hidden wide chrome and embedded
 * control art, plus every possible native console proxy: clock and listener,
 * quest attention, tooltip, and minimap. The conservative bound remains valid
 * when a recipient changes canvas or the quest indicator becomes visible. */
static uint32_t DzConsoleFrameBudget(void) {
    return hud.console.ConsoleUI ? DzFrameTreeCount(hud.console.ConsoleUI) + 5 : 1;
}

static dzFrame_t *DzRegisterFrameTree(frameDef_t *root, int32_t context) {
    dzFrame_t *record = dz_frames + dz_frame_count++;
    record->frame = root;
    record->context = context;
    record->handle = dz_next_handle++;
    FOR_LOOP(i, MAX_UI_CLASSES) if (frames[i].inuse && frames[i].Parent == root)
        DzRegisterFrameTree(frames + i, context);
    return record;
}

static dzFrame_t *DzGameUI(jass_t *j) {
    if (dz_frame_count) return dz_frames;
    UI_LoadHudConsole();
    if (DzConsoleFrameBudget() >= DZ_MAX_FRAMES) {
        jass_rterror(j, "DzGetGameUI: stock console exhausts HUD wire frame budget"); return NULL;
    }
    if (dz_next_handle <= 0 || dz_next_handle >= INT32_MAX - DZ_MAX_FRAMES) {
        jass_rterror(j, "DzFrame: handle space exhausted"); return NULL;
    }
    frameDef_t *root = UI_Spawn(FT_FRAME, NULL);
    if (!root) { jass_rterror(j, "DzFrame: FDF pool exhausted"); return NULL; }
    UI_SetSize(root, UI_BASE_WIDTH, UI_BASE_HEIGHT);
    UI_SetPoint(root, FRAMEPOINT_TOPLEFT, NULL, FRAMEPOINT_TOPLEFT, 0, 0);
    dz_dirty_clients = UINT32_MAX;
    return DzRegisterFrameTree(root, 0);
}

static uint32_t DzGetGameUI(jass_t *j) { return jass_pushinteger(j, DzGameUI(j)->handle); }

static uint32_t DzCreateFrameCommon(jass_t *j, cstring_t kind, cstring_t name,
                                     int32_t parent_handle, cstring_t template, int32_t context) {
    static struct { cstring_t name; FRAMETYPE type; } const kinds[] = {
        { "FRAME", FT_FRAME }, { "TEXT", FT_TEXT }, { "BACKDROP", FT_BACKDROP },
        { "BUTTON", FT_BUTTON }, { "SPRITE", FT_SPRITE },
    };
    dzFrame_t *parent = DzFindHandle(parent_handle);
    FRAMETYPE type = FT_FRAME;
    bool found = kind == NULL;
    if (!parent || !name || strlen(name) >= sizeof(((frameDef_t *)0)->Name)) {
        jass_rterror(j, "DzCreateFrame: invalid parent or frame name"); return 0;
    }
    if (currentplayer) { jass_rterror(j, "DzCreateFrame: local-only frame creation is unsupported"); return 0; }
    if (kind) {
        FOR_LOOP(i, sizeof(kinds) / sizeof(kinds[0])) if (!strcmp(kind, kinds[i].name)) {
            type = kinds[i].type; found = true; break;
        }
    }
    if (!found) { jass_rterror(j, "DzCreateFrame: unsupported frame type"); return 0; }
    frameDef_t *source = template && *template ? UI_FindFrame(template) : NULL;
    if (template && *template && !source) { jass_rterror(j, "DzCreateFrame: FDF template not found"); return 0; }
    uint32_t count = source ? DzFrameTreeCount(source) : 1;
    uint32_t stock = DzConsoleFrameBudget();
    if (stock > DZ_MAX_FRAMES || dz_frame_count > DZ_MAX_FRAMES - stock ||
        count > DZ_MAX_FRAMES - stock - dz_frame_count) {
        jass_rterror(j, "DzCreateFrame: console plus custom frames exceed HUD wire limit of 254"); return 0;
    }
    if (dz_next_handle >= INT32_MAX - (int32_t)count) {
        jass_rterror(j, "DzCreateFrame: handle space exhausted"); return 0;
    }
    frameDef_t *frame = source ? UI_CloneFrameTree(source, parent->frame) : UI_Spawn(type, parent->frame);
    if (!frame) { jass_rterror(j, "DzCreateFrame: FDF pool exhausted"); return 0; }
    if (kind) frame->Type = type;
    strlcpy(frame->Name, name, sizeof(frame->Name));
    dzFrame_t *record = DzRegisterFrameTree(frame, context);
    dz_dirty_clients = UINT32_MAX;
    return jass_pushinteger(j, record->handle);
}

static uint32_t DzCreateFrame(jass_t *j) {
    cstring_t name = jass_checkstring(j, 1);
    return DzCreateFrameCommon(j, NULL, name, jass_checkinteger(j, 2), name, jass_checkinteger(j, 3));
}

static uint32_t DzCreateFrameByTagName(jass_t *j) {
    return DzCreateFrameCommon(j, jass_checkstring(j, 1), jass_checkstring(j, 2),
        jass_checkinteger(j, 3), jass_checkstring(j, 4), jass_checkinteger(j, 5));
}

static uint32_t DzFrameFindByName(jass_t *j) {
    cstring_t name = jass_checkstring(j, 1);
    int32_t context = jass_checkinteger(j, 2);
    FOR_LOOP(i, dz_frame_count) if (dz_frames[i].context == context && name &&
        !strcmp(dz_frames[i].frame->Name, name)) return jass_pushinteger(j, dz_frames[i].handle);
    return jass_pushinteger(j, 0);
}

static uint32_t DzFrameSetText(jass_t *j) {
    dzFrame_t *record = DzCheckFrame(j, 1);
    UI_SetText(DzMutableFrame(j, record, DZ_TEXT), "%s", jass_checkstring(j, 2));
    return 0;
}

static uint32_t DzFrameGetText(jass_t *j) {
    frameDef_t value = DzFrameValue(DzCheckFrame(j, 1), currentplayer ? PLAYER_NUM(currentplayer) : MAX_PLAYERS);
    return jass_pushstring(j, value.Text ? value.Text : "");
}

static uint32_t DzFrameSetSize(jass_t *j) {
    frameDef_t *frame = DzMutableFrame(j, DzCheckFrame(j, 1), DZ_SIZE);
    float width = jass_checknumber(j, 2), height = jass_checknumber(j, 3);
    if (!isfinite(width) || !isfinite(height) || width < 0 || height < 0)
        jass_rterror(j, "DzFrameSetSize: invalid dimensions");
    UI_SetSize(frame, width, height);
    return 0;
}

static UIFRAMEPOINT DzCheckPoint(jass_t *j, int index) {
    int32_t value = jass_checkinteger(j, index);
    if (value < 0 || value > 8) jass_rterror(j, "DzFrame: anchor must be in range 0..8");
    return (UIFRAMEPOINT)(value + value / 3);
}

static void DzSetPoint(jass_t *j, dzFrame_t *record, UIFRAMEPOINT point,
                       frameDef_t const *relative, UIFRAMEPOINT relative_point, float x, float y) {
    frameDef_t *frame = DzMutableFrame(j, record, DZ_POINTS);
    UI_SetPoint(frame, point, relative, relative_point, x, y);
    /* UI_SetPoint makes a spanned axis auto-sized. Preserve that side effect
     * in the same recipient scope as the anchors, independently for each axis. */
    if (frame->Points.x[FPP_MIN].used && frame->Points.x[FPP_MAX].used)
        DzMutableFrame(j, record, DZ_WIDTH)->Width = 0;
    if (frame->Points.y[FPP_MIN].used && frame->Points.y[FPP_MAX].used)
        DzMutableFrame(j, record, DZ_HEIGHT)->Height = 0;
}

static uint32_t DzFrameSetPoint(jass_t *j) {
    dzFrame_t *record = DzCheckFrame(j, 1);
    UIFRAMEPOINT point = DzCheckPoint(j, 2), relative_point = DzCheckPoint(j, 4);
    frameDef_t *relative = DzCheckFrame(j, 3)->frame;
    float x = jass_checknumber(j, 5), y = jass_checknumber(j, 6);
    if (!isfinite(x) || !isfinite(y) || fabsf(x) > 1 || fabsf(y) > 1)
        jass_rterror(j, "DzFrameSetPoint: offset exceeds HUD wire range");
    DzSetPoint(j, record, point, relative, relative_point, x, y);
    return 0;
}

static uint32_t DzFrameSetAbsolutePoint(jass_t *j) {
    dzFrame_t *record = DzCheckFrame(j, 1);
    UIFRAMEPOINT point = DzCheckPoint(j, 2);
    float x = jass_checknumber(j, 3), y = jass_checknumber(j, 4);
    if (!isfinite(x) || !isfinite(y) || fabsf(x) > 1 || fabsf(y) > 1)
        jass_rterror(j, "DzFrameSetAbsolutePoint: coordinates exceed HUD wire range");
    DzSetPoint(j, record, point, dz_frames[0].frame, FRAMEPOINT_BOTTOMLEFT, x, y);
    return 0;
}

static uint32_t DzFrameClearAllPoints(jass_t *j) {
    frameDef_t *frame = DzMutableFrame(j, DzCheckFrame(j, 1), DZ_POINTS);
    memset(&frame->Points, 0, sizeof(frame->Points)); frame->AnyPointsSet = true;
    return 0;
}

static uint32_t DzFrameSetTexture(jass_t *j) {
    frameDef_t *frame = DzMutableFrame(j, DzCheckFrame(j, 1), DZ_TEXTURE);
    uint32_t texture = UI_LoadTexture(jass_checkstring(j, 2), false);
    if (jass_checkinteger(j, 3) != 0) jass_rterror(j, "DzFrameSetTexture: nonzero texture flags are unsupported");
    frame->Texture.Image = texture;
    frame->Texture.TexCoord = MAKE(box2_t, .max = {1, 1});
    if (frame->Type == FT_BACKDROP) {
        frame->Backdrop.Background = texture;
        frame->Backdrop.BlendAll = true;
    }
    return 0;
}

static uint32_t DzFrameSetFont(jass_t *j) {
    frameDef_t *frame = DzMutableFrame(j, DzCheckFrame(j, 1), DZ_FONT);
    cstring_t path = jass_checkstring(j, 2);
    float height = jass_checknumber(j, 3);
    if (!path || !*path || !isfinite(height) || height <= 0 || height > 1 || jass_checkinteger(j, 4) != 0)
        jass_rterror(j, "DzFrameSetFont: invalid font or unsupported flags");
    strlcpy(frame->Font.Name, path, sizeof(frame->Font.Name));
    frame->Font.Size = height;
    frame->Font.Index = UI_FdfFontIndex(path, (uint32_t)(height * UI_FONT_COORD_SCALE));
    return 0;
}

static uint32_t DzFrameSetAlpha(jass_t *j) {
    frameDef_t *frame = DzMutableFrame(j, DzCheckFrame(j, 1), DZ_ALPHA);
    int32_t alpha = jass_checkinteger(j, 2);
    frame->Color.a = frame->Font.Color.a = MAX(0, MIN(255, alpha));
    return 0;
}

static uint32_t DzFrameSetEnable(jass_t *j) {
    frameDef_t *frame = DzMutableFrame(j, DzCheckFrame(j, 1), DZ_ENABLED);
    UI_SetEnabled(frame, jass_checkboolean(j, 2));
    return 0;
}

static uint32_t DzFrameShow(jass_t *j) {
    frameDef_t *frame = DzMutableFrame(j, DzCheckFrame(j, 1), DZ_SHOWN);
    UI_SetHidden(frame, !jass_checkboolean(j, 2));
    return 0;
}

/* Serialize a full replacement snapshot. Geometry-only entries keep hidden
 * frames addressable as anchors without rendering art or accepting input. */
void G_DzFramesWrite(edict_t *ent) {
    if (!dz_frame_count || !ent || !ent->client) return;
    uint32_t player = ent->client->ps.number;
    uint32_t first = ui_next_frame_number;
    /* The stock console and custom controls share one snapshot namespace. */
    if (first + dz_frame_count > UI_PARENT) {
        fprintf(stderr, "DzFrame: console plus custom frames exceed the 254-frame HUD wire limit (%u + %u)\n",
                first - 1, dz_frame_count);
        return;
    }
    frameDef_t *saved = gi.MemAlloc(sizeof(*saved) * dz_frame_count);
    frameDef_t const *visible[DZ_MAX_FRAMES];
    bool drawn[DZ_MAX_FRAMES] = { false };
    if (!saved) return;
    FOR_LOOP(i, dz_frame_count) {
        saved[i] = *dz_frames[i].frame;
        *dz_frames[i].frame = DzFrameValue(dz_frames + i, player);
    }
    uint32_t count = dz_frames[0].frame->hidden ? 0 : UI_CollectFrameTree(dz_frames[0].frame, visible, DZ_MAX_FRAMES);
    FOR_LOOP(i, dz_frame_count) FOR_LOOP(v, count) if (visible[v] == dz_frames[i].frame) drawn[i] = true;
    UI_SetCurrentClient(ent->client);
    FOR_LOOP(i, dz_frame_count) {
        frameDef_t const *frame = dz_frames[i].frame;
        uiFrame_t wire;
        uint8_t data[4096]; char text[4096];
        if (!UI_BuildFrameForWrite(frame, &wire, data, sizeof(data), text, sizeof(text))) continue;
        wire.number = first + i;
        wire.parent = 0;
        FOR_LOOP(r, dz_frame_count) if (frame->Parent == dz_frames[r].frame) wire.parent = first + r;
        FOR_LOOP(p, FPP_COUNT) {
            wire.points.x[p].relativeTo = frame->Points.x[p].relativeTo ? UI_PARENT : 0;
            wire.points.y[p].relativeTo = frame->Points.y[p].relativeTo ? UI_PARENT : 0;
            FOR_LOOP(r, dz_frame_count) {
                if (frame->Points.x[p].relativeTo == dz_frames[r].frame) wire.points.x[p].relativeTo = first + r;
                if (frame->Points.y[p].relativeTo == dz_frames[r].frame) wire.points.y[p].relativeTo = first + r;
            }
        }
        if (!drawn[i]) {
            wire.flags.type = FT_FRAME; wire.flagsvalue |= UIFRAME_DISABLED;
            wire.text = wire.onclick = wire.tooltip = NULL;
            wire.tex.index = wire.tex.index2 = 0; wire.buffer.size = 0;
        }
        gi.Write(PF_UIFRAME, &wire);
    }
    ui_next_frame_number = first + dz_frame_count;
    UI_SetCurrentClient(NULL);
    FOR_LOOP(i, dz_frame_count) *dz_frames[i].frame = saved[i];
    gi.MemFree(saved);
}

void G_DzFramesUpdate(void) {
    if (!dz_dirty_clients) return;
    FOR_LOOP(i, game.max_clients) {
        gameClient_t *client = game.clients + i;
        if (!client->connected || !(dz_dirty_clients & (1u << client->ps.number))) continue;
        edict_t *ent = G_GetPlayerEntityByNumber(client->ps.number);
        if (ent && ent->client == client) {
            client->resourcebar.gold = -1;
            G_RefreshResourceBar(ent);
        }
    }
    dz_dirty_clients = 0;
}

#endif
