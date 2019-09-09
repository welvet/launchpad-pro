#ifndef LP_HANDLE
#define LP_HANDLE

#include "draw.c"
#include "app_defs.h"
#include <stdbool.h>

bool is_in_row(u8 pad, u8 row) {
    return pad / 10 == row;
}

bool is_in_col(u8 pad, u8 col) {
    return pad % 10 == col;
}

bool is_notepad(u8 pad) {
    u8 pad_row = pad / 10;
    u8 pad_col = pad % 10;
    return pad_row > 0 && pad_row <= 4 && pad_col > 0 && pad_col <= 4;
}

bool is_velocity_selector(u8 pad) {
    u8 pad_row = pad / 10;
    u8 pad_col = pad % 10;
    return pad_row > 0 && pad_row < 4 && pad_col >= 5 && pad_col <= 8;
}

bool is_sequencer(u8 pad) {
    u8 pad_row = pad / 10;
    u8 pad_col = pad % 10;
    return pad_row > 4 && pad_row <= 8 && pad_col >= 1 && pad_col <= 8;
}

void play_note_pad(struct Launchpad *lp, u8 pad_index, u8 note, u8 velocity) {
    if (!lp->preview_off_mode) {
        hal_send_midi(USBSTANDALONE, NOTEON | lp->active_track, note, velocity);

        u8 pad_aindex = (pad_index / 10 - 1) * 4 + (pad_index % 10 - 1);
        struct PlayingPad *pad = &lp->playing_pad[pad_aindex];
        pad->is_playing = true;
        pad->note = note;
        pad->track = lp->active_track;
    }
}

void stop_note_pad(struct Launchpad *lp, u8 pad_aindex) {
    struct PlayingPad *pad = &lp->playing_pad[pad_aindex];
    if (pad->is_playing) {
        hal_send_midi(USBSTANDALONE, NOTEOFF | pad->track, pad->note, 0);

        pad->is_playing = false;
    }
}

void play_note_step(struct Launchpad *lp, u8 track, u8 note, u8 velocity) {
    if (!lp->tracks[track].is_muted) {
        hal_send_midi(USBSTANDALONE, NOTEON | track, note, velocity);

        lp->playing_step[track].is_playing = true;
        lp->playing_step[track].note = note;
    }
}

void stop_note_step(struct Launchpad *lp, u8 track) {
    struct PlayingStep *step = &lp->playing_step[track];
    if (step->is_playing) {
        hal_send_midi(USBSTANDALONE, NOTEOFF | track, step->note, 0);

        step->is_playing = false;
    }
}

void handle_track_mute(struct Launchpad *lp, u8 pad_index) {
    if (is_in_row(pad_index, 9)) {
        u8 track_id = pad_index % 10 - 1;
        lp->tracks[track_id].is_muted = !lp->tracks[track_id].is_muted;
        draw_mutes(lp);
        draw_active_track(lp);
    }
}

void switch_active_track(struct Launchpad *lp, u8 track_id) {
    lp->active_track = track_id;

    lp->last_note.note = 0;
    lp->last_note.velocity = 0;

    lp->display_step_info = -1;
    for (u8 i = 0; i < 32; i++) {
        lp->display_step_info_request_ms[i] = 0;
    }

    draw_active_track(lp);
}

void handle_active_track(struct Launchpad *lp, u8 pad_index) {
    if (is_in_row(pad_index, 0)) {
        if (lp->active_track_buttons_pressed++ == 0) {
            lp->prev_active_track = lp->active_track;
            lp->restore_active_track_request_ms = lp->time;
        }

        switch_active_track(lp,  pad_index - 1);
    }
}

void handle_active_track_unpress(struct Launchpad *lp, u8 pad_index) {
    if (is_in_row(pad_index, 0)) {
        if (--lp->active_track_buttons_pressed == 0 && lp->restore_active_track_request_ms + 500 < lp->time) {
            switch_active_track(lp, lp->prev_active_track);
        }
    }
}

void handle_clock_divider(struct Launchpad *lp, u8 pad_index) {
    if (is_in_col(pad_index, 9)) {
        struct Track *track = &lp->tracks[lp->active_track];
        track->clock_divider[track->active_pattern] = 8 - pad_index / 10;
        draw_active_track(lp);
    }
}

void handle_length(struct Launchpad *lp, u8 pad_index) {
    if (pad_index >= 45 && pad_index < 49) {
        struct Track *track = &lp->tracks[lp->active_track];
        track->length[track->active_pattern] = (pad_index - 45);
        draw_active_track(lp);
    }
}

