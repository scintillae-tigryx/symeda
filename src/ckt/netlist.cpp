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
// File:     netlist.cpp
// Dev step: 1
// Date:     2026-10-01
//
// Node-name interning for the Circuit IR. Names are case-folded and the
// usual ground aliases (0, gnd, gnd!, ground) all collapse to node 0.
//----------------------------------------------------------------------
#include "ckt/netlist.h"
#include <algorithm>
#include <cctype>

namespace sym {

i32 Circuit::node(const std::string& raw) {
    std::string n = raw;
    std::transform(n.begin(), n.end(), n.begin(),
                   [](unsigned char ch) { return (char)std::tolower(ch); });
    if (n == "0" || n == "gnd" || n == "gnd!" || n == "ground") {
        return 0;
    }
    auto it = ids.find(n);
    if (it != ids.end()) {
        return it->second;
    }
    i32 id = (i32)names.size();
    ids[n] = id;
    names.push_back(n);
    return id;
}

} // namespace sym
