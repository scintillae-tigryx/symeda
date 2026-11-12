// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Scintillae Ltd.
// Find out more at symeda.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

//----------------------------------------------------------------------
// File:     value.cpp
// Dev step: 1
// Date:     2026-10-01
//
// Engineering-suffix numeric parsing ("1k", "2.2meg", "10n").
// Trailing junk after the suffix letter is ignored, matching SPICE
// ("1kohm" == 1e3). scs mode treats 'M' as mega (SI convention);
// classic mode is case-insensitive with m=milli, meg=mega.
//----------------------------------------------------------------------
#include "parse.h"
#include <cstdlib>
#include <cctype>
#include <string>

namespace sym {

bool parse_value(const std::string& s, f64& out, bool scs_mode) {
    if (s.empty()) {
        return false;
    }
    const char* p = s.c_str();
    char* end = nullptr;
    f64 v = std::strtod(p, &end);
    if (end == p) {
        return false;
    }
    f64 mul = 1.0;
    if (*end) {
        std::string sfx(end);
        if (sfx.rfind("meg", 0) == 0 || sfx.rfind("Meg", 0) == 0 || sfx.rfind("MEG", 0) == 0) {
            mul = 1e6;
        } else {
            char c = sfx[0];
            if (scs_mode) {
                switch (c) { // case-sensitive SI
                case 'a':
                    mul = 1e-18;
                    break;
                case 'f':
                    mul = 1e-15;
                    break;
                case 'p':
                    mul = 1e-12;
                    break;
                case 'n':
                    mul = 1e-9;
                    break;
                case 'u':
                    mul = 1e-6;
                    break;
                case 'm':
                    mul = 1e-3;
                    break;
                case 'k':
                case 'K':
                    mul = 1e3;
                    break;
                case 'M':
                    mul = 1e6;
                    break;
                case 'G':
                    mul = 1e9;
                    break;
                case 'T':
                    mul = 1e12;
                    break;
                default:
                    mul = 1.0;
                    break;
                }
            } else {
                switch (std::tolower((unsigned char)c)) {
                case 'a':
                    mul = 1e-18;
                    break;
                case 'f':
                    mul = 1e-15;
                    break;
                case 'p':
                    mul = 1e-12;
                    break;
                case 'n':
                    mul = 1e-9;
                    break;
                case 'u':
                    mul = 1e-6;
                    break;
                case 'm':
                    mul = 1e-3;
                    break;
                case 'k':
                    mul = 1e3;
                    break;
                case 'g':
                    mul = 1e9;
                    break;
                case 't':
                    mul = 1e12;
                    break;
                default:
                    mul = 1.0;
                    break;
                }
            }
        }
    }
    out = v * mul;
    return true;
}

} // namespace sym
