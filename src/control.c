// -----------------------------------------------------------------------------
// acperience (c) scanner darkly 2021
// ----------------------------------------------------------------------------

#include "compiler.h"
#include "string.h"

#include "control.h"
#include "interface.h"
#include "engine.h"

preset_meta_t meta;
preset_data_t preset;
shared_data_t shared;
int selected_preset;

// ----------------------------------------------------------------------------
// definitions and variables

#define TIMER_RECORDING 0

#define TRANSPOSE_OUTPUT 36
#define NO_STEP 255

#define PAGE_TRACKER 0
#define TRACKER_DIR_V 0
#define TRACKER_DIR_H 1
#define TRACKER_LINES 8

#define LED_SEQ_ON 15
#define LED_SEQ_OFF 4
#define LED_MENU_ON 15
#define LED_MENU_OFF 4

#define LED_RECORDING_ON_1    13
#define LED_RECORDING_ON_2    15
#define LED_RECORDING_ARMED_1  6
#define LED_RECORDING_ARMED_2  8
#define LED_RECORDING_OFF      4

#define LED_TRIGGER_ON_1   12
#define LED_TRIGGER_ON_2   15
#define LED_TRIGGER_GATE_1  6
#define LED_TRIGGER_GATE_2  9
#define LED_TRIGGER_OFF_1   3
#define LED_TRIGGER_OFF_2   6

#define LED_KEYBOARD_REST     3
#define LED_KEYBOARD_OFF      3
#define LED_KEYBOARD_USED     6
#define LED_KEYBOARD_CURRENT  9
#define LED_KEYBOARD_STEP    12
#define LED_KEYBOARD_NOTE    12

#define RECORDING_OFF   0
#define RECORDING_ARMED 1
#define RECORDING_ON    2

// pattern
engine_pattern_t pattern;

// sequencer
u8 seq_on;

// ui
u8 page, tracker_dir, follow_tracker_page;
u8 tracker_page_count, tracker_selector_y1, tracker_selector_y2;
u8 tracker_page, tracker_start_step;

u8 keyboard_on, recording_mode, edited_step;
s8 keyboard_note, recording_led;

// settings TODO
/*
u8 setting_303_slide = 1;                       // [1]
u8 setting_hold_accent_on_ties_and_rests = 1;   // [2]
u8 setting_gate_stays_high_before_slide = 1;    // [3]
u8 setting_slide_is_high_on_ties_and_rests = 1; // [4]
*/

static void grid_press(u8 x, u8 y, u8 pressed);
static void grid_press_menu(u8 x, u8 y, u8 pressed);
static void grid_press_tracker(u8 x, u8 y, u8 pressed);
static void grid_press_tracker_menu(u8 x, u8 y, u8 pressed);
static void grid_press_tracker_tracker(u8 x, u8 y, u8 pressed);

static void render_menu(void);
static void render_tracker(void);
static void render_tracker_menu(void);
static void render_tracker_tracker(void);

static void step(void);
static void step_off(void);


// ----------------------------------------------------------------------------
// functions for multipass

void init_presets(void) {
    for (u8 i = 0; i < get_preset_count(); i++) {
        store_preset_to_flash(i, &meta, &preset);
    }

    store_shared_data_to_flash(&shared);
    store_preset_index(0);
}

void init_control(void) {
    load_shared_data_from_flash(&shared);
    selected_preset = get_preset_index();
    load_preset_from_flash(selected_preset, &preset);
    load_preset_meta_from_flash(selected_preset, &meta);

    e_init(&pattern);
    seq_on = 1;
    
    page = PAGE_TRACKER;
    tracker_dir = TRACKER_DIR_V;
    follow_tracker_page = 0;
    
    tracker_page_count = MAX_PATTERN_LENGTH / TRACKER_LINES;
    tracker_selector_y1 = (TRACKER_LINES - tracker_page_count) >> 1;
    tracker_selector_y2 = tracker_selector_y1 + tracker_page_count - 1;
    
    tracker_page = 0;
    tracker_start_step = 0;

    keyboard_on = 0;
    recording_mode = RECORDING_OFF;
    edited_step = NO_STEP;
    
    keyboard_note = -1;
    recording_led = 0;
    
    refresh_grid();
    add_timed_event(TIMER_RECORDING, 200, 1);
}

