@echo off
REM SPDX-License-Identifier: GPL-3.0-only
REM Copyright (C) 2026 Scintillae Ltd.
REM Find out more at symeda.com
REM Licensed under the GNU GPL version 3 only - see LICENSE for the text.
REM Build all three x64 flavours side by side.
setlocal
call "%~dp0build-msvc.bat"  || exit /b 1
call "%~dp0build-clang.bat" || exit /b 1
call "%~dp0build-mingw.bat" || exit /b 1
echo.
echo All flavours built: bin\msvc-x64, bin\clang-x64, bin\mingw-x64
echo Time them with: symeda --gen-ladder 800 -o bench.sp then --bench N
