/* Optional Warcraft III custom UI extension subset for offline maps.
 * DzLoadToc reads map-owned TOC files and passes FDF entries through
 * OpenRealm's existing FDF template loader. It does not create frames or
 * implement mouse events; DzFrame* bindings require a separate UI bridge. */
#ifndef WC3_API_ORDR_H
#define WC3_API_ORDR_H

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define ORDR_TOC_MAX_BYTES 65536u
#define ORDR_TOC_MAX_ENTRIES 128u
#define ORDR_TOC_MAX_PATH 512u

static bool ORDR_IsFdfFile(cstring_t filename) {
    size_t len;
    if (!filename) return false;
    len = strlen(filename);
    if (len < 5) return false;
    filename += len - 4;
    return filename[0] == '.' &&
           tolower((unsigned char)filename[1]) == 'f' &&
           tolower((unsigned char)filename[2]) == 'd' &&
           tolower((unsigned char)filename[3]) == 'f';
}

static void ORDR_ProcessTocLine(char *line) {
    char *start = line;
    size_t len;
    while (*start && isspace((unsigned char)*start)) start++;
    if (!*start || start[0] == '#' || (start[0] == '/' && start[1] == '/'))
        return;
    len = strlen(start);
    while (len && isspace((unsigned char)start[len-1]))
        start[--len] = '\0';
    if (len > 1 && start[0] == '"' && start[len-1] == '"') {
        start[len-1] = '\0';
        start++;
    }
    if (!ORDR_IsFdfFile(start))
        return;
    if (start[0] == '/' || start[0] == '\\' || strchr(start, ':') ||
        strstr(start, "..")) {
        fprintf(stderr, "ORDR DzLoadToc: rejected unsafe FDF path\n");
        return;
    }
    /* UI_EnsureFDF caches parsed files, so repeated DzLoadToc calls are safe. */
    UI_ParseFDF(start);
}

static uint32_t DzLoadToc(jass_t *j) {
    cstring_t filename = jass_checkstring(j, 1);
    uint32_t size = 0;
    char *buffer, *line;
    uint32_t count = 0;
    if (!filename || !*filename || filename[0] == '/' ||
        filename[0] == '\\' || strchr(filename, ':') || strstr(filename, "..")) {
        fprintf(stderr, "ORDR DzLoadToc: invalid TOC filename\n");
        return 0;
    }

    buffer = gi.ReadFile(filename, &size);
    if (!buffer) {
        fprintf(stderr, "ORDR DzLoadToc: TOC not found: %s\n", filename);
        return 0;
    }
    if (!size || size > ORDR_TOC_MAX_BYTES) {
        fprintf(stderr, "ORDR DzLoadToc: invalid TOC length for %s\n", filename);
        gi.MemFree(buffer);
        return 0;
    }

    char *text = gi.MemAlloc(size + 1);
    if (!text) {
        gi.MemFree(buffer);
        return 0;
    }
    memcpy(text, buffer, size);
    text[size] = '\0';
    gi.MemFree(buffer);

    line = text;
    if (size >= 3 && (unsigned char)line[0] == 0xEF &&
        (unsigned char)line[1] == 0xBB &&
        (unsigned char)line[2] == 0xBF)
        line += 3;
    while (*line && count < ORDR_TOC_MAX_ENTRIES) {
        char *next = strpbrk(line, "\r\n");
        if (next) {
            *next++ = '\0';
            if (*next == '\r' || *next == '\n') next++;
        }
        if (strlen(line) < ORDR_TOC_MAX_PATH)
            ORDR_ProcessTocLine(line);
        else
            fprintf(stderr, "ORDR DzLoadToc: FDF path too long\n");
        count++;
        if (!next) break;
        line = next;
    }
    gi.MemFree(text);
    return 0;
}


/*
 * DzSyncData: offline same-process transport.  A prefix registers one or
 * more triggers; sending that channel queues the trigger actions with the
 * sender player and an immutable per-coroutine payload.  UI callbacks are
 * delivered by the JASS scheduler; no network credentials are involved.
 *
 * Online/server synchronization is not emulated by this path.
 */
static uint32_t DzTriggerRegisterSyncData(jass_t *j) {
    trigger_t *trigger = jass_checkhandle(j, 1, "trigger");
    cstring_t prefix = jass_checkstring(j, 2);
    bool server = jass_checkboolean(j, 3);
    ordrSyncRegistration_t *entry;
    if (!trigger || !prefix || !*prefix || strlen(prefix) >= MAX_ORDR_SYNC_PREFIX) {
        jass_rterror(j, "DzTriggerRegisterSyncData: invalid trigger or prefix");
        return 0;
    }
    if (level.ordr_num_sync_registrations >= MAX_ORDR_SYNC_REGISTRATIONS) {
        jass_rterror(j, "DzTriggerRegisterSyncData: registration limit reached");
        return 0;
    }
    entry = &level.ordr_sync_registrations[level.ordr_num_sync_registrations++];
    entry->trigger = trigger;
    snprintf(entry->prefix, sizeof(entry->prefix), "%s", prefix);
    entry->server = server;
    return 0;
}

static uint32_t DzSyncData(jass_t *j) {
    cstring_t prefix = jass_checkstring(j, 1);
    cstring_t data = jass_checkstring(j, 2);
    jassContext_t const *context = jass_getcontext(j);
    player_t *sender = currentplayer ? currentplayer
        : context && context->localPlayerState ? context->localPlayerState
        : context && context->playerState ? context->playerState
        : G_GetPlayerByNumber(0);
    if (!prefix || !data || strlen(data) >= JASS_SYNC_PAYLOAD_MAX) {
        jass_rterror(j, "DzSyncData: invalid channel or oversized payload");
        return 0;
    }
    FOR_LOOP(i, level.ordr_num_sync_registrations) {
        ordrSyncRegistration_t *entry = &level.ordr_sync_registrations[i];
        if (strcmp(entry->prefix, prefix)) continue;
        /* Event sender and payload survive nested queued sync callbacks. */
        jass_calltriggersync(j, entry->trigger, sender, data);
    }
    return 0;
}

static uint32_t DzGetTriggerSyncData(jass_t *j) {
    jassContext_t const *context = jass_getcontext(j);
    return jass_pushstring(j, context ? context->syncData : "");
}

static uint32_t DzGetTriggerSyncPlayer(jass_t *j) {
    jassContext_t const *context = jass_getcontext(j);
    return context && context->playerState
        ? jass_pushlighthandle(j, context->playerState, "player")
        : jass_pushnullhandle(j, "player");
}

/* Reserved integer ID for the game UI root.  DzCreateFrame and the actual
 * frame render/input bridge are deliberately separate compatibility work. */
#define ORDR_GAME_UI_ROOT_ID 1
static uint32_t DzGetGameUI(jass_t *j) {
    return jass_pushinteger(j, ORDR_GAME_UI_ROOT_ID);
}

#endif /* WC3_API_ORDR_H */
