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
// File:     parse.cpp
// Dev step: 1
// Date:     2026-10-01
//
// Front-end dispatch: .scs/.ssi extension or a lang=... header marker
// selects the scs grammar, otherwise the classic SPICE grammar.
//----------------------------------------------------------------------
#include "parse.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace sym {

ParseResult parse_spice_file(const std::string& path);
ParseResult parse_scs_file(const std::string& path);

ParseResult parse_file(const std::string& path, const std::string& lang) {
    std::string use = lang;
    if (use.empty()) {
        std::string ext = path.size() > 4 ? path.substr(path.size() - 4) : "";
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });
        if (ext == ".scs" || ext == ".ssi") {
            use = "scs";
        } else {
            std::ifstream f(path, std::ios::binary);
            if (f) {
                char buf[4096];
                f.read(buf, sizeof(buf));
                std::string head(buf, (size_t)f.gcount());
                std::transform(head.begin(), head.end(), head.begin(),
                               [](unsigned char c) { return (char)std::tolower(c); });
                if (head.find("lang=spectre") != std::string::npos) {
                    use = "scs";
                }
            }
        }
    }
    if (use == "scs" || use == "spectre") {
        return parse_scs_file(path);
    }
    return parse_spice_file(path);
}

} // namespace sym
