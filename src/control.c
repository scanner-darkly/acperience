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

#define TRANSPOSE_OUTPUT 36
#define NO_STEP 255

#define PAGE_TRACKER 0
#define TRACKER_DIR_V 0
#define TRACKER_DIR_H 1
#define TRACKER_LINES 8

#define LED_TRIGGER_ON 10
#define LED_TRIGGER_OFF 3
#define LED_CURRENT_STEP 3
#define LED_KEYBOARD_OFF 3
#define LED_KEYBOARD_CURRENT 6
#define LED_KEYBOARD_KEY 12
#define LED_KEYBOARD_REST 3
#define LED_MENU_ON 15
#define LED_MENU_OFF 4

// pattern
engine_pattern_t pattern;

// sequencer
u8 seq_on;

// ui
u8 page, tracker_dir, follow_tracker_page;
u8 tracker_page_count, tracker_selector_y1, tracker_selector_y2;
u8 tracker_page, tracker_start_step;
u8 step_mode, edited_step;
u8 keyboard_latched;
s8 keyboard_note;

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

    step_mode = 0;
    edited_step = NO_STEP;

    keyboard_latched = 0;
    keyboard_note = -1;
    
    refresh_grid();
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
    set_grid_led(0, 0, seq_on ? LED_MENU_ON : LED_MENU_OFF);
}

