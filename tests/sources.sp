* SPDX-License-Identifier: GPL-3.0-only
* Copyright (C) 2026 Scintillae Ltd. - symeda.com
* Source waveforms: SIN damped, EXP, PWL - each into an RC load
Vsin sin 0 SIN(0 1 1meg 0 1e8)
Vexp exp 0 EXP(0 1 0.1u 0.1u 0.6u 0.1u)
Vpwl pwl 0 PWL(0 0 0.5u 1 1.5u 0.2 2u 0.2)
R1 sin so 1k
C1 so 0 1n
R2 exp eo 1k
C2 eo 0 1n
R3 pwl po 1k
C3 po 0 1n
.tran 10n 4u
.end
