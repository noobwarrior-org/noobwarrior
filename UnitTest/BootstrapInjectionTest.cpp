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
// File: BootstrapInjectionTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: The path StudioServerPlace actually uses: load a real place, append a bootstrap
//              script, and serialize. Regression cover for mixed String/ProtectedString columns.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

using NoobWarrior::Roblox::LuaSourceContainerSpec;
using NoobWarrior::Roblox::BinaryFormat::BinaryRobloxFile;

TEST(BootstrapInjection, AppendsAndSerializesEveryInstalledPlace) {
    const char *root = std::getenv("NOOBWARRIOR_ENGINES_DIR");
    if (root == nullptr)
        GTEST_SKIP() << "set NOOBWARRIOR_ENGINES_DIR";

    int ok = 0;
    std::error_code code;
    for (auto it = std::filesystem::recursive_directory_iterator(root, code);
         !code && it != std::filesystem::recursive_directory_iterator(); it.increment(code)) {
        if (it->path().extension() != ".rbxl")
            continue;
        std::ifstream stream(it->path(), std::ios::binary);
        const std::vector<unsigned char> bytes(
            (std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        if (bytes.empty())
            continue;

        BinaryRobloxFile file;
        std::string error;
        if (!file.Load(bytes, &error))
            continue;

        const LuaSourceContainerSpec spec {
            "Script", "__noobWarriorPluginBootstrap", "print('x')", false, "ServerScriptService"};
        ASSERT_TRUE(file.AppendLuaSourceContainers(std::span(&spec, 1), &error))
            << it->path().string() << ": " << error;

        std::vector<unsigned char> written;
        ASSERT_TRUE(file.Save(written, &error)) << it->path().string() << ": " << error;

        BinaryRobloxFile reloaded;
        ASSERT_TRUE(reloaded.Load(written, &error)) << it->path().string() << ": " << error;
        ++ok;
    }
    std::cout << "injected a bootstrap into " << ok << " places" << std::endl;
    EXPECT_GT(ok, 0);
}
