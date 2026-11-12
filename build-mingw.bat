@echo off
REM SPDX-License-Identifier: GPL-3.0-only
REM Copyright (C) 2026 Scintillae Ltd.
REM Find out more at symeda.com
REM Licensed under the GNU GPL version 3 only - see LICENSE for the text.
REM Build symeda with MSYS2 mingw64 g++. Output: bin\mingw-x64\symeda.exe
setlocal
set MSYSTEM=
set "PATH=C:\Program Files\MYSYS2\mingw64\bin;%PATH%"
cmake --preset mingw || exit /b 1
cmake --build --preset mingw
