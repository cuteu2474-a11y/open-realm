#ifndef WC3_ORDR_STRINGS_H
#define WC3_ORDR_STRINGS_H

/* ORDR/JN compatibility helpers. All returned strings are malloc-owned. */
char *ordr_string_split(char const *input, char const *separator, int index);
char *ordr_string_replace(char const *input, char const *needle, char const *replacement);

/* POSIX ERE implementation on platforms with <regex.h>.
 * This does not guarantee full compatibility with every JN regex dialect. */
#if !defined(_WIN32)
int ordr_string_regex_count(char const *input, char const *pattern);
char *ordr_string_regex_match(char const *input, char const *pattern, int index);
#endif

#endif
