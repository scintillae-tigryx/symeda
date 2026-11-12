* SPDX-License-Identifier: GPL-3.0-only
* Copyright (C) 2026 Scintillae Ltd. - symeda.com
* Two different voltage sources across the same node pair: the MNA
* matrix is structurally singular and the solve must give up.
V1 a 0 DC 1
V2 a 0 DC 2
.tran 1n 1u
.end
