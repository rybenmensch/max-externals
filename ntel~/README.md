# ntel~

Divide a phasor signal into n total steps. This creates n subdivisions of the
phasor. The users can then decide on which subdivision \[ntel~\] will send out
a bang on a per-step outlet. For ease of use, the subdivisions are written with
integers and converted internally to fractions. The downside is that only
integer divisions of the range are allowed.

## Example

\[ntel~ 3 0 1 2\] (shorthand: \[ntel~ 3\]) divides the phasor into three
subdivisions (0, 0.333..., 0.666...) and creates three bang outlets. The first
will bang when the input signal is 0, the second when it is 0.333..., the third
when it is 0.666...

## Arguments

The first argument (the divider) indicates into how many steps the phasor
signal should be divided. If not supplied or smaller than 1, it will default to
1, generating one step that bangs on 0.

The rest of the arguments are the divisions on which \[ntel\] will bang. If no
steps are supplied, the object generates n = divider steps from 0 to (divider -
1). Steps are clamped to the range 0 to (divider - 1).

It is possible to have multiple outlets triggering on the same step. E.g.,
\[ntel~ 2 1 1 1\] is valid, and will create three outputs that all bang on 0.5;

The usage of the arguments can be queried with the message `usage`.

## Caveat

Any self-respecting phasor generates a signal in the right-open interval \[0,
1). \[ntel~\] also assumes this condition.

## Name

The name comes from the German language construction for expressing fractions,
e.g. "ein Zweitel, ein Drittel, ein Viertel". Soothingly regular in comparison
with the English equivalents ("one half, one third, one fourth"). "Ein Ntel"
means "one nth".

## TODO

- Allow specifying steps with non-integer divisions
- Allow specifying steps as floats to pick an exact time-point in the input
  range of the phasor

## License

ntel~

Copyright (C) 2023-2025 Manolo Müller

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version. This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details. You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.

