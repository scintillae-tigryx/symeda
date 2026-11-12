* SPDX-License-Identifier: GPL-3.0-only
* Copyright (C) 2026 Scintillae Ltd. - symeda.com
* DC divider: out should sit at 0.5 * 2V = 1V
V1 in 0 DC 2
R1 in out 1k
R2 out 0 1k
.tran 1u 10u
.end
