* SPDX-License-Identifier: GPL-3.0-only
* Copyright (C) 2026 Scintillae Ltd. - symeda.com
* Current-source sign convention: I p q i stamps b[p]-=i b[q]+=i, so
* I1 0 pos pushes pos up to +1V while I2 neg 0 pulls neg down to -1V.
I1 0 pos DC 1m
I2 neg 0 DC 1m
R1 pos 0 1k
R2 neg 0 1k
.tran 1n 1u
.end
