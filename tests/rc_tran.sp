* SPDX-License-Identifier: GPL-3.0-only
* Copyright (C) 2026 Scintillae Ltd. - symeda.com
* RC transient test: pulse into 1k/1nF (tau=1us), checks BE integration
V1 in 0 PULSE(0 1 0 1n 1n 0.5u 1u)
R1 in out 1k
C1 out 0 1n
.tran 10n 5u
.end
