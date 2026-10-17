/* Offline-compatible JN string functions used by ORDR Season 2.
 * The real map script declares:
 * JNStringCount(string, string) -> integer
 * JNStringSplit(string, string, integer) -> string
 * JNStringReplace(string, string, string) -> string
 * JNStringRegex(string, string, integer) -> string
 *
 * JNStringCount is a REGEX count, not a literal substring count:
 * ORDR calls it with a Korean character class to validate user input.
 */
#ifndef WC3_API_ORDR_STRINGS_H
#define WC3_API_ORDR_STRINGS_H

#include <limits.h>
#include <regex.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ORDR_STRING_LIMIT (1024u * 1024u)

static unsigned ORDR_Utf8Codepoint(char const **cursor) {
    unsigned char const *s = (unsigned char const *)*cursor;
    unsigned cp;
    if (!*s) return 0;
    if (s[0] < 0x80) { (*cursor)++; return s[0]; }
    if ((s[0] & 0xe0) == 0xc0 && (s[1] & 0xc0) == 0x80) {
        cp = ((unsigned)(s[0] & 0x1f) << 6) | (s[1] & 0x3f);
        if (cp >= 0x80) { *cursor += 2; return cp; }
    }
    if ((s[0] & 0xf0) == 0xe0 && s[1] && s[2] &&
        (s[1] & 0xc0) == 0x80 && (s[2] & 0xc0) == 0x80) {
        cp = ((unsigned)(s[0] & 0x0f) << 12) |
             ((unsigned)(s[1] & 0x3f) << 6) | (s[2] & 0x3f);
        if (cp >= 0x800 && (cp < 0xd800 || cp > 0xdfff)) {
            *cursor += 3; return cp;
        }
    }
    if ((s[0] & 0xf8) == 0xf0 && s[1] && s[2] && s[3] &&
        (s[1] & 0xc0) == 0x80 && (s[2] & 0xc0) == 0x80 &&
        (s[3] & 0xc0) == 0x80) {
        cp = ((unsigned)(s[0] & 7) << 18) |
             ((unsigned)(s[1] & 63) << 12) |
             ((unsigned)(s[2] & 63) << 6) | (s[3] & 63);
        if (cp >= 0x10000 && cp <= 0x10ffff) {
            *cursor += 4; return cp;
        }
    }
    (*cursor)++; return 0xfffd;
}

static int32_t ORDR_RegexCount(cstring_t source, cstring_t pattern) {
    regex_t regex;
    regmatch_t match;
    int32_t count = 0;
    char const *cursor;
    if (!source || !pattern || !*pattern) return 0;
    /* POSIX 'C' locale does not support the Unicode range in ORDR login UI.
     * Recognize precisely the map's class regardless of device locale. */
    if (!strcmp(pattern, "[ㄱ-ㅣ가-힣]")) {
        cursor = source;
        while (*cursor) {
            unsigned cp = ORDR_Utf8Codepoint(&cursor);
            if ((cp >= 0x3131 && cp <= 0x3163) ||
                (cp >= 0xac00 && cp <= 0xd7a3)) ++count;
        }
        return count;
    }
    if (strlen(pattern) > 2048 || regcomp(&regex, pattern, REG_EXTENDED)) return 0;
    cursor = source;
    bool at_start = true;
    while (regexec(&regex, cursor, 1, &match, at_start ? 0 : REG_NOTBOL) == 0) {
        if (count == INT32_MAX || match.rm_so < 0) break;
        ++count;
        /* REG_NOTBOL keeps ^ anchored to the original input, not each suffix. */
        at_start = false;
        if (match.rm_eo > 0) {
            cursor += match.rm_eo;
            if (match.rm_so == match.rm_eo && !*cursor) break;
        } else if (*cursor) {
            (void)ORDR_Utf8Codepoint(&cursor); /* zero-width match */
        } else {
            break; /* zero-width match on final terminator */
        }
    }
    regfree(&regex);
    return count;
}

