/*
 *  ntel~.c
 *
 * Copyright (C) 2023-2025 Manolo Müller
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License
 * as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"

typedef struct _ntel {
    t_pxobject w_obj;
    long divider;
    long current_step;
    long* steps;
    long n_steps;
    void* clock;
    void** outlets;
    void* current_outlet;
    void** tick_outlets;
    t_sample history;
} t_ntel;


void* ntel_new(t_symbol* s, long argc, t_atom* argv);
void ntel_free(t_ntel* x);
void ntel_perform64(t_ntel* x, t_object* dsp64, double** ins, long numins, double** outs, long numouts, long sampleframes, long flags, void* userparam);
void ntel_dsp64(t_ntel* x, t_object* dsp64, short* count, double samplerate, long maxvectorsize, long flags);
void tick(t_ntel* x);
void ntel_usage(t_ntel* x);
void ntel_assist(t_ntel* x, void* b, long m, long a, char* s);

static t_class* s_ntel_class;

void ext_main(void* r) {
    t_class* c = class_new("ntel~", (method)ntel_new, (method)ntel_free, sizeof(t_ntel), NULL, A_GIMME, 0);
    class_addmethod(c, (method)ntel_dsp64,  "dsp64",  A_CANT, 0);
    class_addmethod(c, (method)ntel_usage,  "usage",          0);
    class_addmethod(c, (method)ntel_assist, "assist", A_CANT, 0);
    class_dspinit(c);
    class_register(CLASS_BOX, c);
    s_ntel_class = c;
}

void* ntel_new(t_symbol* s, long argc, t_atom* argv) {
    t_ntel* x = (t_ntel*)object_alloc(s_ntel_class);

    dsp_setup((t_pxobject*)x, 1); // one inlet for the phasor

    if(!argc) {
        object_post((t_object*)x, "no divider given, defaulting to 1");
        x->divider = 1;
    } else {
        x->divider = atom_getlong(argv);
        // ensure divider of at least 1
        // since atom_getlong returns zero on parse failure, we also just handle
        // invalid input for simplicity's sake
        if(x->divider < 1) {
            object_post((t_object*)x, "divider must be at least 1, defaulting to 1");
            x->divider = 1;
        }
    }

    // count how many steps we have
    long n_steps = argc - 1;
    t_bool no_steps = n_steps < 1;
    if(no_steps) {
        // not actually posting the suggestion, as this is a feature now
        // object_post((t_object*)x, "should have at least one step, defaulting to all steps between 0 and %ld", x->divider - 1);
        x->n_steps = x->divider;
    } else { // success
        x->n_steps = n_steps;
    }

    x->outlets      = (void**)sysmem_newptr(x->n_steps * sizeof(void*));
    x->tick_outlets = (void**)sysmem_newptr(x->n_steps * sizeof(void*));
    x->steps        = (long*) sysmem_newptr(x->n_steps * sizeof(long));

    for(int i=0; i<x->n_steps; i++) {
        if(no_steps) {
            x->steps[i] = i;
        } else {
            // check if user has supplied valid step numbers, clamp if outside
            // TODO: might be more useful to do modulo
            long step = atom_getlong(argv + 1 + i);
            long highest_step = x->divider - 1;
            if(step < 0 || step > highest_step) {
                object_post((t_object*)x, "step nr %ld with value %ld clamped to range 0 - %ld", i+1, step, highest_step);
                step = CLAMP(step, 0, highest_step);
            }
            x->steps[i] = step;
        }

        // outlets are created from right to left, we reverse the order
        // so that they line up with the order of the arguments
        int rev_i = x->n_steps - i - 1;
        x->outlets[rev_i] = bangout(x);

        x->tick_outlets[i] = NULL;

    }
    assert(x->n_steps > 0);
    x->current_outlet = x->outlets[0];

    x->clock = clock_new(x, (method)tick);

    x->current_step = 0;
    x->history = 0;

    return x;
}

void ntel_free(t_ntel* x) {
    dsp_free((t_pxobject*)x);
    freeobject(x->clock);
    sysmem_freeptr(x->steps);
    sysmem_freeptr(x->outlets);
    sysmem_freeptr(x->tick_outlets);
}

void ntel_perform64(t_ntel* x, t_object* dsp64, double** ins, long numins, double** outs, long numouts, long sampleframes, long flags, void* userparam) {
    t_sample* in = ins[0];
    long n = sampleframes;
    while(n--) {
        t_sample phase = *in++;

        // scale frequency of input phase by `divider`
        // this will make a phasor at 1Hz go `divider` times faster
        phase *= x->divider;
        phase = fmod(fabs(phase), 1);

        t_sample delta = phase - x->history;
        if(delta < 0) {
            // check if one scaled clock cycle has passed (falling edge)
            for(int i=0; i<x->n_steps; i++) {
                // search for all steps that match current_step
                if(x->steps[i] == x->current_step) {
                    x->tick_outlets[i] = x->outlets[i];
                } else {
                    x->tick_outlets[i] = NULL;
                }
            }
            clock_delay(x->clock, 0);

            //increase counter
            x->current_step = (x->current_step+1) % x->divider;
        } else if(delta == 0) {
            // if the phasor has been turned off, reset the step sequence
            x->current_step = 0;
            clock_unset(x->clock);
        }

        x->history = phase;
    }
}

void ntel_dsp64(t_ntel* x, t_object* dsp64, short* count, double samplerate, long maxvectorsize, long flags) {
    object_method(dsp64, gensym("dsp_add64"), x, ntel_perform64, 0, NULL);
}

void tick(t_ntel* x) {
    for(int i=0; i<x->n_steps; i++) {
        if(x->tick_outlets[i]) {
            outlet_bang(x->tick_outlets[i]);
        }
    }
}

void ntel_usage(t_ntel* x) {
    object_post((t_object*)x, "ntel~ usage:");
    object_post((t_object*)x, "arg 1: (int) into how many steps to divide the phasor (at least 1)");
    object_post((t_object*)x, "args 2...n: (list) on which steps to trigger bang (count starts at 0)");
    object_post((t_object*)x, "if args 2...n is empty, steps 0 .. (value of arg 1) - 1 are automatically generated");
    object_post((t_object*)x, "arguments in args 2...n cannot be larger than (value of arg 1) - 1");
}

void ntel_assist(t_ntel* x, void* b, long m, long a, char* s) {
    if (m == ASSIST_INLET) {
        snprintf_zero(s, 256, "(signal) Phasor input (0 - 1)");
    } else if(m == ASSIST_OUTLET) {
        if(a >= x->n_steps || x->steps == NULL) {
            return;
        }
        snprintf_zero(s, 256, "Bang on step %d", x->steps[a]);
    }
}

