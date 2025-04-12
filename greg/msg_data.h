/*
 *  msg_data.h
 *  struct for representing all possible message inputs to an object
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

#pragma once

#include "ext.h"
#include "atom.h"
#include <sstream>
#include <type_traits>

typedef struct _msg_data {
    t_atom* atoms;
    size_t size;
    e_max_atomtypes type;
} t_msg_data;

inline void msg_data_init(t_msg_data* x) {
    x->atoms = nullptr;
    x->size = 0;
    x->type = A_NOTHING;
}

inline void msg_data_free(t_msg_data* x) {
    sysmem_freeptr(x->atoms);
    x->atoms = nullptr;
    x->size = 0;
    x->type = A_NOTHING;
}

inline void msg_data_resize(t_msg_data* x, size_t n) {
    x->size = n;
    sysmem_freeptr(x->atoms);
    x->atoms = (t_atom*)sysmem_newptr(n * sizeof(t_atom));
}

inline std::stringstream msg_data_to_stream(t_msg_data* x) {
    std::stringstream result;

    if(x->type == A_LONG || x->type == A_FLOAT || x->type == A_SYM) {
        assert(x->size == 1 && "x->type should only ever be (long, float, sym) if the atoms size is also one!");
        append_atom_to_stream(result, x->atoms);
    } else if(x->type == A_GIMME) {
        for(size_t i=0; i<x->size; i++) {
            append_atom_to_stream(result, x->atoms+i);
            result << " ";
        }
    }

    return result;
}

inline void msg_data_from_c_string(t_msg_data* x, const char* parsestr) {
    long size = 0;
    t_atom* atoms = nullptr;
    t_max_err result = atom_setparse(&size, &atoms, parsestr);

    if(result != MAX_ERR_NONE) {
        post("failed to parse string");
        return;
    }

    if(atoms == nullptr) {
        post("ERROR: parse failed, atoms is nullptr!");
        return;
    }

    msg_data_free(x);

    x->size = size;
    x->atoms = atoms;

    if(x->size == 0) {          // no input (can this even happen?)
        post("DEBUG - x->greg_size is 0!");
        x->type = A_NOTHING;
    } else if(x->size == 1) {   // single element in input, just get its type
        x->type = static_cast<e_max_atomtypes>(atom_gettype(x->atoms));
    } else {                    // input was a list
        x->type = A_GIMME;
    }
}

// pure C interface for setting

inline void _msg_data_set_common(t_msg_data* x, size_t n, e_max_atomtypes type) {
    msg_data_resize(x, n);
    x->type = type;
}

inline void msg_data_set_long(t_msg_data* x, t_atom_long value) {
    _msg_data_set_common(x, 1, A_LONG);
    atom_setlong(x->atoms, value);
}

inline void msg_data_set_float(t_msg_data* x, t_atom_float value) {
    _msg_data_set_common(x, 1, A_FLOAT);
    atom_setfloat(x->atoms, value);
}

inline void msg_data_set_symbol(t_msg_data* x, t_symbol* value) {
    _msg_data_set_common(x, 1, A_FLOAT);
    atom_setsym(x->atoms, value);
}

inline void msg_data_set_list(t_msg_data* x, long argc, t_atom* argv) {
    _msg_data_set_common(x, argc, A_GIMME);
    atom_setlist(x->atoms, argc, argv);
}

inline void msg_data_set_anything(t_msg_data* x, t_symbol* s, long argc, t_atom* argv) {
    _msg_data_set_common(x, argc + 1, A_GIMME);
    atom_setanything(x->atoms, s, argc, argv);
}

inline void msg_data_set_list_or_anything(t_msg_data*x, t_symbol* s, long argc, t_atom* argv) {
    if(s == gensym("list")) {
        msg_data_set_list(x, argc, argv);
    } else {
        msg_data_set_anything(x, s, argc, argv);
    }
}

inline void msg_data_outlet(t_msg_data* x, t_outlet* outlet) {
    if(x->type == A_LONG) {
        outlet_int(outlet, atom_getlong(x->atoms));
    } else if(x->type == A_FLOAT) {
        outlet_float(outlet, atom_getfloat(x->atoms));
    } else if(x->type == A_GIMME) {
        if(atom_gettype(x->atoms) == A_SYM) {
            outlet_anything(outlet, atom_getsym(x->atoms), x->size - 1, x->atoms + 1);
        } else {
            outlet_list(outlet, nullptr, x->size, x->atoms);
        }
    }
}

#ifdef __cplusplus

// C++ templates for setting

// handy dandy variadic template for setting the data pointer according
// to all of the messages we support, reducing code duplication
template <typename ... Args>
void msg_data_set(t_msg_data* x, size_t n, e_max_atomtypes type, Args ... values) {
    msg_data_resize(x, n);
    atom_set(x->atoms, values...);
    x->type = type;
}

inline void msg_data_set(t_msg_data* x, long argc, t_atom* argv) {
    msg_data_resize(x, argc);
    atom_set(x->atoms, argc, argv);
    x->type = A_GIMME;
}

inline void msg_data_set(t_msg_data* x, t_symbol* s, long argc, t_atom* argv) {
    msg_data_resize(x, argc+1);
    atom_set(x->atoms, s, argc, argv);
    x->type = A_GIMME;
}

void msg_data_set(t_msg_data* x, std::integral auto l) {
    msg_data_set(x, 1, A_LONG, l);
}

void msg_data_set(t_msg_data* x, std::floating_point auto f) {
    msg_data_set(x, 1, A_FLOAT, f);
}

inline void msg_data_set(t_msg_data* x, t_symbol* s) {
    msg_data_set(x, 1, A_SYM, s);
}

// // TODO: tinkering around w/ automatically setting the right size and type
// // at compile time as an exercise in template metaprogramming
// template <typename V> concept AtomValue =
//     std::is_integral_v<V> ||
//     std::is_floating_point_v<V> ||
//     std::is_same_v<V, t_symbol*>;
//
// template <typename V> struct AtomSize {
//     static constexpr size_t size = 0;
// };
//
// template <AtomValue V> struct AtomSize<V> {
//     static constexpr size_t size = 1;
// };
//
// template <typename ...V> struct AtomType : public AtomSize<V...> {
//     static constexpr e_max_atomtypes type = A_NOTHING;
// };
//
// // template <long size, ...V> struct AtomType : public AtomSize<> {
// //
// // };
//
// template <std::integral V> struct AtomType<V> : public AtomSize<V> {
//     static constexpr e_max_atomtypes type = A_LONG;
// };
//
// template <std::floating_point V> struct AtomType<V> : public AtomSize<V> {
//     static constexpr e_max_atomtypes type = A_FLOAT;
// };
//
// template <> struct AtomType<t_symbol*> : public AtomSize<t_symbol*> {
//     static constexpr e_max_atomtypes type = A_SYM;
// };
//
// template <AtomValue V>
// void msg_data_set(t_msg_data* x, V value) {
//     msg_data_set(x, AtomType<V>::size, AtomType<V>::type, value);
// }

#endif  // __cplusplus