void process_event(u8 event, u8 *data, u8 length) {
    switch (event) {
        case MAIN_CLOCK_RECEIVED:
            if (data[1]) step(); else step_off();
            break;
        
        case MAIN_CLOCK_SWITCHED:
            break;
    
        case GATE_RECEIVED:
            break;
        
        case GRID_KEY_PRESSED:
            grid_press(data[0], data[1], data[2]);
            break;
    
        case GRID_KEY_HELD:
            break;
            
        case FRONT_BUTTON_PRESSED:
            break;
    
        case FRONT_BUTTON_HELD:
            break;
    
        case BUTTON_PRESSED:
            break;
    
        case TIMED_EVENT:
            if (data[0] == TIMER_RECORDING) {
                recording_led = !recording_led;
                refresh_grid();
            }
            break;
        
        default:
            break;
    }
}

void render_arc() {}

void render_grid() {
    clear_all_grid_leds();
    render_menu();
    
    if (page == PAGE_TRACKER) render_tracker();
}

void grid_press(u8 x, u8 y, u8 pressed) {
    if (x < 2) {
        grid_press_menu(x, y, pressed);
        return;
    }
    
    if (page == PAGE_TRACKER) grid_press_tracker(x, y, pressed);
}

// ----------------------------------------------------------------------------
// main menu

void render_menu() {
    set_grid_led(0, 0, seq_on ? LED_SEQ_ON : LED_SEQ_OFF);
}

void grid_press_menu(u8 x, u8 y, u8 pressed) {
    if (x == 0 && y == 0 && pressed) {
        seq_on = !seq_on;
        if (!seq_on) set_gate(0, 0);
        recording_mode = RECORDING_OFF;
        refresh_grid();
    }
}

// ----------------------------------------------------------------------------
// tracker

void render_tracker() {
    render_tracker_menu();
    render_tracker_tracker();
}

void grid_press_tracker(u8 x, u8 y, u8 pressed) {
    if (x < 8) {
        grid_press_tracker_menu(x, y, pressed);
        return;
    }
    
    grid_press_tracker_tracker(x, y, pressed);
}

// ----------------------------------------------------------------------------
// tracker menu

void render_tracker_menu() {
    set_grid_led(5, 0, tracker_dir == TRACKER_DIR_V ? LED_MENU_ON : LED_MENU_OFF);
    set_grid_led(6, 0, follow_tracker_page ? LED_MENU_ON : LED_MENU_OFF);
    set_grid_led(5, 7, keyboard_on ? LED_MENU_ON : LED_MENU_OFF);
    
    if (recording_mode == RECORDING_OFF)
        set_grid_led(6, 7, LED_RECORDING_OFF);
    else if (recording_mode == RECORDING_ARMED)
        set_grid_led(6, 7, recording_led ? LED_RECORDING_ARMED_1 : LED_RECORDING_ARMED_2);
    else
        set_grid_led(6, 7, recording_led ? LED_RECORDING_ON_1 : LED_RECORDING_ON_2);

    u8 playing_page = e_get_current_step(&pattern) / TRACKER_LINES;
    
    for (u8 y = 0; y < tracker_page_count; y++) {
        set_grid_led(5, y + tracker_selector_y1, y == playing_page ? LED_MENU_ON : LED_MENU_OFF);
        set_grid_led(6, y + tracker_selector_y1, y == tracker_page ? LED_MENU_ON : LED_MENU_OFF);
    }
}

