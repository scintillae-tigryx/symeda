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
// File:     dense.cpp
// Dev step: 1
// Date:     2026-10-01
//
// Bootstrap dense LU. This is the v0 solver so the whole pipeline is
// testable end-to-end; it will be replaced by a KLU/NICSLU-style sparse
// direct solver behind the same interface. Do not grow features here.
//----------------------------------------------------------------------
#include "solver/dense.h"
#include <cmath>
#include <algorithm>

namespace sym {

bool DenseMat::lu_solve(f64* b) {
    const i64 n = this->n;
    for (i64 k = 0; k < n; k++) {
        i64 piv = k;
        f64 best = std::fabs(a[(u64)k * n + k]);
        for (i64 i = k + 1; i < n; i++) {
            f64 v = std::fabs(a[(u64)i * n + k]);
            if (v > best) {
                best = v;
                piv = i;
            }
        }
        if (best == 0.0) {
            return false;
        }
        if (piv != k) {
            f64* rk = &a[(u64)k * n];
            f64* rp = &a[(u64)piv * n];
            for (i64 j = 0; j < n; j++) {
                std::swap(rk[j], rp[j]);
            }
            std::swap(b[k], b[piv]);
        }
        const f64* rk = &a[(u64)k * n];
        f64 inv = 1.0 / rk[k];
        f64 bk = b[k];
        for (i64 i = k + 1; i < n; i++) {
            f64* ri = &a[(u64)i * n];
            f64 f = ri[k] * inv;
            ri[k] = f;
            if (f != 0.0) {
                for (i64 j = k + 1; j < n; j++) {
                    ri[j] -= f * rk[j];
                }
                b[i] -= f * bk;
            }
        }
    }
    for (i64 i = n - 1; i >= 0; i--) {
        f64 s = b[i];
        const f64* ri = &a[(u64)i * n];
        for (i64 j = i + 1; j < n; j++) {
            s -= ri[j] * b[j];
        }
        b[i] = s / ri[i];
    }
    return true;
}

} // namespace sym
