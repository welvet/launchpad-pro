#ifndef LP_HANDLE
#define LP_HANDLE

#include "draw.c"
#include <stdbool.h>

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

#endif