void handle_clock(struct Launchpad *lp) {
    for (u8 i = 0; i < 8; i++) {
        struct Track *track = &lp->tracks[i];
        u8 clock_divider = CLOCK_DIVIDER[track->clock_divider[track->active_pattern]];

        if (lp->clock % clock_divider == 0) {
            u8 track_len = 1 << (track->length[track->active_pattern] + 2);
            track->current_step = lp->clock / clock_divider % track_len;

            stop_note_step(lp, i);
            struct Step *step = &track->steps[track->active_pattern][track->current_step];
            if (step->velocity > 0) {
                play_note_step(lp, i, step->note, step->velocity);
            }

            track->current_step_clock = lp->clock;

            if (lp->active_track == i) {
                draw_active_step(lp);
            }
        }

        track->next_step_clock = track->current_step_clock + clock_divider;
    }
}

void handle_velocity(struct Launchpad *lp, u8 pad_index) {
    if (is_velocity_selector(pad_index)) {
        u8 velocity_index = (pad_index / 10 - 1) * 4 + (pad_index % 10 - 5);

        if (lp->display_step_info >= 0) {
            struct Track *track = &lp->tracks[lp->active_track];
            for (u8 i = 0; i < 32; i++) {
                if (lp->display_step_info_request_ms[i] > 0) {
                    track->steps[track->active_pattern][i].velocity = velocity_index * 10.58;
                }
            }
        } else if (lp->last_note.velocity > 0) {
            lp->last_note.velocity = MAX(1, velocity_index * 10.58);
        }

        draw_notepads(lp);
        draw_velocity(lp);
    }

}


void handle_note(struct Launchpad *lp, u8 pad_index, u8 value) {
    if (is_notepad(pad_index)) {
        struct Track *track = &lp->tracks[lp->active_track];

        u8 note = 0;
        if (lp->tracks[lp->active_track].is_drums) {
            note = DRUM_MIDI_NOTE[(pad_index / 10 - 1) * 4 + (pad_index % 10 - 1)];
        } else {
            if (pad_index / 10 == 1) {
                if (pad_index == 13 && track->octave > 0) {
                    track->octave--;
                } else if (pad_index == 14 && track->octave < 6) {
                    track->octave++;
                }
            } else {
                note = 12 * track->octave + MELODY_MIDI_NOTE[(pad_index / 10 - 2) * 4 + (pad_index % 10 - 1)];
            }
        }

        if (note > 0) {
            play_note_pad(lp, pad_index, note, value);
            lp->last_note.note = note;
            lp->last_note.velocity = value;

            if (lp->display_step_info >= 0) {
                for (u8 i = 0; i < 32; i++) {
                    if (lp->display_step_info_request_ms[i] > 0) {
                        track->steps[track->active_pattern][i].note = note;
                        track->steps[track->active_pattern][i].velocity = value;
                    }
                }
            } else if (lp->record_mode) {
                if (track->next_step_clock > track->current_step_clock) {
                    u32 cur_clock = lp->raw_clock - 3 * track->current_step_clock;
                    u32 midstep = 3 * (track->next_step_clock - track->current_step_clock) / 2;

                    if (cur_clock <= midstep) {
                        track->steps[track->active_pattern][track->current_step].note = note;
                        track->steps[track->active_pattern][track->current_step].velocity = value;
                    } else {
                        u8 track_len = 1 << (track->length[track->active_pattern] + 2);
                        u8 next_step = (track->current_step + 1) % track_len;
                        track->steps[track->active_pattern][next_step].note = note;
                        track->steps[track->active_pattern][next_step].velocity = value;
                    }
                }

            }
        }

        draw_notepads(lp);
        draw_velocity(lp);
    }
}

void check_step_info(struct Launchpad *lp) {
    u8 min_step = 0;
    u32 min_time = 0;

    for (u8 i = 0; i < 32; i++) {
        if (lp->display_step_info_request_ms[i] > 0) {
            if (min_time == 0 || lp->display_step_info_request_ms[i] < min_time) {
                min_step = i;
                min_time = lp->display_step_info_request_ms[i];
            }
        }
    }

    if (min_time > 0 && min_time + 500 < lp->time) {
        if (lp->display_step_info != min_step) {
            lp->display_step_info = min_step;

            draw_steps(lp);
            draw_notepads(lp);
            draw_velocity(lp);
        }
    } else {
        if (lp->display_step_info != -1) {
            lp->display_step_info = -1;

            draw_steps(lp);
            draw_notepads(lp);
            draw_velocity(lp);
        }
    }
}