void grid_press_tracker_menu(u8 x, u8 y, u8 pressed) {
    if (!pressed) return;
    
    if (x == 5 && y == 0) {
        tracker_dir = tracker_dir == TRACKER_DIR_V ? TRACKER_DIR_H : TRACKER_DIR_V;
        refresh_grid();
    }
    
    else if (x == 6 && y == 0) {
        follow_tracker_page = !follow_tracker_page;
        refresh_grid();
    }
    
    else if (x == 5 && y == 7) {
        keyboard_on = !keyboard_on;
        if (!keyboard_on) {
            recording_mode = RECORDING_OFF;
        }
        refresh_grid();
    }
    
    else if (x == 6 && y == 7) {
        if (recording_mode == RECORDING_OFF) {
            recording_mode = RECORDING_ARMED;
        } else if (recording_mode == RECORDING_ARMED) {
            recording_mode = RECORDING_ON;
        } else {
            recording_mode = RECORDING_OFF;
        }
        if (recording_mode != RECORDING_OFF) {
            keyboard_on = 1;
        }
        refresh_grid();
    }

    else if (x == 5 && y >= tracker_selector_y1 && y <= tracker_selector_y2) {
        u8 page = y - tracker_selector_y1;
        s8 new_step = (e_get_current_step(&pattern) % TRACKER_LINES) + page * TRACKER_LINES;
        e_set_current_step(&pattern, new_step);
        refresh_grid();
    }

    else if (x == 6 && y >= tracker_selector_y1 && y <= tracker_selector_y2) {
        tracker_page = y - tracker_selector_y1;
        tracker_start_step = tracker_page * TRACKER_LINES;
        refresh_grid();
    }
}
    
// ----------------------------------------------------------------------------
// tracker tracker

