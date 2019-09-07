#ifndef LP_TYPES
#define LP_TYPES

#include <stdbool.h>

struct Color {
    u8 r;
    u8 g;
    u8 b;
};

struct Track {
    bool is_drums;
    bool is_muted;
    struct Color color;
};

struct Launchpad {
    struct Track tracks[8];
};

struct Track drum_track(u8 r, u8 g, u8 b) {
    struct Track c = {
            .is_drums = true, .is_muted = false, .color = {.r = r, .g = g, .b = b}
    };
    return c;
}

struct Track melody_track(u8 r, u8 g, u8 b) {
    struct Track c = {
            .is_drums = false, .is_muted = false, .color = {.r = r, .g = g, .b = b}
    };
    return c;
}

#endif