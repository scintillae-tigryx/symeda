# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Scintillae Ltd.
# Find out more at symeda.com
# Licensed under the GNU GPL version 3 only - see LICENSE for the text.
# SYMEDA - analogue transient simulation engine.
#
# Configure via the repo-root .bat files (build-msvc/clang/mingw.bat) or:
#   cmake --preset msvc|clang|mingw && cmake --build --preset <same>
# Flavours output to bin/<flavor>-x64/symeda.exe.

cmake_minimum_required(VERSION 3.20)
project(symeda CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT SYMEDA_FLAVOR)
    set(SYMEDA_FLAVOR "dev")
endif()

# Single source of truth for the version string: the CLI's --version.
set(SYMEDA_VERSION "0.1.0")

set(SYMEDA_SOURCES
    src/ckt/netlist.cpp
    src/parse/parse.cpp
    src/parse/value.cpp
    src/parse/spice.cpp
    src/parse/scs.cpp
    src/solver/dense.cpp
    src/engine/tran.cpp
    src/out/writer.cpp
)

add_executable(symeda src/symeda.cpp ${SYMEDA_SOURCES})
target_include_directories(symeda PRIVATE src)
target_compile_definitions(symeda PRIVATE SYMEDA_VERSION="${SYMEDA_VERSION}")

# Numeric policy: FMA contraction and errno-free math are enabled, but
# NEVER -ffast-math / /fp:fast - Newton convergence and device models
# depend on IEEE semantics (denormals, signed zero, predictable NaN).
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(symeda PRIVATE
        /W4 /EHsc /O2 /Ob3 /fp:precise /fp:contract /arch:AVX2 /GL /DNDEBUG)
    target_link_options(symeda PRIVATE /LTCG)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang") # clang-cl against MSVC headers
    target_compile_options(symeda PRIVATE
        /EHsc /W3 -O3 -march=native -ffp-contract=fast -fno-math-errno -DNDEBUG)
    target_link_options(symeda PRIVATE /LTCG)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(symeda PRIVATE
        -O3 -march=native -ffp-contract=fast -fno-math-errno
        -flto -Wall -Wextra -DNDEBUG)
    target_link_options(symeda PRIVATE -flto -static)
endif()

set_target_properties(symeda PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${SYMEDA_FLAVOR}-x64")