void handle_time_tick(struct Launchpad *lp) {
    if (lp->time % 100 == 0) {
        check_step_info(lp);
    }
}

void handle_sequencer(struct Launchpad *lp, u8 pad_index) {
    if (is_sequencer(pad_index)) {
        u8 step_index = (8 - (pad_index / 10)) * 8 + (pad_index % 10 - 1);
        lp->display_step_info_request_ms[step_index] = lp->time;
    }
}

void handle_sequencer_unpress(struct Launchpad *lp, u8 pad_index) {
    if (is_sequencer(pad_index)) {
        u8 step_index = (8 - (pad_index / 10)) * 8 + (pad_index % 10 - 1);

        if (lp->display_step_info == -1) {
            struct Track *track = &lp->tracks[lp->active_track];

            if (track->steps[track->active_pattern][step_index].velocity == 0) {
                if (lp->last_note.velocity > 0) {
                    track->steps[track->active_pattern][step_index].note = lp->last_note.note;
                    track->steps[track->active_pattern][step_index].velocity = lp->last_note.velocity;
                }
            } else {
                track->steps[track->active_pattern][step_index].note = 0;
                track->steps[track->active_pattern][step_index].velocity = 0;
            }

            draw_steps(lp);
        }

        lp->display_step_info_request_ms[step_index] = 0;
        check_step_info(lp);
    }
}

void handle_note_unpress(struct Launchpad *lp, u8 pad_index) {
    if (is_notepad(pad_index)) {
        u8 pad_aindex = (pad_index / 10 - 1) * 4 + (pad_index % 10 - 1);
        stop_note_pad(lp, pad_aindex);
    }
}

void handle_start(struct Launchpad *lp) {
    for (u8 i = 0; i < 8; i++) {
        stop_note_step(lp, i);
        lp->tracks[i].current_step = -1;
    }

    lp->clock = -1;
    lp->raw_clock = -1;
}

void handle_stop(struct Launchpad *lp) {
    for (u8 i = 0; i < 16; i++) {
        stop_note_step(lp, i);
    }
}

void handle_control(struct Launchpad *lp, u8 pad_index) {
    if (is_in_col(pad_index, 0)) {
        u8 id = pad_index / 10;
        struct Track *track = &lp->tracks[lp->active_track];

        if (id >= 5 && id <= 8) {
            u8 target_pattern = 8 - id;

            if (lp->clone_pattern_mode) {
                track->clock_divider[target_pattern] = track->clock_divider[track->active_pattern];
                track->length[target_pattern] = track->length[track->active_pattern];
                for (u8 i = 0; i < 32; i++) {
                    track->steps[target_pattern][i].note = track->steps[track->active_pattern][i].note;
                    track->steps[target_pattern][i].velocity = track->steps[track->active_pattern][i].velocity;
                }
            }
            if (lp->clear_pattern_mode) {
                track->clock_divider[target_pattern] = 1;
                track->length[target_pattern] = 2;
                for (u8 i = 0; i < 32; i++) {
                    track->steps[target_pattern][i].note = 0;
                    track->steps[target_pattern][i].velocity = 0;
                }
            }

            track->active_pattern = target_pattern;
            draw_active_track(lp);
        } else if (id == 4) {
            lp->clone_pattern_mode = true;
        } else if (id == 3 && track->length[track->active_pattern] <= 2) {
            u8 curr_len = 1 << (track->length[track->active_pattern] + 2);
            for (u8 i = 0; i < curr_len; i++) {
                track->steps[track->active_pattern][curr_len + i].note = track->steps[track->active_pattern][i].note;
                track->steps[track->active_pattern][curr_len + i].velocity =
                        track->steps[track->active_pattern][i].velocity;
            }

            track->length[track->active_pattern]++;
            draw_active_track(lp);
        } else if (id == 2) {
            lp->preview_off_mode = true;
        } else if (id == 1) {
            lp->record_mode = track;
        }
    }
}

void handle_control_unpress(struct Launchpad *lp, u8 pad_index) {
    if (is_in_col(pad_index, 0)) {
        u8 id = pad_index / 10;
        if (id == 4) {
            lp->clone_pattern_mode = false;
        } else if (id == 2) {
            lp->preview_off_mode = false;
        } else if (id == 1) {
            lp->record_mode = false;
        }
    }
}

void handle_setup(struct Launchpad *lp, bool pressed) {
    lp->clear_pattern_mode = pressed;
}

#endif