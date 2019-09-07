#include "app.h"
#include "types.c"
#include "draw.c"
#include "handle.c"
#include <stdbool.h>

struct Launchpad lp;

void app_init(const u16 *adc_raw) {
    lp.active_track = 0;
    lp.last_note.note = 0;
    lp.last_note.velocity = 0;
    lp.display_step_info = -1;
    for (u8 i = 0; i < 32; i++) {
        lp.display_step_info_request_ms[i] = 0;
    }

    lp.tracks[0] = drum_track(24, 12, 34);
    lp.tracks[1] = drum_track(24, 12, 34);
    lp.tracks[2] = drum_track(24, 12, 34);
    lp.tracks[3] = drum_track(24, 12, 34);
    lp.tracks[4] = drum_track(24, 12, 34);
    lp.tracks[5] = melody_track(12, 44, 4);
    lp.tracks[6] = melody_track(12, 44, 4);
    lp.tracks[7] = melody_track(12, 44, 4);

    draw_all(&lp);
}

void app_surface_event(u8 type, u8 index, u8 value) {
    if (type == TYPEPAD && value > 0) {
        handle_track_mute(&lp, index);
        handle_active_track(&lp, index);
        handle_clock_divider(&lp, index);
        handle_length(&lp, index);
        handle_note(&lp, index, value);
        handle_velocity(&lp, index);
        handle_sequencer(&lp, index);
    } else if (type == TYPEPAD) {
        handle_note_unpress(&lp, index);
        handle_sequencer_unpress(&lp, index);
    }
}

void app_midi_event(u8 port, u8 status, u8 d1, u8 d2) {

}

void app_sysex_event(u8 port, u8 *data, u16 count) {

}

void app_aftertouch_event(u8 index, u8 value) {
}

void app_cable_event(u8 type, u8 value) {

}

u16 clock = 0;

void app_timer_event() {
    lp.time++;
    if (lp.time % 20 == 0) {
        handle_clock(&lp, clock++);
    }
    handle_time_tick(&lp);
}
