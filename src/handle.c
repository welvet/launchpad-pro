#ifndef LP_HANDLE
#define LP_HANDLE

#include "draw.c"
#include "app_defs.h"
#include <stdbool.h>
#include <stdlib.h>

int randint(int n) {
    if ((n - 1) == RAND_MAX) {
        return rand();
    } else {
        int end = RAND_MAX / n;
        end *= n;

        int r;
        while ((r = rand()) >= end);

        return r % n;
    }
}

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
    hal_send_midi(USBMIDI, NOTEON | lp->active_track, note, velocity);
    hal_send_midi(DINMIDI, NOTEON | lp->active_track, note, velocity);

    u8 pad_aindex = (pad_index / 10 - 1) * 4 + (pad_index % 10 - 1);
    struct PlayingPad *pad = &lp->playing_pad[pad_aindex];
    pad->is_playing = true;
    pad->note = note;
    pad->track = lp->active_track;
}

void stop_note_pad(struct Launchpad *lp, u8 pad_aindex) {
    struct PlayingPad *pad = &lp->playing_pad[pad_aindex];
    if (pad->is_playing) {
        hal_send_midi(USBMIDI, NOTEOFF | pad->track, pad->note, 0);
        hal_send_midi(DINMIDI, NOTEOFF | pad->track, pad->note, 0);

        pad->is_playing = false;
    }
}

void play_note_step(struct Launchpad *lp, u8 track, u8 note, u8 velocity) {
    if (!lp->tracks[track].is_muted) {
        hal_send_midi(USBMIDI, NOTEON | track, note, velocity);
        hal_send_midi(DINMIDI, NOTEON | track, note, velocity);

        lp->playing_step[track].is_playing = true;
        lp->playing_step[track].note = note;
    }
}

void stop_note_step(struct Launchpad *lp, u8 track) {
    struct PlayingStep *step = &lp->playing_step[track];
    if (step->is_playing) {
        hal_send_midi(USBMIDI, NOTEOFF | track, step->note, 0);
        hal_send_midi(DINMIDI, NOTEOFF | track, step->note, 0);

        step->is_playing = false;
    }
}

void transfer_midi_note(struct Launchpad *lp, u8 command, u8 note, u8 value) {
    hal_send_midi(USBMIDI, command | lp->active_track, note, value);
    hal_send_midi(DINMIDI, command | lp->active_track, note, value);
}

void mark_current_pattern_active(struct Launchpad *lp) {
    struct Track *track = &lp->tracks[lp->active_track];
    u8 track_len = 1 << (track->length[track->active_pattern] + 2);

    bool has_data = false;
    for (u8 i = 0; i < track_len; i++) {
        struct Step *step = &track->steps[track->active_pattern][i];
        if (step->note > 0 && step->velocity > 0) {
            has_data = true;
            break;
        }
    }

    track->pattern_has_data[track->active_pattern] = has_data;
}

void send_ableton_control_pad(struct Launchpad *lp) {
    if (lp->ableton_control_pad_mode) {
        struct Track *track = &lp->tracks[lp->active_track];
        if (track->is_drums && lp->last_note.velocity > 0) {
            hal_send_midi(USBSTANDALONE, NOTEON | 15, lp->active_track, lp->last_note.note);
            return;
        }
    }

    hal_send_midi(USBSTANDALONE, NOTEOFF | 15, 0, 0);
}

