// -----------------------------------------------------------------------------
// controller - the glue between the engine and the hardware
//
// reacts to events (grid press, clock etc) and translates them into appropriate
// engine actions. reacts to engine updates and translates them into user 
// interface and hardware updates (grid LEDs, CV outputs etc)
//
// should talk to hardware via what's defined in interface.h only
// should talk to the engine via what's defined in engine.h only
// ----------------------------------------------------------------------------

#include "compiler.h"
#include "string.h"

#include "music.h" // TODO remove

#include "control.h"
#include "interface.h"
#include "engine.h"

preset_meta_t meta;
preset_data_t preset;
shared_data_t shared;
int selected_preset;

// ----------------------------------------------------------------------------
// firmware dependent stuff starts here

#define TRANSPOSE_OUTPUT 36

#define OUTPUT_PITCH  0
#define OUTPUT_GATE   0
#define OUTPUT_ACCENT 1
#define OUTPUT_SLIDE  2

#define PAGE_TRACKER 0

#define TRACKER_DIR_V 0
#define TRACKER_DIR_H 1

#define NO_STEP 255

#define LED_PITCH 12
#define LED_TRIGGER_ON 12
#define LED_TRIGGER_OFF 3
#define LED_CURRENT_STEP 3
#define LED_PIANO_W 12
#define LED_PIANO_B 6
#define LED_MENU_ON 15
#define LED_MENU_OFF 4

// pattern
engine_pattern_t pattern;

// ui
u8 page, tracker_dir, piano_on, step_mode, edited_step, piano_note, follow_page;
u8 tracker_page_count, tracker_page, tracker_page_y1, tracker_page_y2, tracker_start_step;

// sequencer
u8 last_gate, last_accent, last_slide;

// settings
u8 setting_303_slide = 1;                       // [1]
u8 setting_hold_accent_on_ties_and_rests = 1;   // [2]
u8 setting_gate_stays_high_before_slide = 1;    // [3]
u8 setting_slide_is_high_on_ties_and_rests = 1; // [4]


static void grid_press(u8 x, u8 y, u8 pressed);
static void grid_press_menu(u8 x, u8 y, u8 pressed);
static void grid_press_tracker(u8 x, u8 y, u8 pressed);

static void render_menu(void);
static void render_tracker(void);

static void step(void);
static void step_off(void);


// ----------------------------------------------------------------------------
// functions for main.c

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
    
    page = PAGE_TRACKER;
    tracker_dir = TRACKER_DIR_V;
    piano_on = 0;
    step_mode = 0;
    edited_step = 0;
    follow_page = 0;
    
    tracker_page_count = MAX_PATTERN_LENGTH >> 3;
    tracker_page = 0;
    tracker_page_y1 = (8 - tracker_page_count) >> 1;
    tracker_page_y2 = tracker_page_y1 + tracker_page_count - 1;
    tracker_start_step = 0;
    
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
    
        case I2C_RECEIVED:
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

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void grid_press(u8 x, u8 y, u8 pressed) {
    if (x < 5) {
        grid_press_menu(x, y, pressed);
        return;
    }
    
    if (page == PAGE_TRACKER) grid_press_tracker(x, y, pressed);
}

void grid_press_menu(u8 x, u8 y, u8 pressed) {
}

