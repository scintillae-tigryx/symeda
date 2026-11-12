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
// File:     symeda.cpp
// Dev step: 1
// Date:     2026-10-01
//
// symeda - analogue transient simulation engine
//
//   symeda <netlist> [-o out.csv] [options]
//
// Options:
//   --format csv          output format (csv now; tr0, psf planned)
//   --mode spice|fast     accuracy tier (fast planned)
//   --threads N           parallel eval threads (planned)
//   --gpu auto|off        GPU backend (planned)
//   --bench N             run the sim N times, report timing
//   --gen-ladder N        emit an N-stage RC ladder netlist and exit
//   --lang spice|scs      force front end (default: auto)
//   -h, --help            this text
//   --version             version string
//----------------------------------------------------------------------
#include "core/types.h"
#include "parse/parse.h"
#include "engine/tran.h"
#include "out/writer.h"
#include <cstdio>
#include <cstring>
#include <chrono>
#include <string>

// Stamped by CMake (SYMEDA_VERSION); the fallback keeps ad-hoc compiles
// of this file building.
#ifndef SYMEDA_VERSION
#define SYMEDA_VERSION "0.0.0"
#endif

namespace {

const char* USAGE = "symeda - analogue transient simulation engine\n"
                    "usage: symeda <netlist> [-o out.csv] [options]\n"
                    "  --format csv          output format (csv now; tr0, psf planned)\n"
                    "  --mode spice|fast     accuracy tier (fast planned)\n"
                    "  --threads N           parallel eval threads (planned)\n"
                    "  --gpu auto|off        GPU backend (planned)\n"
                    "  --bench N             run the sim N times, report timing\n"
                    "  --gen-ladder N        emit an N-stage RC ladder netlist and exit\n"
                    "  --gen-mesh N          emit an NxN RC mesh netlist and exit\n"
                    "  --lang spice|scs      force front end (default: auto)\n"
                    "  -h, --help            this text\n"
                    "  --version             version and licence notice\n"
                    "License GPLv3 (version 3 only). Copyright (C) 2026 Scintillae Ltd.\n";

// The notice the GPL wants users to see: -h/--version print it, and a
// one-line form goes to stderr on every real run.
const char* NOTICE = "symeda " SYMEDA_VERSION "\n"
                     "Copyright (C) 2026 Scintillae Ltd.\n"
                     "License GPLv3: GNU General Public License version 3 only\n"
                     "    <https://gnu.org/licenses/gpl.html>\n"
                     "This is free software: you are free to change and redistribute it.\n"
                     "There is NO WARRANTY, to the extent permitted by law.\n";

void gen_ladder(int nstages, FILE* f) {
    std::fprintf(f, "* SYMEDA generated RC ladder, %d stages\n", nstages);
    std::fprintf(f, "V1 in 0 PULSE(0 1 0 1n 1n 10u 100u)\n");
    std::fprintf(f, "R0 in n1 100\n");
    for (int i = 1; i <= nstages; i++) {
        std::fprintf(f, "C%d n%d 0 1p\n", i, i);
        if (i < nstages) {
            std::fprintf(f, "R%d n%d n%d 100\n", i, i, i + 1);
        }
    }
    std::fprintf(f, "Rout n%d out 100\n", nstages);
    std::fprintf(f, ".tran 0.5u 200u\n.end\n");
}

// N x N resistor grid, 1pF per node to ground, driven at one corner.
// MNA matrix is banded with bandwidth ~N - stresses the solver with a
// different fill profile than the ladder's tridiagonal structure.
void gen_mesh(int n, FILE* f) {
    std::fprintf(f, "* SYMEDA generated %dx%d RC mesh\n", n, n);
    std::fprintf(f, "V1 in 0 PULSE(0 1 0 1n 1n 10u 100u)\n");
    std::fprintf(f, "Rin in n_0_0 100\n");
    int en = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            std::fprintf(f, "C%d n_%d_%d 0 1p\n", ++en, i, j);
        }
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (j + 1 < n) {
                std::fprintf(f, "Rh%d n_%d_%d n_%d_%d 1k\n", ++en, i, j, i, j + 1);
            }
            if (i + 1 < n) {
                std::fprintf(f, "Rv%d n_%d_%d n_%d_%d 1k\n", ++en, i, j, i + 1, j);
            }
        }
    }
    std::fprintf(f, ".tran 0.5u 50u\n.end\n");
}

} // namespace

int main(int argc, char** argv) {
    std::string in, out = "-", lang;
    sym::Opts o;
    std::string format = "csv";
    int bench = 1, ladder = 0, mesh = 0;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto need = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "missing value for %s\n", a.c_str());
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "-o") {
            out = need();
        } else if (a == "--format") {
            format = need();
        } else if (a == "--mode") {
            o.mode = need();
        } else if (a == "--threads") {
            o.threads = std::atoi(need().c_str());
        } else if (a == "--gpu") {
            o.gpu = need() != "off";
        } else if (a == "--bench") {
            bench = std::atoi(need().c_str());
        } else if (a == "--lang") {
            lang = need();
        } else if (a == "--gen-ladder") {
            ladder = std::atoi(need().c_str());
        } else if (a == "--gen-mesh") {
            mesh = std::atoi(need().c_str());
        } else if (a == "-h" || a == "--help") {
            std::fputs(USAGE, stdout);
            return 0;
        } else if (a == "--version") {
            std::fputs(NOTICE, stdout);
            return 0;
        } else if (a[0] == '-') {
            std::fprintf(stderr, "unknown option %s\n", a.c_str());
            return 2;
        } else {
            in = a;
        }
    }

    if (ladder > 0 || mesh > 0) {
        FILE* f = out == "-" ? stdout : std::fopen(out.c_str(), "w");
        if (!f) {
            std::fprintf(stderr, "cannot write %s\n", out.c_str());
            return 1;
        }
        if (ladder > 0) {
            gen_ladder(ladder, f);
        } else {
            gen_mesh(mesh, f);
        }
        if (f != stdout) {
            std::fclose(f);
        }
        return 0;
    }
    if (in.empty()) {
        std::fputs(USAGE, stderr);
        return 2;
    }
    if (format != "csv") {
        std::fprintf(stderr, "error: only --format csv supported so far\n");
        return 2;
    }

    // once per run, on stderr so stdout stays pipe-clean
    std::fprintf(stderr,
                 "symeda %s  Copyright (C) 2026 Scintillae Ltd. - "
                 "License GPLv3: this program comes with "
                 "ABSOLUTELY NO WARRANTY.\n",
                 SYMEDA_VERSION);

    auto pr = sym::parse_file(in, lang);
    if (!pr.ok()) {
        std::fprintf(stderr, "%s:%d: %s\n", in.c_str(), pr.err_line, pr.err.c_str());
        return 1;
    }
    for (auto& w : pr.ckt.warns) {
        std::fprintf(stderr, "%s\n", w.c_str());
    }

    sym::TranResult r;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < bench; i++) {
        r = sym::TranResult{};
        if (sym::run_tran(pr.ckt, o, r, stderr) != 0) {
            return 1;
        }
    }
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (out == "-") {
        out = in + ".csv";
    }
    if (!sym::write_csv(out, r, stderr)) {
        return 1;
    }

    std::fprintf(stderr, "%d nodes, %d steps -> %s\n", pr.ckt.nnodes() - 1, r.steps, out.c_str());
    if (bench > 1) {
        std::fprintf(stderr, "bench: %d runs, %.1f ms total, %.1f ms avg\n", bench, ms, ms / bench);
    }
    return 0;
}
