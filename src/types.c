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

    u8 current_step;
    u8 octave;

    u32 current_step_clock;

    u8 active_pattern;
    u8 clock_divider[4];
    u8 length[4];

    struct Step steps[4][32];
    bool pattern_has_data[4];
};

struct PlayingPad {
    bool is_playing;
    u8 note;
    u8 track;
};

struct PlayingStep {
    bool is_playing;
    u8 note;
};

struct Launchpad {
    u32 time;
    u32 clock;
    u32 raw_clock;

    bool clone_pattern_mode;
    bool setup_mode;
    bool ableton_control_pad_mode;
    bool record_mode;
    bool session_mode;

    u8 active_track;
    s8 display_step_info;
    u32 display_step_info_request_ms[32];
    u32 activate_session_mode_request_ms;

    u8 prev_active_track;
    u8 active_track_buttons_pressed;
    u32 restore_active_track_request_ms;

    struct Step last_note;
    struct Track tracks[8];
    struct PlayingPad playing_pad[16];
    struct PlayingStep playing_step[8];
};

const struct Color COLOR_RED = {.r = 40, .g = 0, .b = 0};
const struct Color COLOR_YELLOW = {.r = 40, .g = 40, .b = 0};
const struct Color COLOR_CYAN = {.r = 0, .g = 40, .b = 40};
const struct Color COLOR_GREEN = {.r = 0, .g = 40, .b = 0};
const struct Color NO_COLOR = {.r = 0, .g = 0, .b = 0};

struct Track create_track(u8 r, u8 g, u8 b) {
    struct Track t = {
            .is_drums = false,
            .is_muted = false,
            .color = {.r = r, .g = g, .b = b},
            .current_step = 0,
            .octave = 3
    };

    for (u8 i = 0; i < 4; i++) {
        t.clock_divider[i] = 1;
        t.length[i] = 2;
    }

    for (u8 i = 0; i < 32; i++) {
        for (u8 j = 0; j < 4; j++) {
            t.steps[j][i].note = 0;
            t.steps[j][i].velocity = 0;
        }
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
        36, 37, 38, 39,
        40, 41, 42, 43,
        44, 45, 46, 47,
        48, 49, 50, 51
};

const u8 MELODY_MIDI_NOTE[12] = {
        24, 25, 26, 27,
        28, 29, 30, 31,
        32, 33, 34, 35
};

const bool MELODY_SEMITONE[12] = {
        false, true, false, true,
        false, false, true, false,
        true, false, true, false,
};

const u8 CLOCK_DIVIDER[8] = {
        1, 2, 4, 6, 8, 16, 32, 64
};

#endif