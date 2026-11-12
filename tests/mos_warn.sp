* SPDX-License-Identifier: GPL-3.0-only
* Copyright (C) 2026 Scintillae Ltd. - symeda.com
* Exercises the nonlinear-not-yet-supported path: M/Q/D/X warn and skip
M1 out in 0 0 nch w=1u l=0.1u
Q1 c b 0 npn
D1 in d1 dmod
X1 a b amp
V1 in 0 DC 1
R1 out 0 1k
.tran 10n 1u
.end
