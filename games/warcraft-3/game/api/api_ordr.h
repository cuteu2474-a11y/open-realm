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

#endif /* WC3_API_ORDR_H */
