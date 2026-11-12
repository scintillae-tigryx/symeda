# SYMEDA

Aiming to create the world's fastest analogue transient simulation engine - well we can dream :-)

It takes a circuit netlist in and produces node waveforms out — with
HSPICE and Spectre netlist compatibility, multicore parallelism, and GPU
acceleration where it actually pays off.

Status: early scaffold. Today it parses both dialects, stamps MNA for
linear elements (R/C/L/V/I with DC/PULSE/SIN/PWL/EXP sources), integrates
with backward Euler against a dense LU, and writes CSV. Everything on the
hot path is being built for speed from the start.

## License

The engine source and test corpus are released under the GNU General
Public License version 3 only (GPL-3.0-only) - see LICENSE.
Copyright (C) 2026 Scintillae Ltd.

## Build

Three x64 toolchain flavours are built side by side and raced:

```
build-all.bat        rem or: build-msvc.bat / build-clang.bat / build-mingw.bat
```

The executable has built-in benchmark generators and a timing flag:
`symeda --gen-ladder 1000 -o bench.sp`, then `--bench N` to time N runs.

Outputs land in `bin\<flavor>-x64\symeda.exe`.

## Usage

```
symeda <netlist> [-o out.csv] [--mode spice|fast] [--threads N]
                 [--gpu auto|off] [--bench N] [--lang spice|scs]
symeda --gen-ladder 1000 -o bench.sp      # generate a benchmark netlist
```

Netlists are auto-detected: `.scs` or a `lang=` header selects the
Spectre-dialect front end, otherwise the HSPICE/SPICE3 dialect.
