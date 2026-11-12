@echo off
REM SPDX-License-Identifier: GPL-3.0-only
REM Copyright (C) 2026 Scintillae Ltd.
REM Find out more at symeda.com
REM Licensed under the GNU GPL version 3 only - see LICENSE for the text.
REM Build symeda with clang-cl. Output: bin\clang-x64\symeda.exe
REM
REM Needs LLVM >= 19: the VS 18 STL uses __builtin_verbose_trap, which
REM clang 18 doesn't have. Override the path with LLVM_DIR=... if needed.
setlocal
set MSYSTEM=
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1

if not defined LLVM_DIR (
    for /d %%D in ("C:\Program Files\clang+llvm-*") do set "LLVM_DIR=%%D"
)
if not defined LLVM_DIR (
    echo No LLVM installation found under "C:\Program Files\clang+llvm-*".
    echo Install LLVM 19+ ^(winget install LLVM.LLVM^) or set LLVM_DIR.
    exit /b 1
)
set "PATH=%LLVM_DIR%\bin;%PATH%"
where clang-cl
clang-cl --version | findstr /R "clang.version"
cmake --preset clang || exit /b 1
cmake --build --preset clang
