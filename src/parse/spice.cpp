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
// File:     spice.cpp
// Dev step: 1
// Date:     2026-10-01
//
// Classic SPICE netlist front end: the Berkeley SPICE3 card grammar,
// including the hierarchical-SPICE extensions common in industry decks.
//
// Covered: comments (* line, $ inline), + continuation, R C L V I D Q M X
// element cards, DC/PULSE/SIN/PWL/EXP sources, .tran, .include, .end.
// Subcircuits, .model, .param and other dot cards warn and skip for now -
// elaboration is on the roadmap.
//----------------------------------------------------------------------
#include "parse.h"
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>

namespace sym {

namespace {

std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> t;
    std::istringstream is(s);
    std::string w;
    while (is >> w) {
        t.push_back(w);
    }
    return t;
}

std::string lower(std::string s) {
    for (auto& c : s) {
        c = (char)std::tolower((unsigned char)c);
    }
    return s;
}

bool is_num(const std::string& s, f64& v) {
    return parse_value(s, v, false);
}

// Join "(...)" argument lists that were split across whitespace, then
// split the args on whitespace/commas.
bool fn_args(const std::string& joined, std::string& fname, std::vector<f64>& args, bool& has_pwl) {
    auto lp = joined.find('(');
    auto rp = joined.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp < lp) {
        return false;
    }
    fname = lower(joined.substr(0, lp));
    std::string body = joined.substr(lp + 1, rp - lp - 1);
    for (auto& c : body) {
        if (c == ',') {
            c = ' ';
        }
    }
    std::istringstream is(body);
    std::string w;
    has_pwl = (fname == "pwl");
    while (is >> w) {
        f64 v;
        if (!parse_value(w, v, false)) {
            // PWL may carry paired expressions like (0 0 1n 1); plain
            // numbers cover v0. Non-numeric tokens fail the card.
            return false;
        }
        args.push_back(v);
    }
    return true;
}

struct SpiceParser {
    ParseResult& r;
    std::unordered_set<std::string> warned;
    bool in_subckt = false;
    bool in_control = false;

    void warn(const std::string& w) {
        if (warned.insert(w).second) {
            r.ckt.warns.push_back(w);
        }
    }
    bool fail(int line, const std::string& m) {
        r.err = m;
        r.err_line = line;
        return false;
    }

