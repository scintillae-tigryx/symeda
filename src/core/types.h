// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Scintillae Ltd.
// Find out more at symeda.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

//----------------------------------------------------------------------
// File:     types.h
// Dev step: 1
// Date:     2026-10-01
//
// Common fixed-width type aliases. Short names are used throughout.
//----------------------------------------------------------------------
#pragma once
#include <cstdint>

namespace sym {

using f64 = double;
using f32 = float;
using i64 = std::int64_t;
using u64 = std::uint64_t;
using i32 = std::int32_t;
using u32 = std::uint32_t;
using i16 = std::int16_t;
using u16 = std::uint16_t;
using i8 = std::int8_t;
using u8 = std::uint8_t;

constexpr f64 PI = 3.14159265358979323846;

} // namespace sym
