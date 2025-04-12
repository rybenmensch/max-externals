/*
 *  atom.h
 *  some additions for manipulating max atoms
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

// filling some gaps that cycling left

// assume `a` holds enough memory
inline void atom_setlist(t_atom* a, long argc, t_atom* argv) {
    atom_setatom_array(argc, a, argc, argv);
}

// assume `a` holds enough memory
inline void atom_setanything(t_atom* a, t_symbol* s, long argc, t_atom* argv) {
    atom_setsym(a, s);
    atom_setlist(a + 1, argc, argv);
}


#ifdef __cplusplus
#include <sstream>
// templates for setting

// all ints
void atom_set(t_atom* a, std::integral auto value) {
    atom_setlong(a, static_cast<long>(value));
}

// all floats
void atom_set(t_atom* a, std::floating_point auto value) {
    atom_setfloat(a, static_cast<double>(value)); 
}

// symbols
inline void atom_set(t_atom* a, t_symbol* value) {
    atom_setsym(a, value); 
}

// list
inline void atom_set(t_atom* a, long argc, t_atom* argv) {
    atom_setlist(a, argc, argv);
}

// anything
inline void atom_set(t_atom* a, t_symbol* s, long argc, t_atom* argv) {
    atom_setanything(a, s, argc, argv);
}

inline void append_atom_to_stream(std::stringstream& stream, t_atom* a) {
    switch(atom_gettype(a)) {
        case A_LONG:
            stream << atom_getlong(a);
            break;
        case A_FLOAT:
            stream << atom_getfloat(a);
            break;
        case A_SYM:
            stream << atom_getsym(a)->s_name;
            break;
        default:
            return;
    }
}


#endif  // __cplusplus

