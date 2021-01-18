// ----------------------------------------------------------------------------
// acperience (c) scanner darkly 2021
// ----------------------------------------------------------------------------

#include "engine.h"
#include "control.h"

void e_init(engine_pattern_t *ep) {
    for (int i = 0; i < MAX_PATTERN_LENGTH; i++) {
        ep->p.steps[i].pitch = 0;
        ep->p.steps[i].gate = GATE_REST;
        ep->p.steps[i].accent = 0;
        ep->p.steps[i].slide = 0;
        ep->p.steps[i].transpose = TRANSPOSE_OFF;
    }
    
    ep->ps.current_step = 0;
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

s8 e_get_pitch_transposed(engine_pattern_t *ep, u8 step) {
    s8 pitch = e_get_pitch(ep, step);
    if (ep->p.steps[step].transpose == TRANSPOSE_UP) pitch += 12;
    else if (ep->p.steps[step].transpose == TRANSPOSE_DOWN) pitch -= 12;
    return pitch;
}

s8 e_get_current_pitch(engine_pattern_t *ep) {
    return ep->p.steps[ep->ps.current_step].pitch;
}

s8 e_get_current_pitch_transposed(engine_pattern_t *ep) {
    s8 pitch = e_get_current_pitch(ep);
    if (ep->p.steps[ep->ps.current_step].transpose == TRANSPOSE_UP) pitch += 12;
    else if (ep->p.steps[ep->ps.current_step].transpose == TRANSPOSE_DOWN) pitch -= 12;
    return pitch;
}

void e_set_pitch(engine_pattern_t *ep, u8 step, s8 pitch) {
    if (step >= MAX_PATTERN_LENGTH) return;
    ep->p.steps[step].pitch = pitch;
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
