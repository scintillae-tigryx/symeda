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
// File:     tran.h
// Dev step: 1
// Date:     2026-10-01
//
// Transient engine: MNA stamping + integration + solve per step.
// v0: linear elements (R C L V I) only, backward Euler, fixed step.
// Newton iteration, LTE step control and nonlinear device models plug
// into run_tran's step loop without changing its interface.
//----------------------------------------------------------------------
#pragma once
#include "ckt/netlist.h"
#include <string>
#include <vector>
#include <cstdio>

namespace sym {

struct Opts {
    std::string mode = "spice"; // "spice" (accurate) | "fast" (planned)
    int threads = 0;            // 0 = hardware default
    bool gpu = false;           // GPU backend requested (planned)
    f64 gmin = 1e-12;           // min conductance to ground (spice gmin)
};

struct TranResult {
    std::vector<f64> t;
    std::vector<std::vector<f64>> y; // y[col][step]
    std::vector<std::string> names;  // column names: v(n1) ... i(v1)
    i32 steps = 0;
};

// Returns 0 on success; prints diagnostics to `log`.
int run_tran(const Circuit& c, const Opts& o, TranResult& r, FILE* log);

// Public so unit tests and the fast-mode table builder can reuse it.
f64 src_value(const Source& s, f64 t);

} // namespace sym
