/* Logical DzFrame compatibility for ORDR offline map scripts.
 * This layer owns stable map-local handles and attributes; it DOES NOT render
 * dynamic artwork or dispatch touch/click events to the client yet.
 * Avoid reporting UI as playable until the client presentation bridge exists.
 */
#ifndef WC3_API_ORDR_FRAMES_H
#define WC3_API_ORDR_FRAMES_H

static ordrFrame_t *ORDR_Frame(uint32_t handle) {
    uint32_t index;
    if (handle <= ORDR_GAME_UI_ROOT_ID) return NULL;
    index = handle - ORDR_GAME_UI_ROOT_ID - 1;
    return index < level.ordr_num_frames ? &level.ordr_frames[index] : NULL;
}

static uint32_t ORDR_CreateLogicalFrame(jass_t *j, cstring_t type, cstring_t name,
                                         uint32_t parent, cstring_t template_name,
                                         int32_t create_context) {
    ordrFrame_t *frame;
    uint32_t id;
    if (level.ordr_num_frames >= MAX_ORDR_FRAMES) {
        jass_rterror(j, "DzCreateFrame: logical frame registry full");
        return jass_pushinteger(j, 0);
    }
    id = ORDR_GAME_UI_ROOT_ID + 1 + level.ordr_num_frames;
    frame = &level.ordr_frames[level.ordr_num_frames++];
    memset(frame, 0, sizeof(*frame));
    frame->handle_id = id;
    frame->parent = parent;
    frame->create_context = create_context;
    frame->visible = true;
    frame->enabled = true;
    frame->alpha = 255;
    snprintf(frame->type, sizeof(frame->type), "%s", type ? type : "");
    snprintf(frame->name, sizeof(frame->name), "%s", name ? name : "");
    snprintf(frame->template_name, sizeof(frame->template_name), "%s",
             template_name ? template_name : "");
    return jass_pushinteger(j, (int32_t)id);
}

static uint32_t DzCreateFrame(jass_t *j) {
    cstring_t name = jass_checkstring(j, 1);
    uint32_t parent = (uint32_t)jass_checkinteger(j, 2);
    int32_t context = jass_checkinteger(j, 3);
    return ORDR_CreateLogicalFrame(j, "TEMPLATE", name, parent, name, context);
}

static uint32_t DzCreateFrameByTagName(jass_t *j) {
    cstring_t type = jass_checkstring(j, 1);
    cstring_t name = jass_checkstring(j, 2);
    uint32_t parent = (uint32_t)jass_checkinteger(j, 3);
    cstring_t template_name = jass_checkstring(j, 4);
    int32_t context = jass_checkinteger(j, 5);
    return ORDR_CreateLogicalFrame(j, type, name, parent, template_name, context);
}

static uint32_t DzFrameFindByName(jass_t *j) {
    cstring_t name = jass_checkstring(j, 1);
    int32_t context = jass_checkinteger(j, 2);
    if (name && *name) {
        for (uint32_t i = level.ordr_num_frames; i; --i) {
            ordrFrame_t *frame = &level.ordr_frames[i - 1];
            if (frame->create_context == context && !strcmp(frame->name, name))
                return jass_pushinteger(j, (int32_t)frame->handle_id);
        }
    }
    return jass_pushinteger(j, 0); /* FDF template instances not yet materialized */
}

static uint32_t DzFrameSetTexture(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    cstring_t path = jass_checkstring(j, 2);
    (void)jass_checkinteger(j, 3); /* draw flags need the client renderer */
    if (frame) snprintf(frame->texture, sizeof(frame->texture), "%s", path ? path : "");
    return 0;
}

static uint32_t DzFrameSetModel(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    cstring_t model = jass_checkstring(j, 2);
    (void)jass_checkinteger(j, 3);
    (void)jass_checkinteger(j, 4);
    if (frame) snprintf(frame->model, sizeof(frame->model), "%s", model ? model : "");
    return 0;
}

static uint32_t DzFrameSetSize(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    float width = jass_checknumber(j, 2), height = jass_checknumber(j, 3);
    if (frame) { frame->width = width; frame->height = height; }
    return 0;
}

