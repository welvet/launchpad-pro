#include "app.h"
#include "types.c"
#include "draw.c"
#include "handle.c"
#include <stdbool.h>
#include <stdlib.h>

struct Launchpad lp;

void app_init(const u16 *adc_raw) {
    lp.active_track = 0;
    lp.last_note.note = 0;
    lp.last_note.velocity = 0;
    lp.display_step_info = -1;
    for (u8 i = 0; i < 32; i++) {
        lp.display_step_info_request_ms[i] = 0;
    }

    lp.tracks[0] = drum_track(63, 41, 30);
    lp.tracks[1] = drum_track(6, 12, 38);
    lp.tracks[2] = drum_track(31, 31, 31);
    lp.tracks[3] = drum_track(47, 28, 38);
    lp.tracks[4] = drum_track(62, 27, 31);
    lp.tracks[5] = melody_track(33, 38, 7);
    lp.tracks[6] = melody_track(61, 62, 30);
    lp.tracks[7] = melody_track(49, 13, 27);

    for (u8 i = 0; i < 8; i++) {
        clear_track(&lp, i);
    }

    draw_all(&lp);
}

void app_surface_event(u8 type, u8 index, u8 value) {
    if (!lp.has_srand) {
        srand(lp.time);
        lp.has_srand = true;
    }

    if (type == TYPEPAD && value > 0) {
        handle_track_mute(&lp, index);
        handle_active_track(&lp, index);
        handle_clock_divider(&lp, index);
        handle_step_components(&lp, index);
        handle_length(&lp, index);
        handle_note(&lp, index, value);
        handle_velocity(&lp, index);
        handle_sequencer(&lp, index);
        handle_session(&lp, index);
        handle_control(&lp, index);
    } else if (type == TYPEPAD) {
        handle_active_track_unpress(&lp, index);
        handle_note_unpress(&lp, index);
        handle_sequencer_unpress(&lp, index);
        handle_control_unpress(&lp, index);
    }

    if (type == TYPESETUP) {
        handle_setup(&lp, value > 0);
    }
}

void app_midi_event(u8 port, u8 status, u8 d1, u8 d2) {
    if (port == USBSTANDALONE) {
        switch (status) {
            case MIDISTART:
                handle_start(&lp);
                break;

            case MIDITIMINGCLOCK:
                if (lp.raw_clock++ % 3 == 0) {
                    lp.clock++;
                    handle_clock(&lp);
                }
                break;

            case MIDISTOP:
                handle_stop(&lp);
                break;


        }
    }

    if (port == USBMIDI) {
        switch (status) {
            case NOTEON:
                transfer_midi_note(&lp, NOTEON, d1, d2);
                break;

            case NOTEOFF:
                transfer_midi_note(&lp, NOTEOFF, d1, d2);
                break;
        }
    }
}

void app_sysex_event(u8 port, u8 *data, u16 count) {

}

void app_aftertouch_event(u8 index, u8 value) {
}

void app_cable_event(u8 type, u8 value) {

}

void app_timer_event() {
    lp.time++;
    handle_time_tick(&lp);
}
