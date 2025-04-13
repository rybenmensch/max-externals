/*
 *  greg.cpp
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

/*
 * some of this code was adapted from the bach project, specifically from
 *
 *  reg.c
 *
 * Copyright (C) 2010-2022 Andrea Agostini and Daniele Ghisi
 * bach is distributed under the terms of the GNU General Public License version 3 (GPL-v3.0).
 *
 * The source code of bach can be obtained at the following URL:
 * https://github.com/bachfamily/bach
 *
 */

#include "atom.h"
#include "msg_data.h"
#include "ext.h"
#include <cassert>
#include <sstream>

typedef struct _greg {
    t_object obj;
    t_outlet* outlet;
    void* proxy;
    long in;
    t_object* editor;
    t_msg_data data;
    bool dirty;
} t_greg;

void* greg_new(t_symbol* s, long argc, t_atom* argv);
void greg_free(t_greg* x);
void greg_int(t_greg* x, long l);
void greg_float(t_greg* x, double f);
void greg_gimme(t_greg* x, t_symbol* s, long argc, t_atom* argv);
void greg_bang(t_greg* x);
void greg_dblclick(t_greg* x);
void greg_edclose(t_greg* x, char* *ht, long size);
void greg_assist(t_greg* x, void* b, long m, long a, char* s);
void greg_okclose(t_greg* x, char* s, short* result);

static t_class* s_greg_class;

void ext_main(void* r) {
    t_class* c = class_new("greg", (method)greg_new, (method)greg_free, sizeof(t_greg), NULL, A_GIMME, 0);
    class_addmethod(c, (method)greg_int,        "int",          A_LONG,     0);
    class_addmethod(c, (method)greg_float,      "float",        A_FLOAT,    0);
    class_addmethod(c, (method)greg_gimme,      "anything",     A_GIMME,    0);
    class_addmethod(c, (method)greg_gimme,      "list",         A_GIMME,    0);
    class_addmethod(c, (method)greg_bang,       "bang",         A_CANT,     0);
    class_addmethod(c, (method)greg_dblclick,   "dblclick",     A_CANT,     0);
    class_addmethod(c, (method)greg_edclose,    "edclose",      A_CANT,     0);
    class_addmethod(c, (method)greg_okclose,    "okclose",      A_CANT,     0);
    class_addmethod(c, (method)greg_assist,     "assist",       A_CANT,     0);
    class_addmethod(c, (method)stdinletinfo,    "inletinfo",    A_CANT,     0);
    class_register(CLASS_BOX, c);
    s_greg_class = c;
}

void* greg_new(t_symbol* s, long argc, t_atom* argv) {
    t_greg* x = (t_greg*)object_alloc(s_greg_class);
    x->proxy = proxy_new((t_object*) x, 1, &x->in);
    x->outlet = outlet_new(x, nullptr);
    x->editor = nullptr;
    msg_data_init(&x->data);
    x->dirty = false;

    return x;
}

void greg_free(t_greg* x) {
    sysmem_freeptr(x->proxy);
    msg_data_free(&x->data);
}

void greg_int(t_greg* x, long l) {
    // msg_data_set(&x->data, l);
    msg_data_set_long(&x->data, l);
    greg_bang(x);
}

void greg_float(t_greg* x, double f) {
    // msg_data_set(&x->data, f);
    msg_data_set_float(&x->data, f);
    greg_bang(x);
}

/*
 * handles list and anything
 *
 * list:
 * if the first atom of a message is NOT a symbol AND `argc` > 1
 * aything:
 *
 * if the first atom of a message is a symbol, no matter the `argc`
 * the first atom of the message is stored in `s`, not in `argv`!
 *
 */
void greg_gimme(t_greg*x, t_symbol* s, long argc, t_atom* argv) {
    msg_data_set_list_or_anything(&x->data, s, argc, argv);
    greg_bang(x);
}

void greg_bang(t_greg* x) {
    // ignore bangs sent to the right inlet
    if(proxy_getinlet((t_object*)x) != 0) {
        return;
    }

    msg_data_outlet(&x->data, x->outlet);
}

void greg_dblclick(t_greg* x) {
    if(x->editor) { // bring editor to the front if it already exists
        object_attr_setchar(x->editor, gensym("visible"), 1);
    } else {        // create a new editor if it doesn't exist already
        x->editor = (t_object*)object_new(CLASS_NOBOX, gensym("jed"), (t_object*)x, 0);
    }

    if(!x->data.size) {
        return;
    }

    std::stringstream stream = msg_data_to_stream(&x->data);
    object_method(x->editor, gensym("settext"), stream.str().c_str(), gensym("utf-8"));
}

// adapted from bach
void greg_edclose(t_greg* x, char** ht, long size) {
    x->editor = nullptr;
    if(!x->dirty) {
        return;
    }

    msg_data_from_c_string(&x->data, *ht);
    x->dirty = false;
}

void greg_assist(t_greg* x, void* b, long m, long a, char* s) {
    if(m==ASSIST_INLET) {
        switch (a) {
            case 0:
                snprintf_zero(s, 256, "(anything) list to store and output immediately");
                break;
            case 1:
                snprintf_zero(s, 256, "(anything) list to store");
                break;
        }
    } else {
        snprintf_zero(s, 256, "(anything) stored list");
    }
}

// directly adapted from bach.reg
// stop "dirty" editor window from prompting the user to save the file
void greg_okclose(t_greg* x, char* s, short* result) {
    *result = 3;
    x->dirty = true;
}

