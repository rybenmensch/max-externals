// copyfct~ external
// calculates curves in buffers from function input
// Copyright (c) 2021 Manolo Müller

/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "curve.h"
#include "pd.h"

#include "ext.h"
#include "ext_buffer.h"
#include "ext_obex.h"
#include "z_dsp.h"

typedef struct _copyfct {
	t_object p_ob;
	t_buffer_ref *buffer_reference;
	t_buffer_obj *buffer_obj;
	t_atom_long buffer_size;
	t_bool buffer_modified;
	t_bool no_buffer;

	float x_value;
	float x_ccinput;
	float x_target;
	double x_vv;
	double x_bb;
	double x_mm;
	float x_y0;
	float x_dy;
	float x_ksr;
	int x_nleft;
	int x_retarget;
	int x_nsegs; /* as used */
	t_curveseg *x_curseg;
	t_curveseg *x_segs;
	t_curveseg x_segini[CURVE_MAXSIZE];
	t_clock *x_clock;
	t_ptr *x_bangout;

	t_symbol *buffer_name;
} t_copyfct;

t_symbol *ps_buffer_modified;
t_symbol *ps_global_binding;
t_class *copyfct_class;
t_float sr;

void *copyfct_new(t_symbol *s, long argc, t_atom *argv);
void copyfct_free(t_copyfct *x);
void copyfct_assist(t_copyfct *x, void *b, long m, long a, char *s);
void copyfct_points(t_copyfct *x, t_symbol *s, long ac, t_atom *av);

void copyfct_doset(t_copyfct *x, t_symbol *s, long ac, t_atom *av);
void copyfct_set(t_copyfct *x, t_symbol *s, long ac, t_atom *av);
void copyfct_dblclick(t_copyfct *x);
t_max_err copyfct_notify(t_copyfct *x, t_symbol *s, t_symbol *msg, void *sender, void *data);

void ext_main(void *r) {
	t_class *c;

	c = class_new("copyfct~", (method)copyfct_new, (method)copyfct_free, sizeof(t_copyfct), NULL, A_GIMME, 0);
	class_addmethod(c, (method)copyfct_assist,	"assist",	A_CANT,	 0);
	class_addmethod(c, (method)copyfct_dblclick,"dblclick",	A_CANT,  0);
	class_addmethod(c, (method)copyfct_notify,	"notify",	A_CANT,  0);
	class_addmethod(c, (method)copyfct_set,		"set",		A_GIMME, 0);
	class_addmethod(c, (method)copyfct_points,	"list", 	A_GIMME, 0);
	class_addmethod(c, (method)curve_float,		"float",	A_FLOAT, 0);

	class_dspinit(c);
	class_register(CLASS_BOX, c);
	copyfct_class = c;

	ps_buffer_modified = gensym("buffer_modified");
	ps_global_binding = gensym("globalsymbol_binding");
}

void *copyfct_new(t_symbol *s, long argc, t_atom *argv) {
	t_copyfct *x = (t_copyfct *)object_alloc(copyfct_class);
	sr = sys_getsr();

	x->buffer_modified = TRUE;
	x->buffer_size = 1;
	x->buffer_reference = NULL;
	x->no_buffer = TRUE;

	// curve setup
	t_float initval = PDCYCURVEINITVAL;
	t_float param = PDCYCURVEPARAM;
	x->x_value = x->x_target = initval;
	curve_factor(x, param);
	x->x_ksr = sr * 0.001;
	x->x_nleft = 0;
	x->x_retarget = 0;
	x->x_nsegs = 0;
	x->x_segs = x->x_segini;
	x->x_curseg = 0;

	x->x_bangout = (t_ptr*)bangout(x);
	x->x_clock = clock_new(x, (method)curve_tick);
	copyfct_set(x, s, argc, argv);

	return (x);
}

void copyfct_free(t_copyfct *x) {
	dsp_free((t_pxobject *)x);
    object_free(x->buffer_reference);
    clock_free(x->x_clock);
}

void copyfct_assist(t_copyfct *x, void *b, long m, long a, char *s) {
	if (m == ASSIST_INLET) {
		switch (a) {
			case 0:
				snprintf(s, 256, "(list) Input from function in mode 1, outputmode 1");
				break;
		}
	} else {
		switch (a) {
			case 0:
				snprintf(s, 256, "Bang on finish");
				break;
		}
	}
}

#pragma mark BUFFERS

