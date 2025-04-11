#pragma once

#include "z_sampletype.h"
typedef struct _copyfct t_copyfct;

/*
   Adapted by Manolo Müller from the pd-cyclone project.
   https://github.com/porres/pd-cyclone
   Source: curve.c
*/

/*
 * Copyright (c) <2003-2020>, <Krzysztof Czaja, Fred Jan Kraan, Alexandre Porres, Derek Kwan, Matt Barber and others>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// definitions for curve-coefs
#define CURVE_C1   1e-20
#define CURVE_C2   1.2
#define CURVE_C3   0.41
#define CURVE_C4   0.91

#define PDCYCURVEINITVAL 0.
#define PDCYCURVEPARAM 0.
#define CURVE_MAXSIZE   256

typedef struct _curveseg {
    float   s_target;
    float   s_delta;
    int     s_nhops;
    float   s_ccinput;
    double  s_bb;
    double  s_mm;
} t_curveseg;

void curve_perform(t_copyfct *x);

void curve_factor(t_copyfct *x, float f);
void curve_coefs(int nhops, double crv, double *bbp, double *mmp);
void curve_cc(t_copyfct *x, t_curveseg *segp, float f);
void curve_tick(t_copyfct *x);
void curve_float(t_copyfct *x, t_float f);
