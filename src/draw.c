#ifndef LP_DRAW
#define LP_DRAW

#include "types.c"
#include "app.h"

struct Color fade(struct Color color, u16 percent) {
    u16 r = color.r;
    u16 g = color.g;
    u16 b = color.b;

    r = (r * percent) / 100;
    g = (g * percent) / 100;
    b = (b * percent) / 100;

    struct Color new_color = {.r = r, .g = g, .b = b};
    return new_color;
}

void draw_pad(u8 pad_index, struct Color color) {
    hal_plot_led(TYPEPAD, pad_index, color.r, color.g, color.b);
}

void draw_mutes(struct Launchpad *lp) {
    for (u8 i = 0; i < 8; i++) {
        struct Track *track = &lp->tracks[i];
        if (!track->is_muted) {
            draw_pad(91 + i, track->color);
        } else {
            draw_pad(91 + i, NO_COLOR);
        }
    }
}

void draw_clock_divider(struct Launchpad *lp) {
    struct Track *track = &lp->tracks[lp->active_track];
    for (u8 i = 0; i < 8; i++) {
        u8 pad_index = (8 - i) * 10 + 9;
        if (track->clock_divider == i) {
            draw_pad(pad_index, COLOR_GREEN);
        } else {
            draw_pad(pad_index, NO_COLOR);
        }
    }
}

void draw_notepads(struct Launchpad *lp) {
    struct Track *track = &lp->tracks[lp->active_track];
    struct Step *step = &track->steps[track->current_step];
    if (lp->display_step_info != -1) {
        step = &track->steps[lp->display_step_info];
    }

    if (track->is_drums) {
        for (u8 i = 0; i < 16; i++) {
            u8 pad_index = (i / 4 + 1) * 10 + i % 4 + 1;

            if (step->velocity > 0 && step->note == DRUM_MIDI_NOTE[i]) {
                u16 velocity = step->velocity;
                draw_pad(pad_index, fade(COLOR_RED, (velocity * 100) / 127));
            } else if (lp->display_step_info == -1 && lp->last_note.velocity > 0 && lp->last_note.note == DRUM_MIDI_NOTE[i]) {
                u16 velocity = lp->last_note.velocity;
                draw_pad(pad_index, fade(COLOR_YELLOW, (velocity * 100) / 127));
            } else {
                draw_pad(pad_index, track->color);
            }
        }
    } else {
        for (u8 i = 0; i < 12; i++) {
            u8 pad_index = (i / 4 + 2) * 10 + i % 4 + 1;
            u8 note = track->octave * MELODY_MIDI_NOTE[i];

            if (step->velocity > 0 && step->note == note) {
                u16 velocity = step->velocity;
                draw_pad(pad_index, fade(COLOR_RED, (velocity * 100) / 127));
            } else if (lp->display_step_info == -1 && lp->last_note.velocity > 0 && lp->last_note.note == note) {
                u16 velocity = lp->last_note.velocity;
                draw_pad(pad_index, fade(COLOR_YELLOW, (velocity * 100) / 127));
            } else {
                if (MELODY_SEMITONE[i]) {
                    draw_pad(pad_index, fade(track->color, 30));
                } else {
                    draw_pad(pad_index, track->color);
                }
            }
        }

        draw_pad(11, NO_COLOR);
        draw_pad(12, NO_COLOR);

        struct Color octave_switch_color = {.r = 12, .g = 4, .b = 43};
        draw_pad(13, fade(octave_switch_color, 30));
        draw_pad(14, octave_switch_color);
    }
}

void draw_steps(struct Launchpad *lp) {
    struct Track *track = &lp->tracks[lp->active_track];
    for (u8 i = 0; i < 32; i++) {
        struct Step *step = &track->steps[i];
        u8 pad_index = (8 - (i / 8)) * 10 + (i % 8 + 1);

        if (lp->display_step_info >= 0 && lp->display_step_info_request_ms[i] > 0) {
            draw_pad(pad_index, COLOR_GREEN);
        } else if (track->current_step == i) {
            if (!track->is_muted) {
                draw_pad(pad_index, COLOR_RED);
            } else {
                draw_pad(pad_index, COLOR_YELLOW);
            }
        } else {
            if (step->velocity > 0) {
                u16 velocity = step->velocity;
                draw_pad(pad_index, fade(track->color, (velocity * 100) / 127));
            } else {
                draw_pad(pad_index, NO_COLOR);
            }
        }
    }
}

void draw_length_selector(struct Launchpad *lp) {
    struct Color color = {.r = 26, .g = 39, .b = 63};
    struct Track *track = &lp->tracks[lp->active_track];

    for (u8 i = 0; i < 4; i++) {
        if (i <= track->length) {
            draw_pad(45 + i, color);
        } else {
            draw_pad(45 + i, fade(color, 20));
        }
    }
}

void draw_velocity(struct Launchpad *lp) {
    struct Color color = {.r = 63, .g = 63, .b = 63};
    u8 velocity = lp->last_note.velocity;
    if (lp->display_step_info != -1) {
        velocity = lp->tracks[lp->active_track].steps[lp->display_step_info].velocity;
    }
    bool current_highlighted = velocity == 0;

    for (u8 i = 0; i < 12; i++) {
        u16 target_velocity = MAX(1, i * 10.58);
        u8 pad_index = (i / 4 + 1) * 10 + i % 4 + 5;

        if (!current_highlighted && (velocity <= target_velocity || i == 11)) {
            if (lp->display_step_info != -1) {
                draw_pad(pad_index, COLOR_RED);
            } else {
                draw_pad(pad_index, COLOR_YELLOW);
            }
            current_highlighted = true;
        } else {
            draw_pad(pad_index, fade(color, (target_velocity * 100) / 127));
        }
    }
}

void draw_track_selector(struct Launchpad *lp) {
    for (u8 i = 0; i < 8; i++) {
        struct Track *track = &lp->tracks[i];
        if (i == lp->active_track) {
            draw_pad(1 + i, track->color);
        } else {
            draw_pad(1 + i, NO_COLOR);
        }
    }
}

void draw_active_step(struct Launchpad *lp) {
    draw_steps(lp);
    draw_notepads(lp);
}

void draw_active_track(struct Launchpad *lp) {
    draw_track_selector(lp);
    draw_clock_divider(lp);
    draw_steps(lp);
    draw_length_selector(lp);
    draw_velocity(lp);
    draw_notepads(lp);
}

void draw_all(struct Launchpad *lp) {
    draw_mutes(lp);
    draw_active_track(lp);
}

#endif