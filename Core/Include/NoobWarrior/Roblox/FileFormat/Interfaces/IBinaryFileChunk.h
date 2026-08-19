/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: IBinaryFileChunk.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: This file is derived from Roblox-File-Format
//              (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Interfaces/IBinaryFileChunk.cs)
#pragma once

#include <string>

namespace NoobWarrior::Roblox {
class BinaryRobloxFileReader;
class BinaryRobloxFileWriter;

// IBinaryFileChunk.cs:6-11. The reference declares Load/Save/WriteInfo as void and reports
// trouble by throwing; noobWarrior's convention is that a fallible operation returns a bool and
// fills an optional message, so the two reading/writing members carry that shape instead.
//
// The reference hangs this interface off the decoded chunk *bodies* (INST, PROP, PRNT, ...) and
// keeps one on BinaryRobloxFileChunk as its `Handler`. This port decodes bodies inline while
// walking the file, so no body outlives the walk and there is no handler to store; the interface
// is implemented by BinaryRobloxFileChunk itself, which is the one type that does read and write
// a chunk end to end.
class IBinaryFileChunk {
public:
    virtual ~IBinaryFileChunk() = default;

    // Reads one chunk, header and payload, advancing the reader past it.
    virtual bool Load(BinaryRobloxFileReader &reader, std::string *error = nullptr) = 0;
    // Writes one chunk. Passing compress=false, or a payload LZ4 fails to shrink, stores the
    // payload verbatim.
    virtual bool Save(BinaryRobloxFileWriter &writer, bool compress = true,
                      std::string *error = nullptr) const = 0;
    // Appends a human-readable one-line summary of the chunk, for diagnostics only. Nothing in
    // the read or write path consults it.
    virtual void WriteInfo(std::string &builder) const = 0;
};
}