void render_tracker_tracker() {
    u8 value, gate, step, led_on, led_off, x, y;
    s8 pitch;
    u8 current_step = e_get_current_step(&pattern);
    u8 show_keyboard = keyboard_on || edited_step != NO_STEP;
    
    if (tracker_dir == TRACKER_DIR_V) { // ||||||||
        
        for (u8 y = 0; y < TRACKER_LINES; y++) {
            
            step = y + tracker_start_step;
            led_on = step == current_step ? LED_TRIGGER_ON_2 : LED_TRIGGER_ON_1;
            led_off = step == current_step ? LED_TRIGGER_OFF_2 : LED_TRIGGER_OFF_1;
            
            // octave shift
            value = e_get_transpose(&pattern, step);
            set_grid_led(8, y, value == TRANSPOSE_DOWN ? led_on : led_off);
            set_grid_led(10, y, value == TRANSPOSE_UP ? led_on : led_off);
            
            gate = e_get_gate(&pattern, step);

            // pitch
            if (step == edited_step)
                set_grid_led(9, y, led_on);
            else if (gate != GATE_REST)
                set_grid_led(9, y, step == current_step ? LED_TRIGGER_GATE_2 : LED_TRIGGER_GATE_1);
            else
                set_grid_led(9, y, step == current_step ? LED_TRIGGER_OFF_2 : LED_TRIGGER_OFF_1);

            // resets
            if (e_get_reset(&pattern, step)) set_grid_led(11, y, led_on);
            
            if (!show_keyboard) {
                // gate
                set_grid_led(12, y, gate == GATE_ON ? led_on : led_off);
                set_grid_led(13, y, gate == GATE_TIE ? led_on : led_off);
                
                // accent/slide
                set_grid_led(14, y, e_get_accent(&pattern, step) == GATE_ON ? led_on : led_off);
                set_grid_led(15, y, e_get_slide(&pattern, step) == GATE_ON ? led_on : led_off);
            }
        }
        
        if (show_keyboard) {
            set_grid_led(12, 3, LED_KEYBOARD_REST);
            set_grid_led(12, 4, LED_KEYBOARD_REST);

            for (u8 x = 13; x < 16; x++)
                for (u8 y = 0; y < 8; y++) set_grid_led(x, y, LED_KEYBOARD_OFF);
            
            // all pitches used in current pattern
            for (u8 i = 0; i < MAX_PATTERN_LENGTH; i++)
                if (e_get_gate(&pattern, i) != GATE_REST) {
                    pitch = e_get_pitch(&pattern, i);
                    x = 15 - (pitch >> 3);
                    y = 7 - (pitch & 7);
                    set_grid_led(x, y, LED_KEYBOARD_USED);
                }
                
            // current step
            step = current_step - tracker_start_step;
            if (e_get_current_gate(&pattern) != GATE_REST && step >= 0 && step < TRACKER_LINES) {
                pitch = e_get_current_pitch(&pattern);
                x = 15 - (pitch >> 3);
                y = 7 - (pitch & 7);
                set_grid_led(x, y, LED_KEYBOARD_CURRENT);
            }

            // pressed note pitch
            if (edited_step != NO_STEP) {
                pitch = e_get_pitch(&pattern, edited_step);
                x = 15 - (pitch >> 3);
                y = 7 - (pitch & 7);
                set_grid_led(x, y, LED_KEYBOARD_STEP);
            }
            
            // keyboard note
            if (keyboard_note != -1) {
                x = 15 - (keyboard_note >> 3);
                y = 7 - (keyboard_note & 7);
                set_grid_led(x, y, LED_KEYBOARD_NOTE);
            }
        }

    } else { // ========
        
        for (u8 x = 8; x < 8 + TRACKER_LINES; x++) {
            
            step = x + tracker_start_step - 8;
            led_on = step == current_step ? LED_TRIGGER_ON_2 : LED_TRIGGER_ON_1;
            led_off = step == current_step ? LED_TRIGGER_OFF_2 : LED_TRIGGER_OFF_1;
            
            // octave shift
            value = e_get_transpose(&pattern, step);
            set_grid_led(x, 0, value == TRANSPOSE_UP ? led_on : led_off);
            set_grid_led(x, 2, value == TRANSPOSE_DOWN ? led_on : led_off);
            
            gate = e_get_gate(&pattern, step);

            // pitch
            if (step == edited_step)
                set_grid_led(x, 1, led_on);
            else if (gate != GATE_REST)
                set_grid_led(x, 1, step == current_step ? LED_TRIGGER_GATE_2 : LED_TRIGGER_GATE_1);
            else
                set_grid_led(x, 1, step == current_step ? LED_TRIGGER_OFF_2 : LED_TRIGGER_OFF_1);

            
            // resets
            if (e_get_reset(&pattern, step)) set_grid_led(x, 3, led_on);
            
            if (!show_keyboard) {
                // gate
                set_grid_led(x, 4, gate == GATE_ON ? led_on : led_off);
                set_grid_led(x, 5, gate == GATE_TIE ? led_on : led_off);
                
                // accent/slide
                set_grid_led(x, 6, e_get_accent(&pattern, step) == GATE_ON ? led_on : led_off);
                set_grid_led(x, 7, e_get_slide(&pattern, step) == GATE_ON ? led_on : led_off);
            }
        }

        if (show_keyboard) {
            set_grid_led(11, 4, LED_KEYBOARD_REST);
            set_grid_led(12, 4, LED_KEYBOARD_REST);

            for (u8 x = 8; x < 16; x++)
                for (u8 y = 5; y < 8; y++) set_grid_led(x, y, LED_KEYBOARD_OFF);
            
            // all pitches used in current pattern
            for (u8 i = 0; i < MAX_PATTERN_LENGTH; i++)
                if (e_get_gate(&pattern, i) != GATE_REST) {
                    pitch = e_get_pitch(&pattern, i);
                    x = 8 + (pitch & 7);
                    y = 7 - (pitch >> 3);
                    set_grid_led(x, y, LED_KEYBOARD_USED);
                }
            
            // current step
            step = current_step - tracker_start_step;
            if (e_get_current_gate(&pattern) != GATE_REST && step >= 0 && step < TRACKER_LINES) {
                pitch = e_get_current_pitch(&pattern);
                x = 8 + (pitch & 7);
                y = 7 - (pitch >> 3);
                set_grid_led(x, y, LED_KEYBOARD_CURRENT);
            }

            // pressed note pitch
            if (edited_step != NO_STEP) {
                pitch = e_get_pitch(&pattern, edited_step);
                x = 8 + (pitch & 7);
                y = 7 - (pitch >> 3);
                set_grid_led(x, y, LED_KEYBOARD_STEP);
            }
            
            // keyboard note
            if (keyboard_note != -1) {
                x = 8 + (keyboard_note & 7);
                y = 7 - (keyboard_note >> 3);
                set_grid_led(x, y, LED_KEYBOARD_NOTE);
            }
        }
    }
}

