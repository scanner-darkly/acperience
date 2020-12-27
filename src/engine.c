// ----------------------------------------------------------------------------
// app engine
//
// hardware agnostic application logic
// sends updates to control and provides actions for control to call
// ----------------------------------------------------------------------------

#include "engine.h"
#include "control.h"

void e_init(engine_pattern_t *ep) {
    for (int i = 0; i < MAX_PATTERN_LENGTH; i++) {
        ep->p.steps[i].pitch = PITCH_REST;
        ep->p.steps[i].gate = GATE_REST;
        ep->p.steps[i].accent = 0;
        ep->p.steps[i].slide = 0;
        ep->p.steps[i].transpose = TRANSPOSE_OFF;
    }
    
    ep->ps.current_step = 0;
    for (int i = 0; i < MAX_PATTERN_LENGTH; i++) ep->ps.determined_pitch[i] = 0;
}

void e_reset(engine_pattern_t *ep) {
    ep->ps.current_step = 0;
}

void e_step(engine_pattern_t *ep) {
    if (ep->p.steps[ep->ps.current_step].is_reset || ++ep->ps.current_step >= MAX_PATTERN_LENGTH)
        ep->ps.current_step = 0;
}

// ----------------------------------------------------------------------------

u8 e_get_current_step(engine_pattern_t *ep) {
    return ep->ps.current_step;
}

void e_set_current_step(engine_pattern_t *ep, u8 step) {
    if (step >= MAX_PATTERN_LENGTH) return;
    
    ep->ps.current_step = step;
}

// ----------------------------------------------------------------------------

s8 e_get_pitch(engine_pattern_t *ep, u8 step) {
    return ep->p.steps[step].pitch;
}

void e_set_pitch(engine_pattern_t *ep, u8 step, s8 pitch) {
    if (step >= MAX_PATTERN_LENGTH) return;
    if (pitch != PITCH_REST && pitch > MAX_PITCH_VALUE) return;
    
    ep->p.steps[step].pitch = pitch;
    u8 last = 0;
    for (int i = 0; i < MAX_PATTERN_LENGTH; i++) {
        if (ep->p.steps[i].pitch != PITCH_REST) last = ep->p.steps[i].pitch;
        ep->ps.determined_pitch[i] = last;
    }
    for (int i = 0; i < MAX_PATTERN_LENGTH; i++) {
        if (ep->p.steps[i].pitch != PITCH_REST) break;
        ep->ps.determined_pitch[i] = last;
    }
}

u8 e_get_determined_pitch(engine_pattern_t *ep, u8 step) {
    return ep->ps.determined_pitch[step];
}

u8 e_get_current_determined_pitch(engine_pattern_t *ep) {
    return ep->ps.determined_pitch[ep->ps.current_step];
}

s8 e_get_current_pitch_value(engine_pattern_t *ep) {
    s8 pitch = ep->ps.determined_pitch[ep->ps.current_step];
    if (ep->p.steps[ep->ps.current_step].transpose == TRANSPOSE_UP) pitch += 12;
    else if (ep->p.steps[ep->ps.current_step].transpose == TRANSPOSE_DOWN) pitch -= 12;
    return pitch;
}

// ----------------------------------------------------------------------------

u8 e_get_current_gate(engine_pattern_t *ep) {
    return ep->p.steps[ep->ps.current_step].gate;
}

u8 e_get_gate(engine_pattern_t *ep, u8 step) {
    return ep->p.steps[step].gate;
}

void e_set_gate(engine_pattern_t *ep, u8 step, u8 gate) {
    if (step >= MAX_PATTERN_LENGTH) return;
    
    ep->p.steps[step].gate = gate;
}

// ----------------------------------------------------------------------------

u8 e_get_current_accent(engine_pattern_t *ep) {
    return ep->p.steps[ep->ps.current_step].accent;
}

u8 e_get_accent(engine_pattern_t *ep, u8 step) {
    return ep->p.steps[step].accent;
}

void e_set_accent(engine_pattern_t *ep, u8 step, u8 accent) {
    if (step >= MAX_PATTERN_LENGTH) return;
    
    ep->p.steps[step].accent = accent;
}

// ----------------------------------------------------------------------------

u8 e_get_current_slide(engine_pattern_t *ep) {
    return ep->p.steps[ep->ps.current_step].slide;
}

u8 e_get_slide(engine_pattern_t *ep, u8 step) {
    return ep->p.steps[step].slide;
}

void e_set_slide(engine_pattern_t *ep, u8 step, u8 slide) {
    if (step >= MAX_PATTERN_LENGTH) return;
    
    ep->p.steps[step].slide = slide;
}

// ----------------------------------------------------------------------------

u8 e_get_current_transpose(engine_pattern_t *ep) {
    return ep->p.steps[ep->ps.current_step].transpose;
}

u8 e_get_transpose(engine_pattern_t *ep, u8 step) {
    return ep->p.steps[step].transpose;
}

void e_set_transpose(engine_pattern_t *ep, u8 step, u8 transpose) {
    if (step >= MAX_PATTERN_LENGTH) return;
    
    ep->p.steps[step].transpose = transpose;
}

// ----------------------------------------------------------------------------

u8 e_get_reset(engine_pattern_t *ep, u8 step) {
    return ep->p.steps[step].is_reset;
}

void e_set_reset(engine_pattern_t *ep, u8 step, u8 is_reset) {
    if (step >= MAX_PATTERN_LENGTH) return;
    
    ep->p.steps[step].is_reset = is_reset;
}
