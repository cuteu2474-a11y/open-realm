#ifndef BZ_GAME_KEYS_H
#define BZ_GAME_KEYS_H

/* Input codes used by the universal client's bindings and optional authored
 * game key commands. These are not Dz/Windows VK numbers; the owning game
 * translates them at its API boundary. Keep the existing numeric ABI. */
enum {
    K_TAB = 9,
    K_ENTER = 13,
    K_ESCAPE = 27,
    K_SPACE = 32,

    K_F1 = 128, K_F2 = 129, K_F3 = 130, K_F4 = 131,
    K_F5 = 132, K_F6 = 133, K_F7 = 134, K_F8 = 135,
    K_F9 = 136, K_F10 = 137, K_F11 = 138, K_F12 = 139,
    K_UPARROW = 140, K_DOWNARROW = 141, K_LEFTARROW = 142, K_RIGHTARROW = 143,
    K_PAGEUP = 144, K_PAGEDOWN = 145, K_HOME = 146, K_END = 147,
    K_SHIFT = 148, K_CTRL = 149, K_ALT = 150,

    K_MOUSE1 = 200,
    K_MOUSE2 = 201,
    K_MOUSE3 = 202,
    K_MWHEELUP = 203,
    K_MWHEELDOWN = 204,
};

#endif
