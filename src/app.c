#include "app.h"
#include "types.c"
#include "draw.c"
#include "handle.c"

struct Launchpad lp;

void app_init(const u16 *adc_raw) {
    lp.tracks[0] = drum_track(245, 12, 34);
    lp.tracks[1] = drum_track(245, 12, 34);
    lp.tracks[2] = drum_track(245, 12, 34);
    lp.tracks[3] = drum_track(245, 12, 34);
    lp.tracks[4] = drum_track(245, 12, 34);
    lp.tracks[5] = melody_track(12, 44, 4);
    lp.tracks[6] = melody_track(12, 44, 4);
    lp.tracks[7] = melody_track(12, 44, 4);

    draw_all(&lp);
}

void app_surface_event(u8 type, u8 index, u8 value) {
    if (type == TYPEPAD && value > 0) {
        handle_track_mute(&lp, index);
        handle_active_track(&lp, index);
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

u16 ms = 0;
u16 clock = 0;

void app_timer_event() {
    if (ms++ % 20 == 0) {
        handle_clock(&lp, clock++);
    }
}
