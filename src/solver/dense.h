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
// File:     dense.h
// Dev step: 1
// Date:     2026-10-01
//
// Bootstrap dense LU. This is the v0 solver so the whole pipeline is
// testable end-to-end; it will be replaced by a KLU/NICSLU-style sparse
// direct solver behind the same interface. Do not grow features here.
//----------------------------------------------------------------------
#pragma once
#include "core/types.h"
#include <vector>

namespace sym {

struct DenseMat {
    i32 n = 0;
    std::vector<f64> a; // row-major n*n

    void init(i32 n_) {
        n = n_;
        a.assign((u64)n * n, 0.0);
    }
    void zero() { std::fill(a.begin(), a.end(), 0.0); }
    f64& at(i32 r, i32 c) { return a[(u64)r * n + c]; }

    // In-place LU with partial pivoting; b in -> x out.
    // Returns false on a structurally singular matrix.
    bool lu_solve(f64* b);
};

} // namespace sym
