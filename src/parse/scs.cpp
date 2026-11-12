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
// File:     scs.cpp
// Dev step: 1
// Date:     2026-10-01
//
// .scs netlist front end - minimal dialect:
//   name (n1 n2 ...) master k=v k=v ...
//   resistor capacitor inductor vsource isource diode + model references
//   <name> tran stop=.. step=..
// Comments are // ; continuation is a trailing backslash.
//----------------------------------------------------------------------
#include "parse.h"
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

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

struct ScsParser {
    ParseResult& r;
    std::unordered_set<std::string> warned;

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

    // Extract [ ... ] groups (e.g. wave=[0 0 1n 1]) as single tokens so
    // k=v pairs keep their array values together.
    std::vector<std::string> tokenize(std::string line) {
        std::vector<std::string> out;
        size_t i = 0, n = line.size();
        while (i < n) {
            while (i < n && isspace((unsigned char)line[i])) {
                i++;
            }
            if (i >= n) {
                break;
            }
            if (line[i] == '(' || line[i] == ')') {
                i++;
                continue;
            }
            size_t s = i;
            int depth = 0;
            while (i < n && (depth > 0 || !isspace((unsigned char)line[i]))) {
                if (line[i] == '[') {
                    depth++;
                }
                if (line[i] == ']') {
                    depth--;
                }
                i++;
            }
            out.push_back(line.substr(s, i - s));
        }
        return out;
    }

    static std::unordered_map<std::string, std::string>
    params_of(const std::vector<std::string>& tok, size_t from) {
        std::unordered_map<std::string, std::string> m;
        for (size_t i = from; i < tok.size(); i++) {
            auto eq = tok[i].find('=');
            if (eq != std::string::npos) {
                m[lower(tok[i].substr(0, eq))] = tok[i].substr(eq + 1);
            }
        }
        return m;
    }

    f64 val(const std::unordered_map<std::string, std::string>& m, const char* k, f64 dflt = 0.0) {
        auto it = m.find(k);
        if (it == m.end()) {
            return dflt;
        }
        f64 v;
        if (!parse_value(it->second, v, true)) {
            return dflt;
        }
        return v;
    }

    void fill_source(Element& e, const std::unordered_map<std::string, std::string>& m) {
        e.src.dc = val(m, "dc", 0.0);
        auto it = m.find("type");
        std::string type = it != m.end() ? lower(it->second) : "dc";
        if (type == "pulse") {
            e.src.kind = SrcKind::PULSE;
            e.src.p[0] = val(m, "val0");
            e.src.p[1] = val(m, "val1");
            e.src.p[2] = val(m, "delay");
            e.src.p[3] = val(m, "rise");
            e.src.p[4] = val(m, "fall");
            e.src.p[5] = val(m, "width");
            e.src.p[6] = val(m, "period");
        } else if (type == "sine") {
            e.src.kind = SrcKind::SIN;
            e.src.p[0] = val(m, "sinedc", e.src.dc);
            e.src.p[1] = val(m, "ampl");
            e.src.p[2] = val(m, "freq", val(m, "fund"));
            e.src.p[3] = val(m, "delay");
            e.src.p[4] = val(m, "damp");
        } else if (type == "pwl") {
            e.src.kind = SrcKind::PWL;
            auto w = m.find("wave");
            if (w != m.end()) {
                std::string b = w->second;
                for (auto& c : b) {
                    if (c == '[' || c == ']' || c == ',') {
                        c = ' ';
                    }
                }
                std::istringstream is(b);
                std::string s;
                f64 v;
                while (is >> s) {
                    if (parse_value(s, v, true)) {
                        e.src.pwl.push_back(v);
                    }
                }
            }
        } else if (type == "exp") {
            e.src.kind = SrcKind::EXP;
            e.src.p[0] = val(m, "val0");
            e.src.p[1] = val(m, "val1");
            e.src.p[2] = val(m, "td0");
            e.src.p[3] = val(m, "tau0");
            e.src.p[4] = val(m, "td1");
            e.src.p[5] = val(m, "tau1");
        } else {
            e.src.kind = SrcKind::DC; // type=dc and friends
        }
    }

