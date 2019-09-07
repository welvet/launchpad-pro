#ifndef LP_DRAW
#define LP_DRAW

#include "types.c"
#include "app.h"

void draw_mutes(struct Launchpad *lp) {
    for (int i = 0; i < 8; i++) {
        struct Track* track = &lp->tracks[i];
        if (!track->is_muted) {
            hal_plot_led(TYPEPAD, 91 + i, track->color.r, track->color.g, track->color.b);
        } else {
            hal_plot_led(TYPEPAD, 91 + i, 0, 0, 0);
        }
    }
}

void draw_all(struct Launchpad *lp) {
    draw_mutes(lp);
}

#endif