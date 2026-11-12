* SPDX-License-Identifier: GPL-3.0-only
* Copyright (C) 2026 Scintillae Ltd. - symeda.com
* Series RLC ring-down: w0 = 1/sqrt(1uH * 1nF) = 31.6Mrad/s (5 MHz),
* Q = w0*L/R ~ 316 -> nearly undamped oscillation at the OP kick.
* Driven by a pulse; exercises the inductor branch stamping.
V1 in 0 PULSE(0 1 0 1n 1n 500n 1u)
R1 in n1 0.1
L1 n1 out 1u
C1 out 0 1n
.tran 1n 3u
.end