void switch_ableton_control_pad(struct Launchpad *lp, bool force_off) {
    bool prev_state = lp->ableton_control_pad_mode;
    if (force_off) {
        lp->ableton_control_pad_mode = false;
    } else {
        lp->ableton_control_pad_mode = !lp->ableton_control_pad_mode;
    }

    if (lp->ableton_control_pad_mode != prev_state) {
        send_ableton_control_pad(lp);
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

    switch_ableton_control_pad(lp, true);

    draw_active_track(lp);
}

void handle_active_track(struct Launchpad *lp, u8 pad_index) {
    if (is_in_row(pad_index, 0)) {
        if (lp->active_track_buttons_pressed++ == 0) {
            lp->prev_active_track = lp->active_track;
            lp->restore_active_track_request_ms = lp->time;
        }

        switch_active_track(lp, pad_index - 1);
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
        if (lp->display_step_info < 0) {
            struct Track *track = &lp->tracks[lp->active_track];
            track->clock_divider[track->active_pattern] = 8 - pad_index / 10;
            draw_active_track(lp);
        }
    }
}

void handle_step_components(struct Launchpad *lp, u8 pad_index) {
    if (is_in_col(pad_index, 9)) {
        if (lp->display_step_info >= 0) {
            u8 id = pad_index / 10;
            struct Track *track = &lp->tracks[lp->active_track];

            for (u8 i = 0; i < 32; i++) {
                if (lp->display_step_info_request_ms[i] > 0) {
                    struct StepComponent *component = &track->step_components[track->active_pattern][i];

                    if (id == 8) {
                        component->random_trigger = !component->random_trigger;
                    } else if (id == 7) {
                        component->random_note = !component->random_note;
                    } else if (id == 6) {
                        component->random_velocity = !component->random_velocity;
                    } else if (id == 5) {
                        component->random_jump_step = !component->random_jump_step;
                    }
                }
            }

            draw_step_components(lp);
        }
    }
}

void handle_length(struct Launchpad *lp, u8 pad_index) {
    if (pad_index >= 45 && pad_index < 49) {
        struct Track *track = &lp->tracks[lp->active_track];
        track->length[track->active_pattern] = (pad_index - 45);
        mark_current_pattern_active(lp);

        draw_active_track(lp);
    }
}

u8 normalize(u8 val) {
    if (val < 0) {
        return 0;
    } else if (val >= 127) {
        return 127;
    }

    return val;
}

void handle_clock(struct Launchpad *lp) {
    for (u8 i = 0; i < 8; i++) {
        struct Track *track = &lp->tracks[i];
        u8 clock_divider = CLOCK_DIVIDER[track->clock_divider[track->active_pattern]];

        if (lp->clock % clock_divider == 0) {
            u8 track_len = 1 << (track->length[track->active_pattern] + 2);
            track->current_step = lp->clock / clock_divider % track_len;
            track->current_step = (track->current_step + track->step_offset) % track_len;

            stop_note_step(lp, i);
            struct Step *step = &track->steps[track->active_pattern][track->current_step];
            struct StepComponent *component = &track->step_components[track->active_pattern][track->current_step];

            u8 note = step->note;
            u8 velocity = step->velocity;
            if (step->velocity > 0) {
                if (component->random_note) {
                    note = note + (randint(7)) - 3;
                }
                if (component->random_velocity) {
                    velocity = velocity + (randint(60)) - 30;
                }
                if (component->random_trigger) {
                    if (rand() & 1) {
                        velocity = 0;
                    }
                }

                if (velocity > 0) {
                    play_note_step(lp, i, normalize(note), normalize(velocity));
                }
            }

            if (component->random_jump_step) {
                track->step_offset = randint(track->current_step);
            }

            track->current_step_clock = lp->clock;

            if (lp->active_track == i) {
                draw_active_step(lp);
            }
        }
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
            lp->last_note.note = note;
            lp->last_note.velocity = value;

            play_note_pad(lp, pad_index, note, value);
            send_ableton_control_pad(lp);

            if (lp->display_step_info >= 0) {
                for (u8 i = 0; i < 32; i++) {
                    if (lp->display_step_info_request_ms[i] > 0) {
                        track->steps[track->active_pattern][i].note = note;
                        track->steps[track->active_pattern][i].velocity = value;
                    }
                }
            } else if (lp->record_mode) {
                track->steps[track->active_pattern][track->current_step].note = note;
                track->steps[track->active_pattern][track->current_step].velocity = value;
            }

            mark_current_pattern_active(lp);
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
            draw_clock_divider(lp);
            draw_step_components(lp);
        }
    } else {
        if (lp->display_step_info != -1) {
            lp->display_step_info = -1;

            draw_steps(lp);
            draw_notepads(lp);
            draw_velocity(lp);
            draw_clock_divider(lp);
            draw_step_components(lp);
        }
    }
}


void handle_time_tick(struct Launchpad *lp) {
    if (lp->time % 100 == 0) {
        check_step_info(lp);
    }
}

void handle_sequencer(struct Launchpad *lp, u8 pad_index) {
    if (!lp->session_mode && is_sequencer(pad_index)) {
        u8 step_index = (8 - (pad_index / 10)) * 8 + (pad_index % 10 - 1);
        lp->display_step_info_request_ms[step_index] = lp->time;
    }
}

void handle_sequencer_unpress(struct Launchpad *lp, u8 pad_index) {
    if (!lp->session_mode && is_sequencer(pad_index)) {
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
        mark_current_pattern_active(lp);
        check_step_info(lp);
    }
}