    bool parse_text(const std::string& text, const std::string& dir) {
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
                size_t cm = raw.find("//");
                if (cm != std::string::npos) {
                    raw = raw.substr(0, cm);
                }
                while (!raw.empty() && raw.back() == '\\') {
                    raw.pop_back();
                    std::string nxt;
                    if (!std::getline(is, nxt)) {
                        break;
                    }
                    ln++;
                    size_t c2 = nxt.find("//");
                    if (c2 != std::string::npos) {
                        nxt = nxt.substr(0, c2);
                    }
                    raw += " " + nxt;
                }
                lines.emplace_back(raw, ln);
            }
        }

        for (auto& [raw, ln] : lines) {
            size_t b = raw.find_first_not_of(" \t");
            if (b == std::string::npos) {
                continue;
            }
            if (raw[b] == '*') {
                continue;
            }
            auto tok = tokenize(raw);
            if (tok.empty()) {
                continue;
            }
            std::string t0 = lower(tok[0]);

            if (t0 == "simulator" || t0 == "global" || t0 == "save" || t0 == "export" ||
                t0 == "options" || t0 == "stats") {
                for (auto& t : tok) {
                    if (lower(t) == "lang=spice") {
                        warn("note: lang=spice sections not supported");
                    }
                }
                continue;
            }
            if (t0 == "include" || t0 == "ahdl_include") {
                if (t0 == "ahdl_include") {
                    warn("note: Verilog-A not yet supported: " + raw);
                    continue;
                }
                if (tok.size() < 2) {
                    return fail(ln, "include needs a file");
                }
                std::string inc = tok[1];
                if (inc.front() == '"') {
                    inc = inc.substr(1, inc.size() - 2);
                }
                if (inc.find(':') == std::string::npos && inc.front() != '/' &&
                    inc.front() != '\\') {
                    inc = dir + inc;
                }
                ScsParser sub{r, warned};
                if (!sub.parse_file(inc)) {
                    return false;
                }
                warned = std::move(sub.warned);
                continue;
            }
            if (t0 == "model" || t0 == "subckt" || t0 == "ends" || t0 == "parameters" ||
                t0 == "inline") {
                warn("note: '" + t0 + "' not yet implemented (ignored)");
                continue;
            }

            if (tok.size() < 2) {
                return fail(ln, "bad statement: " + raw);
            }

            // tokenize() drops the parens, so tok[1] is either a node
            // name (element statement) or the analysis type.
            if (lower(tok[1]) == "tran") {
                auto m = params_of(tok, 2);
                r.ckt.tran = true;
                r.ckt.tstop = val(m, "stop", 0.0);
                r.ckt.tstep = val(m, "step", r.ckt.tstop / 100.0);
                r.ckt.tstart = val(m, "start", 0.0);
                continue;
            }
            if (lower(tok[1]) == "dc" || lower(tok[1]) == "ac" || lower(tok[1]) == "noise" ||
                lower(tok[1]) == "info") {
                warn("note: analysis " + tok[1] + " ignored (tran only)");
                continue;
            }

            // element: name (n...) master k=v ...  - the node span comes
            // from raw so k=v pairs in the tail can't confuse it.
            size_t lp = raw.find('('), rp = raw.find(')', lp);
            if (lp == std::string::npos || rp == std::string::npos) {
                return fail(ln, "cannot parse: " + raw);
            }
            Element e;
            e.name = tok[0];
            for (auto& nn : split(raw.substr(lp + 1, rp - lp - 1))) {
                if (e.nnodes < 4) {
                    e.nodes[e.nnodes++] = r.ckt.node(nn);
                }
            }
            // master = first token after ')'
            {
                std::string after = raw.substr(rp + 1);
                auto at = tokenize(after);
                if (at.empty()) {
                    return fail(ln, "missing master: " + raw);
                }
                std::string master = lower(at[0]);
                auto m = params_of(at, 1);
                if (master == "resistor") {
                    e.kind = ElemKind::R;
                    e.value = val(m, "r");
                } else if (master == "capacitor") {
                    e.kind = ElemKind::C;
                    e.value = val(m, "c");
                } else if (master == "inductor") {
                    e.kind = ElemKind::L;
                    e.value = val(m, "l");
                } else if (master == "vsource" || master == "isource") {
                    e.kind = master == "vsource" ? ElemKind::V : ElemKind::I;
                    fill_source(e, m);
                } else if (master == "diode") {
                    e.kind = ElemKind::D;
                    e.model = master;
                } else {
                    // model reference (mosfet, bjt, subckt instance)
                    e.model = master;
                    e.kind = e.nnodes >= 4   ? ElemKind::M
                             : e.nnodes == 3 ? ElemKind::Q
                                             : ElemKind::D;
                    if (e.nnodes > 4) {
                        warn("note: " + e.name + " has >4 terminals");
                    }
                }
            }
            r.ckt.elems.push_back(std::move(e));
        }
        return true;
    }
};

} // namespace

ParseResult parse_scs_file(const std::string& path) {
    ParseResult r;
    ScsParser p{r, {}};
    p.parse_file(path);
    return r;
}

} // namespace sym
