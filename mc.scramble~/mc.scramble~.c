/*
 * mc.scramble~ - randomly redistribute multichannel outputs
 * Copyright (C) 2024-2025 Manolo Müller
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
 */

#include "ext.h"
#include "ext_obex.h"
#include "ext_sysmem.h"
#include "z_dsp.h"
#include <stdbool.h>

enum mcscramble_state { LINEAR=0, SCRAMBLED};

typedef struct _mcscramble {
    t_pxobject m_obj;
    long numchans;
    int* index_map;
    enum mcscramble_state state;
} t_mcscramble;


void* mcscramble_new(void);
void mcscramble_free(t_mcscramble* x);
void mcscramble_reset(t_mcscramble* x);
void mcscramble_bang(t_mcscramble* x);
long mcscramble_multichanneloutputs(t_mcscramble* x, long index);
long mcscramble_inputchanged(t_mcscramble* x, long index, long count);
void mcscramble_perform(t_mcscramble* x, t_object* dsp64, double** ins, long numins, double** outs, long numouts, long sampleframes, long flags, void* userparam);
void mcscramble_dsp64(t_mcscramble* x, t_object* dsp64, short* count, double samplerate, long maxvectorsize, long flags);
void mcscramble_assist(t_mcscramble* x, void* b, long m, long a, char* s);

static t_class* s_mcscramble_class;

void ext_main(void* r)
{
    t_class* c = class_new("mc.scramble~", (method)mcscramble_new, (method)mcscramble_free, sizeof(t_mcscramble), 0L, 0);

    class_addmethod(c, (method)mcscramble_bang,                "bang",                        0);
    class_addmethod(c, (method)mcscramble_reset,               "reset",                       0);
    class_addmethod(c, (method)mcscramble_multichanneloutputs, "multichanneloutputs", A_CANT, 0);
    class_addmethod(c, (method)mcscramble_inputchanged,        "inputchanged",        A_CANT, 0);
    class_addmethod(c, (method)mcscramble_dsp64,               "dsp64",                  A_CANT, 0);
    class_addmethod(c, (method)mcscramble_assist,              "assist",              A_CANT, 0);

    class_dspinit(c);

    s_mcscramble_class = c;
    class_register(CLASS_BOX, s_mcscramble_class);
}

void* mcscramble_new(void) {
    t_mcscramble* x = (t_mcscramble*)object_alloc(s_mcscramble_class);

    dsp_setup((t_pxobject*)x, 1);
    x->m_obj.z_misc |= Z_NO_INPLACE | Z_MC_INLETS;
    outlet_new((t_object*)x, "multichannelsignal");

    x->numchans = 1;
    x->index_map = (int*)sysmem_newptrclear(x->numchans * sizeof(int));
    mcscramble_reset(x);

    return x;
}

void mcscramble_free(t_mcscramble* x) {
    dsp_free((t_pxobject*)x);
    sysmem_freeptr(x->index_map);
}

/*
 * randlong and mcscramble_bang adapted from:
 * urner - a max object shell
 * jeremy bernstein - jeremy@bootsquad.com
 * Copyright (c) 2015, Cycling '74.
 * All rights reserved.
 * 
 * The software to which this license pertains is the Max SDK that consists of the
 * C language header files and source code examples contained within this archive.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 */

long randlong(long max) {
    long rand;
#ifdef WIN_VERSION
    rand_s(&rand);
#else
    rand = random();
#endif
    return rand % max;
}

// shuffles index map
void mcscramble_bang(t_mcscramble* x) {
    x->state = SCRAMBLED;
    bool* table = (bool*)sysmem_newptrclear(x->numchans * sizeof(bool));
    for(int i=0; i<x->numchans; i++) {
        long rand = randlong(x->numchans);
        if(table[rand] != false) { // NUMBER HAS ALREADY BEEN CHOSEN
            do {
                rand = randlong(x->numchans);
            } while(table[rand] != false);
        }
        table[rand] = true; // MARK THIS VALUE AS USED

        x->index_map[i] = rand;
    }

    sysmem_freeptr(table);
}

// sets index map to be linear again
void mcscramble_reset(t_mcscramble* x) {
    x->state = LINEAR;
    for(int i=0; i<x->numchans; i++) {
        x->index_map[i] = i;
    }
}

long mcscramble_multichanneloutputs(t_mcscramble* x, long index) {
    return x->numchans;
}

long mcscramble_inputchanged(t_mcscramble* x, long index, long count) {
    if(count != x->numchans) {
        x->numchans = CLAMP(count, 1, MC_MAX_CHANS);

        sysmem_freeptr(x->index_map);
        x->index_map = (int*)sysmem_newptrclear(x->numchans * sizeof(int));

        if(x->state == LINEAR) {
            mcscramble_reset(x);
        } else {
            mcscramble_bang(x);
        }

        return true;
    }

    return false;
}

void mcscramble_perform(t_mcscramble* x, t_object* dsp64, double** ins, long numins, double** outs, long numouts, long sampleframes, long flags, void* userparam) {
    long numchans = MIN(numins, numouts);
    for(int i=0; i<numchans; i++) {
        int chan = x->index_map[i];
        double* in = ins[i];
        double* out = outs[chan];
        sysmem_copyptr(in, out, sizeof(double) * sampleframes);
    }
}

void mcscramble_dsp64(t_mcscramble* x, t_object* dsp64, short* count, double samplerate, long maxvectorsize, long flags) {
    dsp_add64(dsp64, (t_object*)x, (t_perfroutine64)mcscramble_perform, 0, NULL);
}

void mcscramble_assist(t_mcscramble* x, void* b, long m, long a, char* s) {
    if(m == 1) {
        strcpy(s, "(multi-channel signal) Input");
    } else if(m == 2) {
        sprintf(s, "(multi-channel signal) Input, rotated");
    }
}

