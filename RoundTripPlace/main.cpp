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
// File: main.cpp
// Started by: Hattozo
// Started on: 8/18/2026
// Description: This deserializes a place and reserializes it back, so that we can see if
// the binary reader/writer works properly and Roblox can parse it correctly.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

using NoobWarrior::Roblox::BinaryFormat::BinaryRobloxFile;

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "usage: NoobWarrior.RoundTripPlace <input.rbxl> <output.rbxl>" << std::endl;
        return 2;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "could not open " << argv[1] << std::endl;
        return 1;
    }
    const std::vector<unsigned char> bytes(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    BinaryRobloxFile file;
    std::string error;
    if (!file.Load(bytes, &error)) {
        std::cerr << "load failed: " << error << std::endl;
        return 1;
    }

    std::vector<unsigned char> written;
    if (!file.Save(written, &error)) {
        std::cerr << "save failed: " << error << std::endl;
        return 1;
    }

    // Reload what was written, so a corrupt result is caught here rather than in Studio.
    BinaryRobloxFile verify;
    if (!verify.Load(written, &error)) {
        std::cerr << "the written place could not be read back: " << error << std::endl;
        return 1;
    }
    if (verify.NumObjects != file.NumObjects || verify.NumClasses != file.NumClasses) {
        std::cerr << "object counts changed across the round trip" << std::endl;
        return 1;
    }

    std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
    if (!output) {
        std::cerr << "could not write " << argv[2] << std::endl;
        return 1;
    }
    output.write(reinterpret_cast<const char *>(written.data()),
                 static_cast<std::streamsize>(written.size()));
    if (!output) {
        std::cerr << "could not finish writing " << argv[2] << std::endl;
        return 1;
    }

    std::cout << "ok: " << file.NumObjects << " objects, " << file.NumClasses << " classes, "
              << bytes.size() << " -> " << written.size() << " bytes" << std::endl;
    return 0;
}
