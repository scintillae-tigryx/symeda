* SPDX-License-Identifier: GPL-3.0-only
* Copyright (C) 2026 Scintillae Ltd. - symeda.com
* Inductor at the DC op: stamped as a 0V source, so out snaps to v(in)
* and 1mA flows into R1. With v(L) = 0 the branch current then holds,
* keeping v(out) pinned at 1V for the whole run.
V1 in 0 DC 1
L1 in out 1u
R1 out 0 1k
.tran 1n 1u
.end