    bool parse_file(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) {
            return fail(0, "cannot open " + path);
        }
        std::stringstream ss;
        ss << f.rdbuf();
        std::string dir = path.substr(0, path.find_last_of("/\\") + 1);
        return parse_text(ss.str(), dir);
    }

    bool parse_text(const std::string& text, const std::string& dir) {
        // Logical lines: a line whose first non-blank char is '+' is a
        // continuation of the previous.
        std::vector<std::pair<std::string, int>> lines;
        {
            std::istringstream is(text);
            std::string raw;
            int ln = 0;
            while (std::getline(is, raw)) {
                ln++;
                if (!raw.empty() && raw.back() == '\r') {
                    raw.pop_back();
                }
                size_t b = raw.find_first_not_of(" \t");
                if (b != std::string::npos && raw[b] == '+' && !lines.empty()) {
                    lines.back().first += " " + raw.substr(b + 1);
                    continue;
                }
                lines.emplace_back(raw, ln);
            }
        }

        bool first = true;
        for (auto& [raw, ln] : lines) {
            std::string line = raw;
            size_t b = line.find_first_not_of(" \t");
            if (b == std::string::npos) {
                continue;
            }
            if (line[b] == '*') {
                continue; // comment card
            }
            size_t d = line.find('$'); // inline comment
            if (d != std::string::npos) {
                line = line.substr(0, d);
            }
            auto tok = split(line);
            if (tok.empty()) {
                continue;
            }

            std::string t0 = lower(tok[0]);
            // .control ... .endc script blocks belong to the simulator's
            // UI, not the netlist - skip the whole block (the lines inside
            // don't start with '.' and could parse as element cards).
            if (in_control) {
                if (t0 == ".endc") {
                    in_control = false;
                }
                continue;
            }
            if (t0 == ".control") {
                in_control = true;
                continue;
            }

            if (t0[0] == '.') {
                if (in_subckt) {
                    if (t0 == ".ends") {
                        in_subckt = false;
                    }
                    continue;
                }
                if (t0 == ".end") {
                    break;
                }
                if (t0 == ".tran") {
                    std::vector<f64> nums;
                    for (size_t i = 1; i < tok.size(); i++) {
                        std::string lt = lower(tok[i]);
                        if (lt == "uic") {
                            continue;
                        }
                        size_t eq = lt.find('=');
                        if (eq != std::string::npos) {
                            lt = lt.substr(eq + 1);
                        }
                        f64 v;
                        if (is_num(lt, v)) {
                            nums.push_back(v);
                        }
                    }
                    if (nums.size() >= 2) {
                        r.ckt.tran = true;
                        r.ckt.tstep = nums[0];
                        r.ckt.tstop = nums[1];
                        if (nums.size() >= 3) {
                            r.ckt.tstart = nums[2];
                        }
                    } else {
                        return fail(ln, ".tran needs tstep tstop");
                    }
                    continue;
                }
                if (t0 == ".include" || t0 == ".inc") {
                    if (tok.size() < 2) {
                        return fail(ln, ".include needs a file");
                    }
                    std::string inc = tok[1];
                    if (inc.front() == '"' || inc.front() == '\'') {
                        inc = inc.substr(1, inc.size() - 2);
                    }
                    if (inc.find(':') == std::string::npos && inc.front() != '/' &&
                        inc.front() != '\\') {
                        inc = dir + inc;
                    }
                    SpiceParser sub{r, warned, false};
                    sub.warned = warned;
                    if (!sub.parse_file(inc)) {
                        return false;
                    }
                    warned = std::move(sub.warned);
                    continue;
                }
                if (t0 == ".subckt") {
                    in_subckt = true;
                    warn("note: .subckt not yet implemented; body skipped");
                    continue;
                }
                warn("note: card " + t0 + " ignored (not yet implemented)");
                continue;
            }

            if (in_subckt) {
                continue;
            }

            // element card
            Element e;
            e.name = tok[0];
            char kind = (char)std::toupper((unsigned char)t0[0]);
            switch (kind) {
            case 'R':
            case 'C':
            case 'L': {
                if (tok.size() < 4) {
                    return fail(ln, "bad " + e.name + " card");
                }
                e.nodes[0] = r.ckt.node(tok[1]);
                e.nodes[1] = r.ckt.node(tok[2]);
                e.nnodes = 2;
                f64 v;
                if (!is_num(tok[3], v)) {
                    warn("note: model-based " + e.name + " skipped");
                    continue;
                }
                e.value = v;
                e.kind = kind == 'R' ? ElemKind::R : kind == 'C' ? ElemKind::C : ElemKind::L;
                break;
            }
            case 'V':
            case 'I': {
                if (tok.size() < 3) {
                    return fail(ln, "bad " + e.name + " card");
                }
                e.nodes[0] = r.ckt.node(tok[1]);
                e.nodes[1] = r.ckt.node(tok[2]);
                e.nnodes = 2;
                e.kind = kind == 'V' ? ElemKind::V : ElemKind::I;
                // join the rest; function forms carry parens
                std::string rest;
                for (size_t i = 3; i < tok.size(); i++) {
                    rest += tok[i] + " ";
                }
                if (rest.find('(') != std::string::npos) {
                    std::string fn;
                    std::vector<f64> a;
                    bool pwl;
                    if (!fn_args(rest, fn, a, pwl)) {
                        return fail(ln, "cannot parse source: " + rest);
                    }
                    if (fn == "pulse" && a.size() >= 7) {
                        e.src.kind = SrcKind::PULSE;
                        for (int i = 0; i < 7; i++) {
                            e.src.p[i] = a[i];
                        }
                    } else if (fn == "sin" && a.size() >= 3) {
                        e.src.kind = SrcKind::SIN;
                        for (size_t i = 0; i < a.size() && i < 8; i++) {
                            e.src.p[i] = a[i];
                        }
                    } else if (fn == "exp" && a.size() >= 6) {
                        e.src.kind = SrcKind::EXP;
                        for (int i = 0; i < 6; i++) {
                            e.src.p[i] = a[i];
                        }
                    } else if (fn == "pwl" && a.size() >= 2) {
                        e.src.kind = SrcKind::PWL;
                        e.src.pwl = a;
                    } else {
                        return fail(ln, "unsupported/bad source fn " + fn);
                    }
                } else {
                    f64 v = 0.0;
                    for (size_t i = 3; i < tok.size(); i++) {
                        std::string lt = lower(tok[i]);
                        if (lt == "dc" || lt == "ac") {
                            continue;
                        }
                        if (is_num(lt, v)) {
                            break; // ac mag: ignore rest
                        }
                    }
                    e.src.kind = SrcKind::DC;
                    e.src.dc = v;
                }
                break;
            }
            case 'D': {
                if (tok.size() < 4) {
                    return fail(ln, "bad " + e.name);
                }
                e.kind = ElemKind::D;
                e.nodes[0] = r.ckt.node(tok[1]);
                e.nodes[1] = r.ckt.node(tok[2]);
                e.nnodes = 2;
                e.model = tok[3];
                break;
            }
            case 'Q': {
                if (tok.size() < 5) {
                    return fail(ln, "bad " + e.name);
                }
                e.kind = ElemKind::Q;
                for (int i = 0; i < 3; i++) {
                    e.nodes[i] = r.ckt.node(tok[i + 1]);
                }
                e.nnodes = 3;
                e.model = tok[4];
                break;
            }
            case 'M': {
                if (tok.size() < 6) {
                    return fail(ln, "bad " + e.name);
                }
                e.kind = ElemKind::M;
                for (int i = 0; i < 4; i++) {
                    e.nodes[i] = r.ckt.node(tok[i + 1]);
                }
                e.nnodes = 4;
                e.model = tok[5];
                break;
            }
            case 'X': {
                e.kind = ElemKind::X;
                e.model = tok.size() > 1 ? tok.back() : "";
                for (size_t i = 1; i + 1 < tok.size() && e.nnodes < 4; i++) {
                    e.nodes[e.nnodes++] = r.ckt.node(tok[i]);
                }
                break;
            }
            case 'E':
            case 'F':
            case 'G':
            case 'H':
            case 'S':
            case 'W':
            case 'T':
            case 'O':
            case 'K':
            case 'B': {
                warn("note: element " + e.name + " (" + kind + ") ignored - not yet implemented");
                continue;
            }
            default:
                if (first) {
                    continue; // probably a title line
                }
                return fail(ln, "unrecognised card: " + tok[0]);
            }
            r.ckt.elems.push_back(std::move(e));
            first = false;
        }
        return true;
    }
};

} // namespace

ParseResult parse_spice_file(const std::string& path) {
    ParseResult r;
    SpiceParser p{r, {}, false};
    p.parse_file(path);
    return r;
}

ParseResult parse_spice_text(const std::string& text, const std::string& dir) {
    ParseResult r;
    SpiceParser p{r, {}, false};
    p.parse_text(text, dir);
    return r;
}

} // namespace sym
