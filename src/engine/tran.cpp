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
// File:     tran.cpp
// Dev step: 1
// Date:     2026-10-01
//
// Transient engine v0: MNA stamp + backward Euler + dense solve.
//
// MNA sign conventions (match SPICE):
//   conductance g between p,q:      A[p][p]+=g A[q][q]+=g A[p][q]-=g A[q][p]-=g
//   current source p->q of value i: b[p]-=i  b[q]+=i   (positive current
//                                   flows from + node through source to -)
//   voltage source p->q, branch k:  A[k][p]+=1 A[k][q]-=1 b[k]=v
//                                   A[p][k]+=1 A[q][k]-=1
//   capacitor BE, g=C/h:            conductance g; b[p]+=g*vdiff_prev,
//                                   b[q]-=g*vdiff_prev
//   inductor BE, branch k:          A[k][k]-=L/h; b[k]=-(L/h)*i_prev
//                                   (at the DC op an L is a 0V source)
//----------------------------------------------------------------------
#include "engine/tran.h"
#include "solver/dense.h"
#include <cmath>
#include <algorithm>
#include <unordered_map>

namespace sym {

f64 src_value(const Source& s, f64 t) {
    switch (s.kind) {
    case SrcKind::DC:
        return s.dc;
    case SrcKind::PULSE: {
        f64 v1 = s.p[0], v2 = s.p[1], td = s.p[2], tr = s.p[3], tf = s.p[4], pw = s.p[5],
            per = s.p[6];
        if (t < td || per <= 0) {
            return v1;
        }
        f64 tt = std::fmod(t - td, per);
        if (tt < tr) {
            return v1 + (v2 - v1) * (tt / tr);
        }
        if (tt < tr + pw) {
            return v2;
        }
        if (tt < tr + pw + tf) {
            return v2 + (v1 - v2) * ((tt - tr - pw) / tf);
        }
        return v1;
    }
    case SrcKind::SIN: {
        f64 vo = s.p[0], va = s.p[1], f = s.p[2], td = s.p[3], th = s.p[4];
        if (t < td) {
            return vo;
        }
        f64 tt = t - td;
        return vo + va * std::exp(-tt * th) * std::sin(2.0 * PI * f * tt);
    }
    case SrcKind::EXP: {
        // SPICE EXP: v1 + (v2-v1)(1-e^-(t-td1)/t1) + (v1-v2)(1-e^-(t-td2)/t2)
        f64 v1 = s.p[0], v2 = s.p[1], td1 = s.p[2], t1 = s.p[3], td2 = s.p[4], t2 = s.p[5];
        f64 v = v1;
        if (t > td1) {
            v += (v2 - v1) * (1.0 - std::exp(-(t - td1) / t1));
        }
        if (t > td2) {
            v += (v1 - v2) * (1.0 - std::exp(-(t - td2) / t2));
        }
        return v;
    }
    case SrcKind::PWL: {
        const auto& w = s.pwl;
        if (w.size() < 2) {
            return s.dc;
        }
        if (t <= w[0]) {
            return w[1];
        }
        for (size_t i = 2; i + 1 < w.size(); i += 2) {
            if (t <= w[i]) {
                f64 dt = w[i] - w[i - 2];
                f64 f = dt > 0 ? (t - w[i - 2]) / dt : 1.0;
                return w[i - 1] + f * (w[i + 1] - w[i - 1]);
            }
        }
        return w.back();
    }
    }
    return s.dc;
}

int run_tran(const Circuit& c, const Opts& o, TranResult& r, FILE* log) {
    if (!c.tran) {
        std::fprintf(log, "error: no transient analysis (.tran / tran) found\n");
        return 1;
    }
    if (c.tstep <= 0 || c.tstop <= 0) {
        std::fprintf(log, "error: .tran needs positive tstep tstop\n");
        return 1;
    }
    if (o.mode == "fast") {
        std::fprintf(log, "warn: --mode fast not implemented; running accurate\n");
    }
    if (o.gpu) {
        std::fprintf(log, "warn: --gpu not implemented; running on CPU\n");
    }

    const i32 nv = c.nnodes() - 1; // voltage unknowns

    std::unordered_map<i32, i32> branch; // elem idx -> branch row
    i32 nb = 0, nonlinear = 0;
    for (i32 i = 0; i < (i32)c.elems.size(); i++) {
        ElemKind k = c.elems[i].kind;
        if (k == ElemKind::V || k == ElemKind::L) {
            branch[i] = nb++;
        } else if (k == ElemKind::D || k == ElemKind::M || k == ElemKind::Q || k == ElemKind::X) {
            nonlinear++;
        }
    }
    if (nonlinear) {
        std::fprintf(log,
                     "warn: %d nonlinear elements ignored "
                     "(models not yet implemented)\n",
                     nonlinear);
    }

    const i32 n = nv + nb;
    DenseMat A;
    A.init(n);
    std::vector<f64> b(n), vprev(c.nnodes(), 0.0), vnow(c.nnodes(), 0.0), jprev(nb, 0.0),
        jnow(nb, 0.0);

    auto stamp_cond = [&](i32 p, i32 q, f64 g) {
        if (p) {
            A.at(p - 1, p - 1) += g;
        }
        if (q) {
            A.at(q - 1, q - 1) += g;
        }
        if (p && q) {
            A.at(p - 1, q - 1) -= g;
            A.at(q - 1, p - 1) -= g;
        }
    };
    auto stamp_br = [&](i32 p, i32 q, i32 k) {
        A.at(k, k) = 0.0;
        if (p) {
            A.at(k, p - 1) += 1;
            A.at(p - 1, k) += 1;
        }
        if (q) {
            A.at(k, q - 1) -= 1;
            A.at(q - 1, k) -= 1;
        }
    };

    auto stamp = [&](f64 t, f64 h, bool op) {
        A.zero();
        std::fill(b.begin(), b.end(), 0.0);
        for (i32 i = 0; i < (i32)c.elems.size(); i++) {
            const Element& e = c.elems[i];
            i32 p = e.nodes[0], q = e.nodes[1];
            switch (e.kind) {
            case ElemKind::R:
                if (e.value == 0.0) {
                    break;
                }
                stamp_cond(p, q, 1.0 / e.value);
                break;
            case ElemKind::C:
                if (!op) {
                    f64 g = e.value / h;
                    f64 vh = g * (vprev[p] - vprev[q]);
                    stamp_cond(p, q, g);
                    if (p) {
                        b[p - 1] += vh;
                    }
                    if (q) {
                        b[q - 1] -= vh;
                    }
                }
                break;
            case ElemKind::I: {
                f64 i = src_value(e.src, t);
                if (p) {
                    b[p - 1] -= i;
                }
                if (q) {
                    b[q - 1] += i;
                }
                break;
            }
            case ElemKind::V: {
                i32 k = nv + branch.at(i);
                stamp_br(p, q, k);
                b[k] = src_value(e.src, t);
                break;
            }
            case ElemKind::L: {
                i32 k = nv + branch.at(i);
                stamp_br(p, q, k);
                if (!op) {
                    A.at(k, k) -= e.value / h;
                    b[k] = -(e.value / h) * jprev[branch.at(i)];
                }
                break;
            }
            default:
                break; // nonlinear: warned above
            }
        }
        for (i32 i = 0; i < nv; i++) {
            A.at(i, i) += o.gmin;
        }
    };

    auto extract = [&]() {
        for (i32 i = 1; i < c.nnodes(); i++) {
            vnow[i] = b[i - 1];
        }
        for (i32 k = 0; k < nb; k++) {
            jnow[k] = b[nv + k];
        }
    };

    // result columns: v(every non-ground node), i(every branch)
    r.names.clear();
    for (i32 i = 1; i < c.nnodes(); i++) {
        r.names.push_back("v(" + c.names[i] + ")");
    }
    for (auto& [ei, k] : branch) {
        r.names.push_back("i(" + c.elems[ei].name + ")");
    }
    // keep columns in declaration order for readability
    std::sort(r.names.begin() + nv, r.names.end());
    r.y.assign(r.names.size(), {});
    r.t.clear();

    auto record = [&](f64 t) {
        r.t.push_back(t);
        i32 col = 0;
        for (i32 i = 1; i < c.nnodes(); i++) {
            r.y[col++].push_back(vnow[i]);
        }
        // branch columns were sorted by name; rebuild in that order
        std::vector<i32> order(branch.size());
        i32 k = 0;
        for (auto& [ei, kk] : branch) {
            order[k++] = ei;
        }
        std::sort(order.begin(), order.end(),
                  [&](i32 a, i32 b) { return c.elems[a].name < c.elems[b].name; });
        for (i32 ei : order) {
            r.y[col++].push_back(jnow[branch.at(ei)]);
        }
    };

    if (n > 0) {
        // DC operating point: caps open, inductors short, sources at t=0
        stamp(0.0, 0.0, true);
        if (!A.lu_solve(b.data())) {
            std::fprintf(log, "error: singular matrix at operating point "
                              "(floating nodes?)\n");
            return 1;
        }
        extract();
        vprev = vnow;
        jprev = jnow;
        record(c.tstart);

        f64 h = c.tstep;
        f64 tend = c.tstop;
        i32 guard = 0;
        for (f64 t = h; t <= tend + 1e-15; t += h) {
            stamp(t, h, false);
            if (!A.lu_solve(b.data())) {
                std::fprintf(log, "error: singular matrix at t=%g\n", t);
                return 1;
            }
            extract();
            if (t >= c.tstart) {
                record(t);
            }
            std::swap(vprev, vnow);
            std::swap(jprev, jnow);
            if (++guard > 4000000) {
                std::fprintf(log, "error: step limit exceeded\n");
                return 1;
            }
        }
        r.steps = (i32)r.t.size();
    }
    return 0;
}

} // namespace sym
