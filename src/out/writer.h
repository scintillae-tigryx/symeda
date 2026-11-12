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
// File:     writer.h
// Dev step: 1
// Date:     2026-10-01
//
// Output writers. csv now; the industry .tr0 and .psf binary output
// formats are the compatibility targets on the roadmap.
//----------------------------------------------------------------------
#pragma once
#include "engine/tran.h"
#include <string>

namespace sym {

bool write_csv(const std::string& path, const TranResult& r, FILE* log);

} // namespace sym
