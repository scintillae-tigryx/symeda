@echo off
REM SPDX-License-Identifier: GPL-3.0-only
REM Copyright (C) 2026 Scintillae Ltd.
REM Find out more at symeda.com
REM Licensed under the GNU GPL version 3 only - see LICENSE for the text.
REM Build symeda with MSVC (VS 18 Community). Output: bin\msvc-x64\symeda.exe
setlocal
set MSYSTEM=
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cmake --preset msvc || exit /b 1
cmake --build --preset msvc