void grid_press_tracker_tracker(u8 x, u8 y, u8 pressed) {
    u8 value;
    x -= 8;
    if (tracker_dir == TRACKER_DIR_H) {
        value = x;
        x = y;
        y = value;
    }
    
    u8 step = y + tracker_start_step;
    
    if (x == 0) {
        if (!pressed) return;
        value = tracker_dir == TRACKER_DIR_V ? TRANSPOSE_DOWN : TRANSPOSE_UP;
        e_set_transpose(&pattern, step, e_get_transpose(&pattern, step) == value ? TRANSPOSE_OFF : value);
        refresh_grid();
        return;
    }
    
    if (x == 2) {
        if (!pressed) return;
        value = tracker_dir == TRACKER_DIR_V ? TRANSPOSE_UP : TRANSPOSE_DOWN;
        e_set_transpose(&pattern, step, e_get_transpose(&pattern, step) == value ? TRANSPOSE_OFF : value);
        refresh_grid();
        return;
    }
    
    if (x == 3) {
        if (!pressed) return;
        e_set_reset(&pattern, step, !e_get_reset(&pattern, step));
        refresh_grid();
        return;
    }
    
    if (x == 1) {
        if (pressed) {
            edited_step = step;
            if (!seq_on) e_set_current_step(&pattern, step);
        } else {
            if (edited_step == step) edited_step = NO_STEP;
        }
        refresh_grid();
        return;
    }
    
    if (keyboard_on || edited_step != NO_STEP) { // keyboard note selection
        if (x == 4) { // rest
            if ((y != 3 && y != 4) || !pressed) return;
            e_set_gate(&pattern, edited_step, GATE_REST);
            refresh_grid();
            return;
        }
        
        if (tracker_dir == TRACKER_DIR_H) y = 7 - y;
        u8 note = 7 - y + ((7 - x) << 3);
        
        if (pressed) {
            if (edited_step != NO_STEP) {
                e_set_pitch(&pattern, edited_step, note);
                e_set_gate(&pattern, edited_step, GATE_ON);
                set_cv(0, note_to_pitch(e_get_pitch_transposed(&pattern, edited_step) + TRANSPOSE_OUTPUT));
                set_gate(0, 1);
            } else {
                keyboard_note = note;
                set_cv(0, note_to_pitch(note + TRANSPOSE_OUTPUT));
                set_gate(0, 1);
            }
        } else {
            if (edited_step != NO_STEP) {
                if (!seq_on) set_gate(0, 0);
            } else {
                if (note == keyboard_note) {
                    if (!seq_on) set_gate(0, 0);
                    keyboard_note = -1;
                }
            }
        }
        
        refresh_grid();
        return;
    }
    
    if (!pressed) return;

    switch (x) {
        case 4:
            value = e_get_gate(&pattern, step);
            e_set_gate(&pattern, step, value == GATE_ON ? GATE_REST : GATE_ON);
            break;
        case 5:
            value = e_get_gate(&pattern, step);
            e_set_gate(&pattern, step, value == GATE_TIE ? GATE_REST : GATE_TIE);
            break;
        case 6:
            e_set_accent(&pattern, step, !e_get_accent(&pattern, step));
            break;
        case 7:
            e_set_slide(&pattern, step, !e_get_slide(&pattern, step));
            break;
        default:
            break;
    }
        
    refresh_grid();
}

// ----------------------------------------------------------------------------
// sequencer

void step() {
    if (!seq_on) return;
    
    e_step(&pattern);
    set_cv(0, note_to_pitch(e_get_current_pitch_transposed(&pattern) + TRANSPOSE_OUTPUT));
    set_gate(0, e_get_current_gate(&pattern) != GATE_REST);
    set_gate(1, e_get_current_accent(&pattern));
    set_gate(2, e_get_current_slide(&pattern));
    
    if (follow_tracker_page) {
        tracker_page = e_get_current_step(&pattern) / TRACKER_LINES;
        tracker_start_step = tracker_page * TRACKER_LINES;
    }

    refresh_grid();
}

void step_off() {
    if (!seq_on) return;
    
    if (e_get_current_gate(&pattern) != GATE_TIE) set_gate(0, 0);
}
