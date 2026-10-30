#ifndef cl_key_h
#define cl_key_h

#include "common/game_keys.h"

#define MAX_KEYS 256
#define KEY_MOD_SHIFT 1u // bit; left or right Shift; used as bind modifier
#define KEY_MOD_CTRL  2u // bit; left or right Ctrl; used as bind modifier
#define KEY_MOD_ALT   4u // bit; left or right Alt; used as bind modifier
#define KEY_MOD_MASK  7u // mask; SHIFT|CTRL|ALT; used to index the bind table
#define KEY_MOD_COUNT 8u // slots; 2^3 modifier combinations; used as bind-table depth

typedef unsigned char keyCode_t;

void Key_Init(void);
void Key_SetBinding(keyCode_t key, uint32_t mods, cstring_t binding);
cstring_t Key_GetBinding(keyCode_t key, uint32_t mods);
void Key_Event(keyCode_t key, uint32_t mods, bool down, uint32_t time);
void Key_WriteBindings(FILE *file);
#ifdef BZ_TESTS
cstring_t Key_FindBindingForTest(keyCode_t key, uint32_t mods);
#endif

#endif