void copyfct_doset(t_copyfct *x, t_symbol *s, long ac, t_atom *av) {
	// that's a lot of checking...
	if (x->buffer_name == gensym("")) {
		object_error((t_object *)x, "ERROR: no buffer name provided.");
		return;
	}

	if (!x->buffer_reference) {
		x->buffer_reference = buffer_ref_new((t_object *)x, x->buffer_name);
	} else {
		buffer_ref_set(x->buffer_reference, x->buffer_name);
	}

	if ((x->buffer_obj = buffer_ref_getobject(x->buffer_reference))) {
		x->no_buffer = FALSE;
		x->buffer_size = buffer_getframecount(x->buffer_obj) - 1;

		if (x->buffer_size == -1 || !x->buffer_size) {
			object_method(x->buffer_obj, gensym("sizeinsamps"), sr);
			post("Buffer had no size, set buffer length to 1000ms.");
			x->buffer_size = sr;
		}
		x->buffer_modified = FALSE;
	} else {
		object_error((t_object *)x, "Buffer %s probably doesn't exist.",
			   x->buffer_name->s_name);
		x->no_buffer = TRUE;
	}
}

void copyfct_set(t_copyfct *x, t_symbol *s, long ac, t_atom *av) {
	x->buffer_name = (av) ? atom_getsym(av) : x->buffer_name;
	defer_low(x, (method)copyfct_doset, NULL, 0, NULL);
}

void copyfct_dblclick(t_copyfct *x) { buffer_view(x->buffer_obj); }

t_max_err copyfct_notify(t_copyfct *x, t_symbol *s, t_symbol *msg, void *sender, void *data) {
	if (msg == ps_buffer_modified || msg == ps_global_binding) {
		x->buffer_modified = TRUE;
		copyfct_set(x, NULL, 0, NULL);
	}
	return buffer_ref_notify(x->buffer_reference, s, msg, sender, data);
}

#pragma mark CURVECODE

/*
   Adapted from the pd-cyclone-project by Manolo Müller.
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


void copyfct_points(t_copyfct *x, t_symbol *s, long ac, t_atom *av) {
	int natoms, nsegs, odd;
	t_atom *ap;
	t_curveseg *segp;
	for (natoms = 0, ap = av; natoms < ac; natoms++, ap++) {
		if (ap->a_type != A_FLOAT) {
			object_error((t_object *)x, "list needs to only contain floats");
			return;
		}
	}

	// do nothing if amount is 1?
	if (!natoms || natoms == 3) {
		return;
	}
	odd = natoms % 3;
	nsegs = natoms / 3;
	if (odd) {
		nsegs++;
	}

	// clip at maxsize
	if (nsegs > CURVE_MAXSIZE) {
		nsegs = CURVE_MAXSIZE;
		odd = 0;
	};

	x->x_nsegs = nsegs;
	segp = x->x_segs;
	if (odd) {
		nsegs--;
	}

	t_float total_length = 0;
	t_atom *avp = av;

	// get total length of function
	while (nsegs--) {
		avp++;
		total_length += avp++->a_w.w_float;
		avp++;
	}

	nsegs = x->x_nsegs;
	t_float buffer_size_ms = (t_float)x->buffer_size * 1000 / sr;
	t_float scale_factor = buffer_size_ms / total_length;

	while (nsegs--) {
		segp->s_target = av++->a_w.w_float;
		// scale time to 0-1
		segp->s_delta = av++->a_w.w_float * scale_factor;
		curve_cc(x, segp, av++->a_w.w_float);
		segp++;
	}

	if (odd) {
		segp->s_target = av->a_w.w_float;
		if (odd > 1) {
			segp->s_delta = av[1].a_w.w_float * scale_factor;
		} else {
			segp->s_delta = 0;
		}
		total_length += segp->s_delta;
		curve_cc(x, segp, x->x_ccinput);
	}

	x->x_target = x->x_segs->s_target;
	x->x_curseg = x->x_segs;
	x->x_retarget = 1;
	curve_perform(x);
}

void curve_perform(t_copyfct *x) {
	if (x->buffer_modified) {
		copyfct_doset(x, NULL, 0, NULL);
	}

	if (x->no_buffer) {
		object_error((t_object *)x, "No buffer yet!");
		return;
	}
	t_float *out = buffer_locksamples(x->buffer_obj);
	if (!out) {
		object_error((t_object *)x, "Couldn't lock any samples.");
		return;
	}

	int nblock = (int)x->buffer_size;
	int nxfer = x->x_nleft;
	float curval = x->x_value;
	double vv = x->x_vv;
	double bb = x->x_bb;
	double mm = x->x_mm;
	float dy = x->x_dy;
	float y0 = x->x_y0;

	if (PD_BIGORSMALL(curval)) {
		curval = x->x_value = 0;
	}

retarget:
	if (x->x_retarget) {
		float target = x->x_curseg->s_target;
        // compiler complains about this even though delta is used later on
		float delta = x->x_curseg->s_delta;
		int nhops = x->x_curseg->s_nhops;
		bb = x->x_curseg->s_bb;
		mm = x->x_curseg->s_mm;
		if (x->x_curseg->s_ccinput < 0) {
			dy = x->x_value - target;
		} else {
			dy = target - x->x_value;
		}
		x->x_nsegs--;
		x->x_curseg++;

		while (nhops <= 0) {
			curval = x->x_value = target;
			if (x->x_nsegs) {
				target = x->x_curseg->s_target;
				delta = x->x_curseg->s_delta;
				nhops = x->x_curseg->s_nhops;
				bb = x->x_curseg->s_bb;
				mm = x->x_curseg->s_mm;
				if (x->x_curseg->s_ccinput < 0) {
					dy = x->x_value - target;
				} else {
					dy = target - x->x_value;
				}
				x->x_nsegs--;
				x->x_curseg++;
			} else {
				while (nblock--) {
					*out++ = curval;
				}
				x->x_nleft = 0;
				clock_delay(x->x_clock, 0);
				x->x_retarget = 0;
			}
		}
		nxfer = x->x_nleft = nhops;
		x->x_vv = vv = bb;
		x->x_bb = bb;
		x->x_mm = mm;
		x->x_dy = dy;
		x->x_y0 = y0 = x->x_value;
		x->x_target = target;
		x->x_retarget = 0;
	}

	if (nxfer >= nblock) {
		int silly = ((x->x_nleft -= nblock) == 0); /* LATER rethink */
		while (nblock--) {
			*out++ = curval = (vv - bb) * dy + y0;
			vv *= mm;
		}
		if (silly) {
			if (x->x_nsegs) {
				x->x_retarget = 1;

			} else {
				clock_delay(x->x_clock, 0);
			}
			x->x_value = x->x_target;
		} else {
			x->x_value = curval;
			x->x_vv = vv;
		}
	} else if (nxfer > 0) {
		nblock -= nxfer;
		do {
			*out++ = (vv - bb) * dy + y0;
			vv *= mm;
		} while (--nxfer);
		curval = x->x_value = x->x_target;
		if (x->x_nsegs) {
			x->x_retarget = 1;
			goto retarget;
		} else {
			while (nblock--) {
				*out++ = curval;
			}
			x->x_nleft = 0;
			clock_delay(x->x_clock, 0);
		}
	} else {
		while (nblock--) {
			*out++ = curval;
		}
	}

	buffer_unlocksamples(x->buffer_obj);
	buffer_setdirty(x->buffer_obj);
	outlet_bang(x->x_bangout);
}

