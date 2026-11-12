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
// File:     arena.h
// Dev step: 1
// Date:     2026-10-01
//
// Bump-pointer arena. Hot-path allocations (matrix data, per-step
// vectors) come out of arenas so a timestep does zero malloc/free churn.
//----------------------------------------------------------------------
#pragma once
#include "types.h"
#include <cstdlib>
#include <new>

namespace sym {

struct Arena {
    u8* base = nullptr;
    u64 cap = 0;
    u64 used = 0;

    bool init(u64 bytes) {
        base = (u8*)std::malloc(bytes);
        cap = base ? bytes : 0;
        used = 0;
        return base != nullptr;
    }
    void destroy() {
        std::free(base);
        base = nullptr;
        cap = used = 0;
    }
    void reset() { used = 0; }

    void* alloc(u64 bytes, u64 align = 16) {
        u64 p = (used + align - 1) & ~(align - 1);
        if (p + bytes > cap) {
            return nullptr;
        }
        used = p + bytes;
        return base + p;
    }
    template <class T> T* alloc_n(u64 n) { return (T*)alloc(sizeof(T) * n, alignof(T)); }
};

} // namespace sym
