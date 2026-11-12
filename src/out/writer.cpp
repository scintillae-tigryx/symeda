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
// File:     writer.cpp
// Dev step: 1
// Date:     2026-10-01
//
// CSV writer for transient results: a time column first, then one
// column per probed signal.
//----------------------------------------------------------------------
#include "out/writer.h"
#include <cstdio>

namespace sym {

bool write_csv(const std::string& path, const TranResult& r, FILE* log) {
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) {
        std::fprintf(log, "error: cannot write %s\n", path.c_str());
        return false;
    }
    std::fprintf(f, "time");
    for (auto& n : r.names) {
        std::fprintf(f, ",%s", n.c_str());
    }
    std::fprintf(f, "\n");
    for (size_t i = 0; i < r.t.size(); i++) {
        std::fprintf(f, "%.9g", r.t[i]);
        for (auto& col : r.y) {
            std::fprintf(f, ",%.9g", col[i]);
        }
        std::fprintf(f, "\n");
    }
    std::fclose(f);
    return true;
}

} // namespace sym
