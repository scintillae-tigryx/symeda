* SPDX-License-Identifier: GPL-3.0-only
* Copyright (C) 2026 Scintillae Ltd. - symeda.com
* Engineering-suffix spellings: each divider writes 1k a different way,
* so every tap must settle at exactly half the 1V drive.
*   a: "1k" vs plain "1e3"        b: "0.001meg" vs uppercase "1K"
*   c: milli "1000m" vs plain "1" d: "1k" vs "1000000m"
V1 in 0 DC 1
R1 in a 1k
R2 a 0 1e3
R3 in b 0.001meg
R4 b 0 1K
R5 in c 1000m
R6 c 0 1
R7 in d 1k
R8 d 0 1000000m
.tran 1n 1u
.end
