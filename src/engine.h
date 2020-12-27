// ----------------------------------------------------------------------------
// defines engine actions available to control
// ----------------------------------------------------------------------------

#pragma once
#include "types.h"

#define MAX_PATTERN_LENGTH 32
#define MAX_PITCH_VALUE 11

#define PITCH_REST -1

#define GATE_REST 0
#define GATE_ON   1
#define GATE_TIE  2

#define TRANSPOSE_OFF  0
#define TRANSPOSE_UP   1
#define TRANSPOSE_DOWN 2

typedef struct {
    s8 pitch; // -1 means not specified (PITCH_REST)
    u8 gate;
    u8 accent;
    u8 slide;
    u8 transpose;
    u8 is_reset;
} step_t;

typedef struct {
    step_t steps[MAX_PATTERN_LENGTH];
} pattern_t;

typedef struct {
    s8 current_step;
    u8 determined_pitch[MAX_PATTERN_LENGTH];
} pattern_state_t;

typedef struct {
    pattern_t p;
    pattern_state_t ps;
} engine_pattern_t;

void e_init(engine_pattern_t *ep);
void e_reset(engine_pattern_t *ep);
void e_step(engine_pattern_t *ep);

u8 e_get_current_step(engine_pattern_t *ep);
void e_set_current_step(engine_pattern_t *ep, u8 step);

s8 e_get_pitch(engine_pattern_t *ep, u8 step);
void e_set_pitch(engine_pattern_t *ep, u8 step, s8 pitch);

u8 e_get_determined_pitch(engine_pattern_t *ep, u8 step);
u8 e_get_current_determined_pitch(engine_pattern_t *ep);
s8 e_get_current_pitch_value(engine_pattern_t *ep);

u8 e_get_current_gate(engine_pattern_t *ep);
u8 e_get_gate(engine_pattern_t *ep, u8 step);
void e_set_gate(engine_pattern_t *ep, u8 step, u8 gate);

u8 e_get_current_accent(engine_pattern_t *ep);
u8 e_get_accent(engine_pattern_t *ep, u8 step);
void e_set_accent(engine_pattern_t *ep, u8 step, u8 accent);

u8 e_get_current_slide(engine_pattern_t *ep);
u8 e_get_slide(engine_pattern_t *ep, u8 step);
void e_set_slide(engine_pattern_t *ep, u8 step, u8 slide);

u8 e_get_current_transpose(engine_pattern_t *ep);
u8 e_get_transpose(engine_pattern_t *ep, u8 step);
void e_set_transpose(engine_pattern_t *ep, u8 step, u8 transpose);

u8 e_get_reset(engine_pattern_t *ep, u8 step);
void e_set_reset(engine_pattern_t *ep, u8 step, u8 is_reset);
