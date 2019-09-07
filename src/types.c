#ifndef LP_TYPES
#define LP_TYPES

#include <stdbool.h>

#define MAX(a, b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)

struct Color {
    u8 r;
    u8 g;
    u8 b;
};

struct Step {
    u8 note;
    u8 velocity;
};

struct Track {
    bool is_drums;
    bool is_muted;

    struct Color color;

    u8 clock_divider;
    u8 current_step;
    u8 length;
    struct Step steps[32];
};

struct Launchpad {
    u32 time;

    u8 active_track;
    s8 display_step_info;
    u32 display_step_info_request_ms[32];

    struct Step last_note;
    struct Track tracks[8];
};

const struct Color COLOR_RED = {.r = 40, .g = 0, .b = 0};
const struct Color COLOR_YELLOW = {.r = 40, .g = 40, .b = 0};
const struct Color COLOR_GREEN = {.r = 0, .g = 40, .b = 0};
const struct Color NO_COLOR = {.r = 0, .g = 0, .b = 0};

struct Track create_track(u8 r, u8 g, u8 b) {
    struct Track t = {
            .is_drums = false,
            .is_muted = false,
            .color = {.r = r, .g = g, .b = b},
            .clock_divider = 3,
            .current_step = 0,
            .length = 2
    };

    for (u8 i = 0; i < 32; i++) {
        t.steps[i].note = 0;
        t.steps[i].velocity = 0;
    }

    return t;
}

struct Track drum_track(u8 r, u8 g, u8 b) {
    struct Track t = create_track(r, g, b);
    t.is_drums = true;
    return t;
}

struct Track melody_track(u8 r, u8 g, u8 b) {
    struct Track t = create_track(r, g, b);
    return t;
}

const u8 DRUM_MIDI_NOTE[16] = {
        48, 49, 50, 51,
        44, 45, 46, 47,
        40, 41, 42, 43,
        36, 37, 38, 39
};

#endif