void grid_press_menu(u8 x, u8 y, u8 pressed) {
    if (x == 0 && y == 0 && pressed) {
        seq_on = !seq_on;
        if (!seq_on) set_gate(0, 0);
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
    set_grid_led(5, 7, step_mode ? LED_MENU_ON : LED_MENU_OFF);
    set_grid_led(6, 7, keyboard_latched ? LED_MENU_ON : LED_MENU_OFF);

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
        step_mode = !step_mode;
        if (step_mode && edited_step == NO_STEP) edited_step = 0;
        else if (!keyboard_latched && !step_mode) edited_step = NO_STEP;
        refresh_grid();
    }
    
    else if (x == 6 && y == 7 && !step_mode) {
        keyboard_latched = !keyboard_latched;
        if (keyboard_latched && !step_mode && edited_step == NO_STEP) {
            s8 current_step = e_get_current_step(&pattern) - tracker_start_step;
            edited_step = (current_step >= 0 && current_step < TRACKER_LINES) ? current_step : 0;
        }
        else if (!keyboard_latched && !step_mode) edited_step = NO_STEP;
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
    s8 value, step;
    s8 current_step = e_get_current_step(&pattern) - tracker_start_step;
    u8 show_keyboard = edited_step != NO_STEP;
    
    if (tracker_dir == TRACKER_DIR_V) { // ||||||||
        
        for (u8 y = 0; y < TRACKER_LINES; y++) {
            
            step = y + tracker_start_step;
            
            // octave shift
            value = e_get_transpose(&pattern, step);
            set_grid_led(8, y, value == TRANSPOSE_DOWN ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
            set_grid_led(10, y, value == TRANSPOSE_UP ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
            
            // pitch
            set_grid_led(9, y, (y == edited_step ? LED_TRIGGER_ON : LED_TRIGGER_OFF) + (value ? LED_CURRENT_STEP : 0));
            value = e_get_gate(&pattern, step);
            if (show_keyboard && value) set_grid_led(9, y, get_grid_led(9, y) + 2);
            
            // resets
            if (e_get_reset(&pattern, step)) set_grid_led(11, y, LED_TRIGGER_ON);
            
            if (!show_keyboard) {
                // gate
                set_grid_led(12, y, value == GATE_ON ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
                set_grid_led(13, y, value == GATE_TIE ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
                
                // accent/slide
                set_grid_led(14, y, e_get_accent(&pattern, step) == GATE_ON ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
                set_grid_led(15, y, e_get_slide(&pattern, step) == GATE_ON ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
            }
        }
        
        if (current_step >= 0 && current_step < TRACKER_LINES) {
            for (u8 x = 8; x < 11; x++)
                set_grid_led(x, current_step, get_grid_led(x, current_step) + LED_CURRENT_STEP);
            
            if (!show_keyboard)
                for (u8 x = 12; x < 16; x++)
                    set_grid_led(x, current_step, get_grid_led(x, current_step) + LED_CURRENT_STEP);
        }
            
        if (show_keyboard) {
            for (u8 x = 13; x < 16; x++)
                for (u8 y = 0; y < 8; y++) set_grid_led(x, y, LED_KEYBOARD_OFF);
            
            set_grid_led(12, 3, LED_KEYBOARD_REST);
            set_grid_led(12, 4, LED_KEYBOARD_REST);
            
            if (keyboard_note != -1) 
                set_grid_led(15 - (keyboard_note >> 3), 7 - (keyboard_note & 7), LED_KEYBOARD_KEY);

            value = e_get_current_pitch(&pattern);
            u8 x = 15 - (value >> 3);
            u8 y = 7 - (value & 7);
            set_grid_led(x, y, get_grid_led(x, y) + LED_KEYBOARD_OFF);
        }

    } else { // ========
        
        for (u8 x = 8; x < 8 + TRACKER_LINES; x++) {
            
            step = x + tracker_start_step - 8;
            
            // octave shift
            value = e_get_transpose(&pattern, step);
            set_grid_led(x, 0, value == TRANSPOSE_UP ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
            set_grid_led(x, 2, value == TRANSPOSE_DOWN ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
            
            // pitch
            set_grid_led(x, 1, x == 8 + edited_step ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
            value = e_get_gate(&pattern, step);
            if (show_keyboard && value) set_grid_led(x, 1, get_grid_led(x, 1) + 2);
            
            // resets
            if (e_get_reset(&pattern, step)) set_grid_led(x, 3, LED_TRIGGER_ON);
            
            if (!show_keyboard) {
                // gate
                set_grid_led(x, 4, value == GATE_ON ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
                set_grid_led(x, 5, value == GATE_TIE ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
                
                // accent/slide
                set_grid_led(x, 6, e_get_accent(&pattern, step) == GATE_ON ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
                set_grid_led(x, 7, e_get_slide(&pattern, step) == GATE_ON ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
            }
        }

        if (current_step >= 0 && current_step < TRACKER_LINES) {
            for (u8 y = 0; y < 3; y++)
                set_grid_led(current_step + 8, y, get_grid_led(current_step + 8, y) + LED_CURRENT_STEP);
            
            if (!show_keyboard)
                for (u8 y = 4; y < 8; y++)
                    set_grid_led(current_step + 8, y, get_grid_led(current_step + 8, y) + LED_CURRENT_STEP);
        }
    
        if (show_keyboard) {
            for (u8 x = 8; x < 16; x++)
                for (u8 y = 5; y < 8; y++) set_grid_led(x, y, LED_KEYBOARD_OFF);
            
            set_grid_led(11, 4, LED_KEYBOARD_REST);
            set_grid_led(12, 4, LED_KEYBOARD_REST);
            
            if (keyboard_note != -1)
                set_grid_led(8 + (keyboard_note & 7), 7 - (keyboard_note >> 3), LED_KEYBOARD_KEY);
            
            value = e_get_current_pitch(&pattern);
            u8 x = 8 + (value & 7);
            u8 y = 7 - (value >> 3);
            set_grid_led(x, y, get_grid_led(x, y) + LED_KEYBOARD_OFF);
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
            edited_step = y;
            keyboard_note = e_get_pitch(&pattern, edited_step);
            if (!seq_on) {
                e_set_current_step(&pattern, step);
                set_cv(0, note_to_pitch(e_get_current_pitch_transposed(&pattern) + TRANSPOSE_OUTPUT));
                set_gate(0, 1);
            }
        } else {
            if (!step_mode && !keyboard_latched && edited_step == y) {
                edited_step = NO_STEP;
                keyboard_note = -1;
            }
            if (!seq_on) set_gate(0, 0);
        }
        refresh_grid();
        return;
    }
    
    if (edited_step != NO_STEP) { // keyboard note selection
        step = edited_step + tracker_start_step;
        
        if (x == 4)  {
            if ((y != 3 && y != 4) || !pressed) return;
            e_set_gate(&pattern, step, GATE_REST);
            if (step_mode) edited_step = (edited_step + 1) % TRACKER_LINES; // FIXME should go to loop start
            refresh_grid();
            return;
        }
        
        if (tracker_dir == TRACKER_DIR_H) y = 7 - y;
        u8 note = 7 - y + ((7 - x) << 3);
        
        if (pressed) {
            keyboard_note = note;
            
            e_set_pitch(&pattern, step, keyboard_note);
            e_set_gate(&pattern, step, GATE_ON);
            if (step_mode) edited_step = (edited_step + 1) % TRACKER_LINES; // FIXME should go to loop start
            
            set_cv(0, note_to_pitch(e_get_pitch_transposed(&pattern, step) + TRANSPOSE_OUTPUT));
            set_gate(0, 1);
            
        } else {
            if (note == keyboard_note) {
                if (!seq_on) set_gate(0, 0);
                keyboard_note = -1;
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