void handle_session(struct Launchpad *lp, u8 pad_index) {
    if (lp->session_mode && is_sequencer(pad_index)) {
        u8 track = pad_index % 10 - 1;
        u8 pattern = 8 - pad_index / 10;

        lp->tracks[track].active_pattern = pattern;
        lp->tracks[track].step_offset = 0;

        if (track == lp->active_track) {
            draw_control(lp);
        }

        draw_session(lp);
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

void clear_track(struct Launchpad *lp, u8 track) {
    for (u8 i = 0; i < 4; i++) {
        for (u8 j = 0; j < 32; j++) {
            struct Step *step = &lp->tracks[track].steps[i][j];

            step->note = 0;
            step->velocity = 0;

            struct StepComponent *component = &lp->tracks[track].step_components[i][j];
            component->random_trigger = false;
            component->random_note = false;
            component->random_velocity = false;
            component->random_jump_step = false;
        }

        lp->tracks[track].length[i] = 2;
        lp->tracks[track].clock_divider[i] = 1;
        lp->tracks[track].pattern_has_data[i] = 0;
        lp->tracks[track].step_offset = 0;
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
                track->pattern_has_data[target_pattern] = track->pattern_has_data[track->active_pattern];

                for (u8 i = 0; i < 32; i++) {
                    track->steps[target_pattern][i].note = track->steps[track->active_pattern][i].note;
                    track->steps[target_pattern][i].velocity = track->steps[track->active_pattern][i].velocity;

                    struct StepComponent *target_component = &track->step_components[target_pattern][i];
                    struct StepComponent *active_component = &track->step_components[track->active_pattern][i];
                    target_component->random_trigger = active_component->random_trigger;
                    target_component->random_note = active_component->random_note;
                    target_component->random_velocity = active_component->random_velocity;
                    target_component->random_jump_step = active_component->random_jump_step;
                }
            }
            if (lp->setup_mode) {
                track->clock_divider[target_pattern] = 1;
                track->length[target_pattern] = 2;
                track->pattern_has_data[target_pattern] = false;

                for (u8 i = 0; i < 32; i++) {
                    track->steps[target_pattern][i].note = 0;
                    track->steps[target_pattern][i].velocity = 0;

                    struct StepComponent *component = &track->step_components[target_pattern][i];
                    component->random_trigger = false;
                    component->random_note = false;
                    component->random_velocity = false;
                    component->random_jump_step = false;
                }
            }

            track->active_pattern = target_pattern;
            track->step_offset = 0;

            draw_active_track(lp);
        } else if (id == 4) {
            if (lp->setup_mode) {
                clear_track(lp, lp->active_track);
                draw_active_track(lp);
            } else {
                lp->clone_pattern_mode = true;
                draw_control(lp);
            }
        } else if (id == 3) {
            if (lp->setup_mode) {
                for (u8 i = 0; i < 8; i++) {
                    clear_track(lp, i);
                }
                draw_active_track(lp);
            } else {
                if (track->length[track->active_pattern] <= 2) {
                    u8 curr_len = 1 << (track->length[track->active_pattern] + 2);
                    for (u8 i = 0; i < curr_len; i++) {
                        track->steps[track->active_pattern][curr_len +i].note
                                = track->steps[track->active_pattern][i].note;
                        track->steps[track->active_pattern][curr_len + i].velocity =
                                track->steps[track->active_pattern][i].velocity;
                    }

                    track->length[track->active_pattern]++;
                    draw_active_track(lp);
                }
            }
        } else if (id == 2) {
            if (lp->setup_mode) {

            } else {
                switch_ableton_control_pad(lp, false);
                draw_control(lp);
            }
        } else if (id == 1) {
            if (lp->setup_mode) {

            } else {
                lp->record_mode = true;
            }
        }
    }
}

void handle_control_unpress(struct Launchpad *lp, u8 pad_index) {
    if (!lp->setup_mode && is_in_col(pad_index, 0)) {
        u8 id = pad_index / 10;
        if (id == 4) {
            lp->clone_pattern_mode = false;
            draw_control(lp);
        } else if (id == 1) {
            lp->record_mode = false;
        }
    }
}

void handle_setup(struct Launchpad *lp, bool pressed) {
    lp->setup_mode = pressed;
    draw_control(lp);

    if (pressed) {
        lp->activate_session_mode_request_ms = lp->time;
    } else {
        if (500 + lp->activate_session_mode_request_ms >= lp->time) {
            lp->session_mode = !lp->session_mode;
            draw_steps(lp);
            draw_session(lp);
        }
    }
}

#endif