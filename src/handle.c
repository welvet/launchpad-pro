#ifndef LP_HANDLE
#define LP_HANDLE

#include "draw.c"
#include <stdbool.h>

int ipow(int base, int exp) {
    int result = 1;
    for (;;) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

bool is_in_row(u8 pad, u8 row) {
    return pad / 10 == row;
}

void handle_track_mute(struct Launchpad *lp, u8 pad_index) {
    if (is_in_row(pad_index, 9)) {
        u8 track_id = pad_index % 10 - 1;
        lp->tracks[track_id].is_muted = !lp->tracks[track_id].is_muted;
        draw_mutes(lp);
    }
}

void handle_active_track(struct Launchpad *lp, u8 pad_index) {
    if (is_in_row(pad_index, 0)) {
        lp->active_track = pad_index - 1;
        draw_active_track(lp);
    }
}

void handle_clock(struct Launchpad *lp, u16 clock) {
    for (u8 i = 0; i < 8; i++) {
        struct Track *track = &lp->tracks[i];
        if (clock % (1 << track->clock_divider) == 0) {
            if (++track->current_step >= (ipow(4, track->length))) {
                track->current_step = 0;
            }

            if (lp->active_track == i) {
                draw_steps(lp);
            }
        }
    }
}

#endif