static uint32_t JNStringCount(jass_t *j) {
    cstring_t source = jass_checkstring(j, 1);
    cstring_t regex = jass_checkstring(j, 2);
    return jass_pushinteger(j, ORDR_RegexCount(source, regex));
}

static uint32_t JNStringSplit(jass_t *j) {
    cstring_t source = jass_checkstring(j, 1);
    cstring_t separator = jass_checkstring(j, 2);
    int32_t index = jass_checkinteger(j, 3);
    char const *start, *end;
    size_t separator_length;
    if (!source || !separator || !*separator || index < 0)
        return jass_pushstring(j, "");
    start = source;
    separator_length = strlen(separator);
    while (index--) {
        end = strstr(start, separator);
        if (!end) return jass_pushstring(j, "");
        start = end + separator_length;
    }
    end = strstr(start, separator);
    return jass_pushstringlen(j, start,
                              (uint32_t)(end ? (size_t)(end - start) : strlen(start)));
}

static uint32_t JNStringReplace(jass_t *j) {
    cstring_t source = jass_checkstring(j, 1);
    cstring_t before = jass_checkstring(j, 2);
    cstring_t after = jass_checkstring(j, 3);
    size_t source_length, before_length, after_length, count = 0, result_length;
    char const *cursor, *hit;
    char *buffer, *out;
    uint32_t result;
    if (!source) source = "";
    if (!before || !*before) return jass_pushstring(j, source);
    if (!after) after = "";
    source_length = strlen(source);
    before_length = strlen(before);
    after_length = strlen(after);
    if (source_length > ORDR_STRING_LIMIT) return jass_pushstring(j, source);
    cursor = source;
    while ((hit = strstr(cursor, before)) != NULL) {
        ++count;
        cursor = hit + before_length;
    }
    if (!count) return jass_pushstring(j, source);
    /* Calculate bounded output size without overflow, even for long replacements. */
    if (after_length > before_length &&
        count > (ORDR_STRING_LIMIT - source_length) / (after_length - before_length))
        return jass_pushstring(j, source);
    if (after_length >= before_length)
        result_length = source_length + count * (after_length - before_length);
    else
        result_length = source_length - count * (before_length - after_length);
    if (result_length > ORDR_STRING_LIMIT) return jass_pushstring(j, source);
    buffer = gi.MemAlloc((long)result_length + 1);
    if (!buffer) return jass_pushstring(j, source);
    out = buffer;
    cursor = source;
    while ((hit = strstr(cursor, before)) != NULL) {
        size_t prefix = (size_t)(hit - cursor);
        memcpy(out, cursor, prefix);
        out += prefix;
        memcpy(out, after, after_length);
        out += after_length;
        cursor = hit + before_length;
    }
    strcpy(out, cursor);
    result = jass_pushstring(j, buffer);
    gi.MemFree(buffer);
    return result;
}

static uint32_t JNStringRegex(jass_t *j) {
    cstring_t source = jass_checkstring(j, 1);
    cstring_t pattern = jass_checkstring(j, 2);
    int32_t index = jass_checkinteger(j, 3);
    regex_t regex;
    regmatch_t match[16];
    uint32_t result;
    if (!source || !pattern || index < 0 || index >= 16)
        return jass_pushstring(j, "");
    if (strlen(pattern) > 2048 || regcomp(&regex, pattern, REG_EXTENDED))
        return jass_pushstring(j, "");
    if (regexec(&regex, source, (size_t)index + 1, match, 0) != 0 ||
        match[index].rm_so < 0 || match[index].rm_eo < match[index].rm_so) {
        regfree(&regex);
        return jass_pushstring(j, "");
    }
    result = jass_pushstringlen(j, source + match[index].rm_so,
                  (uint32_t)(match[index].rm_eo - match[index].rm_so));
    regfree(&regex);
    return result;
}

#endif /* WC3_API_ORDR_STRINGS_H */