void grid_press_tracker(u8 x, u8 y, u8 pressed) {
    if (pressed && x == 5 && y == 0) {
        tracker_dir = tracker_dir == TRACKER_DIR_V ? TRACKER_DIR_H : TRACKER_DIR_V;
        refresh_grid();
        return;
    }
    
    if (pressed && x == 6 && y == 0) {
        follow_page = !follow_page;
        refresh_grid();
        return;
    }
    
    if (pressed && x == 5 && y == 7) {
        step_mode = !step_mode;
        piano_on = step_mode;
        refresh_grid();
        return;
    }
    
    if (pressed && x == 5 && y >= tracker_page_y1 && y <= tracker_page_y2) {
        u8 page = y - tracker_page_y1;
        s8 new_step = (e_get_current_step(&pattern) & 7) + (page << 3);
        e_set_current_step(&pattern, new_step);
        refresh_grid();
        return;
    }

    if (pressed && x == 6 && y >= tracker_page_y1 && y <= tracker_page_y2) {
        tracker_page = y - tracker_page_y1;
        tracker_start_step = tracker_page << 3;
        refresh_grid();
        return;
    }

    if (x < 8) return;
    u8 value;
    x -= 8;
    if (tracker_dir == TRACKER_DIR_H) {
        value = x;
        x = y;
        y = value;
    }
    
    /*
    if (!pressed) {
        if (edited_step == y) edited_step = NO_STEP;
        return;
    }
    */
    
    if (pressed && x == 1) {
        edited_step = y;
        return;
    }

    if (step_mode && x > 3) {
        if (tracker_dir == TRACKER_DIR_H) x = 11 - x;
        
        u8 note;
        if (x < 6 && y == 0)
            note = 0;
        else if (x >= 6 && y == 1)
            note = 1;
        else if (x < 6 && y == 1)
            note = 2;
        else if (x >= 6 && y == 2)
            note = 3;
        else if (x < 6 && y == 2)
            note = 4;
        else if (x < 6 && y == 3)
            note = 5;
        else if (x >= 6 && y == 4)
            note = 6;
        else if (x < 6 && y == 4)
            note = 7;
        else if (x >= 6 && y == 5)
            note = 8;
        else if (x < 6 && y == 5)
            note = 9;
        else if (x >= 6 && y == 6)
            note = 10;
        else if (x < 6 && y == 6)
            note = 11;
        else
            note = NO_STEP;
        if (pressed) {
            piano_note = note;
            if (piano_note != NO_STEP) {
                set_cv(OUTPUT_PITCH, ET[piano_note + TRANSPOSE_OUTPUT]);
                set_gate(OUTPUT_GATE, 1);
            }
        } else if (piano_note == note) {
            piano_note = NO_STEP;
            set_gate(OUTPUT_GATE, 0);
        }
        return;
    }
    
    if (!pressed) return;
    u8 step = y + tracker_start_step;

    switch (x) {
        case 0:
            value = tracker_dir == TRACKER_DIR_V ? TRANSPOSE_DOWN : TRANSPOSE_UP;
            e_set_transpose(&pattern, step, e_get_transpose(&pattern, step) == value ? TRANSPOSE_OFF : value);
            break;
        case 2:
            value = tracker_dir == TRACKER_DIR_V ? TRANSPOSE_UP : TRANSPOSE_DOWN;
            e_set_transpose(&pattern, step, e_get_transpose(&pattern, step) == value ? TRANSPOSE_OFF : value);
            break;
        case 3:
            e_set_reset(&pattern, step, !e_get_reset(&pattern, step));
            break;
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

void render_menu() {
}

void render_tracker() {
    s8 value, step;
    s8 current_step = e_get_current_step(&pattern) - tracker_start_step;
    
    if (tracker_dir == TRACKER_DIR_V) { // ||||||||
        
        for (u8 y = 0; y < 8; y++) {
            
            step = y + tracker_start_step;
            
            // octave shift
            value = e_get_transpose(&pattern, step);
            set_grid_led(8, y, value == TRANSPOSE_DOWN ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
            set_grid_led(10, y, value == TRANSPOSE_UP ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
            
            // pitch
            set_grid_led(9, y, e_get_pitch(&pattern, step) == PITCH_REST ? LED_TRIGGER_OFF : LED_PITCH);
            
            // resets
            if (e_get_reset(&pattern, step)) set_grid_led(11, y, LED_TRIGGER_ON);
            
            if (!piano_on) {
                // gate
                value = e_get_gate(&pattern, step);
                set_grid_led(12, y, value == GATE_ON ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
                set_grid_led(13, y, value == GATE_TIE ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
                
                // accent/slide
                set_grid_led(14, y, e_get_accent(&pattern, step) == GATE_ON ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
                set_grid_led(15, y, e_get_slide(&pattern, step) == GATE_ON ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
            }
        }
        
        if (current_step >= 0 && current_step <= 7) {
            for (u8 x = 8; x < 11; x++)
                set_grid_led(x, current_step, get_grid_led(x, current_step) + LED_CURRENT_STEP);
            
            if (!piano_on)
                for (u8 x = 12; x < 16; x++)
                    set_grid_led(x, current_step, get_grid_led(x, current_step) + LED_CURRENT_STEP);
        }
            
        if (piano_on) {
            set_grid_led(12, 0, LED_PIANO_W);
            set_grid_led(13, 0, LED_PIANO_W);
            
            set_grid_led(12, 1, LED_PIANO_W);
            set_grid_led(13, 1, LED_PIANO_W);
            
            set_grid_led(14, 1, LED_PIANO_B);
            set_grid_led(15, 1, LED_PIANO_B);
            
            set_grid_led(12, 2, LED_PIANO_W);
            set_grid_led(13, 2, LED_PIANO_W);
            
            set_grid_led(14, 2, LED_PIANO_B);
            set_grid_led(15, 2, LED_PIANO_B);
            
            set_grid_led(12, 3, LED_PIANO_W);
            set_grid_led(13, 3, LED_PIANO_W);
            
            set_grid_led(12, 4, LED_PIANO_W);
            set_grid_led(13, 4, LED_PIANO_W);
            
            set_grid_led(14, 4, LED_PIANO_B);
            set_grid_led(15, 4, LED_PIANO_B);
            
            set_grid_led(12, 5, LED_PIANO_W);
            set_grid_led(13, 5, LED_PIANO_W);
            
            set_grid_led(14, 5, LED_PIANO_B);
            set_grid_led(15, 5, LED_PIANO_B);
            
            set_grid_led(12, 6, LED_PIANO_W);
            set_grid_led(13, 6, LED_PIANO_W);
            
            set_grid_led(14, 6, LED_PIANO_B);
            set_grid_led(15, 6, LED_PIANO_B);
        }

    } else { // ========
        
        for (u8 x = 8; x < 16; x++) {
            
            step = x + tracker_start_step - 8;
            
            // octave shift
            value = e_get_transpose(&pattern, step);
            set_grid_led(x, 0, value == TRANSPOSE_UP ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
            set_grid_led(x, 2, value == TRANSPOSE_DOWN ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
            
            // pitch
            set_grid_led(x, 1, e_get_pitch(&pattern, step) == PITCH_REST ? LED_TRIGGER_OFF : LED_PITCH);
            
            // resets
            if (e_get_reset(&pattern, step)) set_grid_led(x, 3, LED_TRIGGER_ON);
            
            if (!piano_on) {
                // gate
                value = e_get_gate(&pattern, step);
                set_grid_led(x, 4, value == GATE_ON ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
                set_grid_led(x, 5, value == GATE_TIE ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
                
                // accent/slide
                set_grid_led(x, 6, e_get_accent(&pattern, step) == GATE_ON ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
                set_grid_led(x, 7, e_get_slide(&pattern, step) == GATE_ON ? LED_TRIGGER_ON : LED_TRIGGER_OFF);
            }
        }
        
        if (current_step >= 0 && current_step <= 7) {
            for (u8 y = 0; y < 3; y++)
                set_grid_led(current_step + 8, y, get_grid_led(current_step + 8, y) + LED_CURRENT_STEP);
            
            if (!piano_on)
                for (u8 y = 4; y < 8; y++)
                    set_grid_led(current_step + 8, y, get_grid_led(current_step + 8, y) + LED_CURRENT_STEP);
        }
    
        if (piano_on) {
            set_grid_led(8, 6, LED_PIANO_W);
            set_grid_led(8, 7, LED_PIANO_W);
            
            set_grid_led(9, 6, LED_PIANO_W);
            set_grid_led(9, 7, LED_PIANO_W);
            
            set_grid_led(9, 4, LED_PIANO_B);
            set_grid_led(9, 5, LED_PIANO_B);
            
            set_grid_led(10, 6, LED_PIANO_W);
            set_grid_led(10, 7, LED_PIANO_W);
            
            set_grid_led(10, 4, LED_PIANO_B);
            set_grid_led(10, 5, LED_PIANO_B);
            
            set_grid_led(11, 6, LED_PIANO_W);
            set_grid_led(11, 7, LED_PIANO_W);
            
            set_grid_led(12, 6, LED_PIANO_W);
            set_grid_led(12, 7, LED_PIANO_W);
            
            set_grid_led(12, 4, LED_PIANO_B);
            set_grid_led(12, 5, LED_PIANO_B);
            
            set_grid_led(13, 6, LED_PIANO_W);
            set_grid_led(13, 7, LED_PIANO_W);
            
            set_grid_led(13, 4, LED_PIANO_B);
            set_grid_led(13, 5, LED_PIANO_B);
            
            set_grid_led(14, 6, LED_PIANO_W);
            set_grid_led(14, 7, LED_PIANO_W);
            
            set_grid_led(14, 4, LED_PIANO_B);
            set_grid_led(14, 5, LED_PIANO_B);
        }
        
    }
    
    u8 playing_page = e_get_current_step(&pattern) >> 3;
    
    for (u8 y = 0; y < tracker_page_count; y++) {
        set_grid_led(5, y + tracker_page_y1, y == playing_page ? LED_MENU_ON : LED_MENU_OFF);
        set_grid_led(6, y + tracker_page_y1, y == tracker_page ? LED_MENU_ON : LED_MENU_OFF);
    }
    
    set_grid_led(5, 0, tracker_dir == TRACKER_DIR_V ? LED_MENU_ON : LED_MENU_OFF);
    set_grid_led(6, 0, follow_page ? LED_MENU_ON : LED_MENU_OFF);
    set_grid_led(5, 7, step_mode ? LED_MENU_ON : LED_MENU_OFF);
}

// ----------------------------------------------------------------------------

void step() {
    if (step_mode) return;
    
    e_step(&pattern);
    set_cv(OUTPUT_PITCH, ET[e_get_current_pitch_value(&pattern) + TRANSPOSE_OUTPUT]);
    set_gate(OUTPUT_GATE, e_get_current_gate(&pattern) != GATE_REST);
    set_gate(OUTPUT_ACCENT, e_get_current_accent(&pattern));
    set_gate(OUTPUT_SLIDE, e_get_current_slide(&pattern));
    
    if (follow_page) {
        tracker_page = e_get_current_step(&pattern) >> 3;
        tracker_start_step = tracker_page << 3;
    }

    refresh_grid();
}

void step_off() {
    if (step_mode) return;
    
    if (e_get_current_gate(&pattern) != GATE_TIE) set_gate(OUTPUT_GATE, 0);
}
