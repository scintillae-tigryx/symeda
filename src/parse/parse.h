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
// File:     parse.h
// Dev step: 1
// Date:     2026-10-01
//
// Netlist front ends. One IR, two grammars:
//   spice.cpp - classic SPICE netlist dialect (SPICE3 lineage,
//               including the hierarchical-SPICE extension)
//   scs.cpp   - .scs netlist dialect ("simulator lang=..." headers)
//----------------------------------------------------------------------
#pragma once
#include "ckt/netlist.h"
#include <string>

namespace sym {

struct ParseResult {
    Circuit ckt;
    std::string err;
    int err_line = 0; // 1-based, in the top-level file
    bool ok() const { return err.empty(); }
};

// lang: "spice", "scs" (or the dialect's own name), or "" (auto:
// .scs/.ssi extension or a lang=... header marker wins).
ParseResult parse_file(const std::string& path, const std::string& lang = "");

// Engineering-suffix number parser shared by both front ends.
// classic mode: m=1e-3, meg=1e6, case-insensitive
// scs mode:     SI convention - m=1e-3 but M=1e6 (case-sensitive)
bool parse_value(const std::string& s, f64& out, bool scs_mode);

} // namespace sym
