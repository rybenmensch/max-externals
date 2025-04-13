/*
 *
 * mc.rotate~ -- example of MC auto-adapting
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
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Adaptation that adds int input 
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
#include "z_dsp.h"

typedef struct _mcrotate {
    t_pxobject m_obj;
    long numchans;
    int rot;
} t_mcrotate;


void* mcrotate_new(void);
void mcrotate_free(t_mcrotate* x);
void mcrotate_int(t_mcrotate* x, long a);
long mcrotate_multichanneloutputs(t_mcrotate* x, long index);
long mcrotate_inputchanged(t_mcrotate* x, long index, long count);
void mcrotate_perform64(t_mcrotate* x, t_object* dsp64, double** ins, long numins, double** outs, long numouts, long sampleframes, long flags, void* userparam);
void mcrotate_dsp64(t_mcrotate* x, t_object* dsp64, short* count, double samplerate, long maxvectorsize, long flags);
void mcrotate_assist(t_mcrotate* x, void* b, long m, long a, char* s);

static t_class* s_mcrotate_class;

void ext_main(void* r) {
    t_class* c = class_new("mc.rotate~", (method)mcrotate_new, (method)mcrotate_free, sizeof(t_mcrotate), 0L, 0);

    class_addmethod(c, (method)mcrotate_int,                 "int",                 A_LONG, 0);
    class_addmethod(c, (method)mcrotate_multichanneloutputs, "multichanneloutputs", A_CANT, 0);
    class_addmethod(c, (method)mcrotate_inputchanged,        "inputchanged",        A_CANT, 0);
    class_addmethod(c, (method)mcrotate_dsp64,               "dsp64",                A_CANT, 0);
    class_addmethod(c, (method)mcrotate_assist,              "assist",                A_CANT, 0);

    class_dspinit(c);

    s_mcrotate_class = c;
    class_register(CLASS_BOX, s_mcrotate_class);
}

void* mcrotate_new(void) {
    t_mcrotate* x = (t_mcrotate*)object_alloc(s_mcrotate_class);

    dsp_setup((t_pxobject*)x, 1);
    x->m_obj.z_misc |= Z_NO_INPLACE | Z_MC_INLETS;
    outlet_new((t_object*)x, "multichannelsignal");

    x->numchans = 1;
    x->rot = 0;

    return x;
}

void mcrotate_free(t_mcrotate* x) {
    dsp_free((t_pxobject*)x);
}

void mcrotate_int(t_mcrotate* x, long a) {
    a = labs(a);
    x->rot = a % x->numchans;
}

long mcrotate_multichanneloutputs(t_mcrotate* x, long index) {
    return x->numchans;
}

long mcrotate_inputchanged(t_mcrotate* x, long index, long count) {
    if(count != x->numchans) {
        x->numchans = CLAMP(count, 1, MC_MAX_CHANS);
        return true;
    }
    return false;
}

void mcrotate_perform64(t_mcrotate* x, t_object* dsp64, double** ins, long numins, double** outs, long numouts, long sampleframes, long flags, void* userparam) {
    long numchans = MIN(numins, numouts);
    for(long i=0; i<numchans; i++) {
        int chan_idx = (i + x->rot) % x->numchans;
        double* in = ins[i];
        double* out = outs[chan_idx];
        sysmem_copyptr(in, out, sampleframes * sizeof(double));
    }
}

void mcrotate_dsp64(t_mcrotate* x, t_object* dsp64, short* count, double samplerate, long maxvectorsize, long flags) {
    dsp_add64(dsp64, (t_object*)x, (t_perfroutine64)mcrotate_perform64, 0, NULL);
}

void mcrotate_assist(t_mcrotate* x, void* b, long m, long a, char* s) {
    if(m == 1) {
        strcpy(s, "(multi-channel signal) Input");
    } else if (m == 2) {
        sprintf(s, "(multi-channel signal) Input, rotated");
    }
}

