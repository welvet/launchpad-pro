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

void draw_steps(struct Launchpad *lp) {
    struct Track *track = &lp->tracks[lp->active_track];
    for (u8 i = 0; i < 32; i++) {
        struct Step *step = &track->steps[i];
        u8 pad_index = (8 - (i / 8)) * 10 + (i % 8 + 1);

        if (track->current_step == i) {
            if (!track->is_muted) {
                draw_pad(pad_index, COLOR_RED);
            } else {
                draw_pad(pad_index, COLOR_YELLOW);
            }
        } else {
            if (step->velocity > 0) {
                draw_pad(pad_index, fade(track->color, step->velocity));
            } else {
                draw_pad(pad_index, NO_COLOR);
            }
        }

    }
}

void draw_drumpads(struct Launchpad *lp) {
    struct Track *track = &lp->tracks[lp->active_track];
    for (u8 i = 0; i < 16; i++) {

    }
}

void draw_melodypads(struct Launchpad *lp) {

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

    draw_clock_divider(lp);
    draw_steps(lp);
    draw_length_selector(lp);
    draw_velocity(lp);

    if (lp->tracks[lp->active_track].is_drums) {
        draw_drumpads(lp);
    } else {
        draw_melodypads(lp);
    }
}

void draw_all(struct Launchpad *lp) {
    draw_mutes(lp);
    draw_active_track(lp);
}

#endif