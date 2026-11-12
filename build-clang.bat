@echo off
REM SPDX-License-Identifier: GPL-3.0-only
REM Copyright (C) 2026 Scintillae Ltd.
REM Find out more at symeda.com
REM Licensed under the GNU GPL version 3 only - see LICENSE for the text.
REM Build symeda with clang++ targeting the MinGW ABI (MSYS2 sysroot,
REM lld). Output: bin\clang-x64\symeda.exe
REM
REM GNU-driver clang works on LLVM >= 18: it compiles against MSYS2's
REM libstdc++, not the VS 18 STL (whose __builtin_verbose_trap needs
REM clang 19 - which is why clang-cl is no longer used here). Override
REM paths with LLVM_DIR / MSYS2_DIR if needed.
setlocal
set MSYSTEM=

if not defined LLVM_DIR (
    for /d %%D in ("C:\Program Files\clang+llvm-*") do set "LLVM_DIR=%%D"
    if not defined LLVM_DIR if exist "C:\Program Files\LLVM\bin\clang++.exe" set "LLVM_DIR=C:\Program Files\LLVM"
)
if not defined LLVM_DIR (
    echo No LLVM installation found under "C:\Program Files\clang+llvm-*" or "C:\Program Files\LLVM".
    echo Install LLVM ^(winget install LLVM.LLVM^) or set LLVM_DIR.
    exit /b 1
)
if not defined MSYS2_DIR set "MSYS2_DIR=C:\Program Files\MYSYS2\mingw64"
if not exist "%MSYS2_DIR%\include" (
    echo MSYS2 mingw64 not found at %MSYS2_DIR% - set MSYS2_DIR.
    exit /b 1
)
REM The preset reads the sysroot from SYMEDA_MSYS2 (forward slashes).
set "SYMEDA_MSYS2=%MSYS2_DIR:\=/%"
set "PATH=%LLVM_DIR%\bin;%MSYS2_DIR%\bin;%PATH%"
where clang++
clang++ --version | findstr /R "clang.version"
cmake --preset clang || exit /b 1
cmake --build --preset clang