void curve_factor(t_copyfct *x, t_float f) {
	if (f < -1.) {
		x->x_ccinput = -1.;
	} else if (f > 1.) {
		x->x_ccinput = 1.;
	} else {
		x->x_ccinput = f;
	}
}

void curve_coefs(int nhops, double crv, double *bbp, double *mmp) {
	if (nhops > 0) {
		double hh, ff, eff, gh;
		if (crv < 0) {
			if (crv < -1.)
				crv = -1.;
			hh = pow(((CURVE_C1 - crv) * CURVE_C2), CURVE_C3) * CURVE_C4;
			ff = hh / (1. - hh);
			eff = exp(ff) - 1.;
			gh = (exp(ff * .5) - 1.) / eff;
			*bbp = gh * (gh / (1. - (gh + gh)));
			*mmp = 1. / (((exp(ff * (1. / (double)nhops)) - 1.) / (eff * *bbp)) + 1.);
			*bbp += 1.;
		} else {
			if (crv > 1.) {
				crv = 1.;
			}
			hh = pow(((crv + CURVE_C1) * CURVE_C2), CURVE_C3) * CURVE_C4;
			ff = hh / (1. - hh);
			eff = exp(ff) - 1.;
			gh = (exp(ff * .5) - 1.) / eff;
			*bbp = gh * (gh / (1. - (gh + gh)));
			*mmp = ((exp(ff * (1. / (double)nhops)) - 1.) / (eff * *bbp)) + 1.;
		}
	} else if (crv < 0) {
		*bbp = 2.;
		*mmp = 1.;
	} else {
		*bbp = *mmp = 1.;
	}
}

void curve_cc(t_copyfct *x, t_curveseg *segp, float f) {
	int nhops = segp->s_delta * x->x_ksr + 0.5;
	segp->s_ccinput = f;
	segp->s_nhops = (nhops > 0 ? nhops : 0);
	curve_coefs(segp->s_nhops, (double)f, &segp->s_bb, &segp->s_mm);
}

void curve_tick(t_copyfct *x) {
    return;
}

void curve_float(t_copyfct *x, t_float f) {
	object_error(
		(t_object *)x,
		"Invalid input. Change outputmode of function to 1 (list output)");
	return;
}

