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
// File:     netlist.h
// Dev step: 1
// Date:     2026-10-01
//
// Circuit IR shared by the netlist front ends.
// Parsing is not the hot path, so std containers are fine here; the
// engine converts this into SoA flat arrays at elaboration time.
//----------------------------------------------------------------------
#pragma once
#include "core/types.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace sym {

enum class ElemKind : u8 { R, C, L, V, I, D, M, Q, X };
enum class SrcKind : u8 { DC, PULSE, SIN, PWL, EXP };

struct Source {
    SrcKind kind = SrcKind::DC;
    f64 dc = 0.0;
    f64 p[8] = {};
    // PULSE: v1 v2 td tr tf pw per
    // SIN:   vo va freq td theta
    // EXP:   v1 v2 td1 tau1 td2 tau2
    std::vector<f64> pwl; // t,v pairs
};

struct Element {
    ElemKind kind = ElemKind::R;
    std::string name;
    i32 nodes[4] = {};
    i32 nnodes = 0;
    f64 value = 0.0;   // R/C/L primitive value
    Source src;        // V/I waveform
    std::string model; // D/M/Q/X model or subckt name
};

struct Circuit {
    std::unordered_map<std::string, i32> ids;
    std::vector<std::string> names; // names[0] is always "0" (ground)
    std::vector<Element> elems;

    // analysis
    bool tran = false;
    f64 tstep = 0.0, tstop = 0.0, tstart = 0.0;

    std::vector<std::string> warns;

    Circuit() {
        ids["0"] = 0;
        names.push_back("0");
    }

    // Intern a node name. Ground aliases collapse to id 0.
    i32 node(const std::string& raw);
    i32 nnodes() const { return (i32)names.size(); }
};

} // namespace sym