static uint32_t DzFrameSetPoint(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    int32_t point = jass_checkinteger(j, 2);
    uint32_t relative = (uint32_t)jass_checkinteger(j, 3);
    int32_t relative_point = jass_checkinteger(j, 4);
    float x = jass_checknumber(j, 5), y = jass_checknumber(j, 6);
    if (frame) {
        frame->absolute = false;
        frame->anchor = point;
        frame->relative_frame = relative;
        frame->relative_point = relative_point;
        frame->x = x; frame->y = y;
    }
    return 0;
}

static uint32_t DzFrameSetAbsolutePoint(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    int32_t point = jass_checkinteger(j, 2);
    float x = jass_checknumber(j, 3), y = jass_checknumber(j, 4);
    if (frame) {
        frame->absolute = true;
        frame->anchor = point;
        frame->relative_frame = 0;
        frame->x = x; frame->y = y;
    }
    return 0;
}

static uint32_t DzFrameClearAllPoints(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    if (frame) {
        frame->relative_frame = 0;
        frame->relative_point = 0;
        frame->anchor = 0;
        frame->absolute = false;
        frame->x = frame->y = 0.0f;
    }
    return 0;
}

static uint32_t DzFrameShow(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    bool visible = jass_checkboolean(j, 2);
    if (frame) frame->visible = visible;
    return 0;
}

static uint32_t DzFrameSetEnable(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    bool enabled = jass_checkboolean(j, 2);
    if (frame) frame->enabled = enabled;
    return 0;
}

static uint32_t DzFrameSetText(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    cstring_t text = jass_checkstring(j, 2);
    if (frame) snprintf(frame->text, sizeof(frame->text), "%s", text ? text : "");
    return 0;
}

static uint32_t DzFrameGetText(jass_t *j) {
    ordrFrame_t const *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    return jass_pushstring(j, frame ? frame->text : "");
}

static uint32_t DzFrameSetFont(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    cstring_t font = jass_checkstring(j, 2);
    float height = jass_checknumber(j, 3);
    int32_t flags = jass_checkinteger(j, 4);
    if (frame) {
        snprintf(frame->font, sizeof(frame->font), "%s", font ? font : "");
        frame->font_height = height;
        frame->font_flags = flags;
    }
    return 0;
}

static uint32_t DzFrameSetAlpha(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    int32_t alpha = jass_checkinteger(j, 2);
    if (frame) frame->alpha = MAX(0, MIN(255, alpha));
    return 0;
}

static uint32_t DzFrameSetTextAlignment(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    int32_t alignment = jass_checkinteger(j, 2);
    if (frame) frame->text_alignment = alignment;
    return 0;
}

static uint32_t DzFrameSetPriority(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    int32_t priority = jass_checkinteger(j, 2);
    if (frame) frame->priority = priority;
    return 0;
}

static uint32_t DzFrameSetFocus(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    bool focus = jass_checkboolean(j, 2);
    if (frame) frame->focused = focus;
    return jass_pushboolean(j, frame != NULL);
}

static uint32_t DzFrameSetScriptByCode(jass_t *j) {
    ordrFrame_t *frame = ORDR_Frame((uint32_t)jass_checkinteger(j, 1));
    int32_t event = jass_checkinteger(j, 2);
    jassFunc_t const *func = jass_checkcode(j, 3);
    bool sync = jass_checkboolean(j, 4);
    if (!frame) return 0;
    for (uint32_t i = 0; i < frame->callback_count; ++i)
        if (frame->callbacks[i].event_id == event) {
            frame->callbacks[i] = (ordrFrameEvent_t){event, func, sync};
            return 0;
        }
    if (frame->callback_count >= MAX_ORDR_FRAME_SCRIPT_EVENTS) {
        jass_rterror(j, "DzFrameSetScriptByCode: too many event callbacks for a frame");
        return 0;
    }
    frame->callbacks[frame->callback_count++] = (ordrFrameEvent_t){event, func, sync};
    return 0;
}

/* These context getters are meaningful only after a UI event has been
 * delivered from a client; the presentation/input bridge is still pending. */
static uint32_t DzGetTriggerUIEventPlayer(jass_t *j) {
    return jass_pushnullhandle(j, "player");
}
static uint32_t DzGetTriggerUIEventFrame(jass_t *j) {
    return jass_pushinteger(j, 0);
}

#endif /* WC3_API_ORDR_FRAMES_H */
