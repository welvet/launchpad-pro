#ifndef LP_DRAW
#define LP_DRAW

#include "types.c"
#include "app.h"

struct Color fade(struct Color color, u8 percent) {
//    u16
    struct Color new_color = {};
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

void draw_steps(struct Launchpad *lp) {
    struct Track *track = &lp->tracks[lp->active_track];
    for (u8 i = 0; i < 32; i++) {
        struct Step *step = &track->steps[i];
        u8 pad_index = (8 - (i / 8)) * 10 + (i % 8 + 1);

        if (track->current_step == i) {
            draw_pad(pad_index, COLOR_RED);
        } else {
            if (step->velocity > 0) {
                draw_pad(pad_index, fade(track->color, step->velocity));
            } else {
                draw_pad(pad_index, NO_COLOR);
            }
        }

    }
}

void draw_active_track(struct Launchpad *lp) {
    for (u8 i = 0; i < 8; i++) {
        struct Track *track = &lp->tracks[i];
        if (i == lp->active_track) {
            draw_pad(1 + i, track->color);
        } else {
            draw_pad(1 + i, NO_COLOR);
        }
    }

    draw_steps(lp);
}

void draw_all(struct Launchpad *lp) {
    draw_mutes(lp);
    draw_active_track(lp);
}

#endif