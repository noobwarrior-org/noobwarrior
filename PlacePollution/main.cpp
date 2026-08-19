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
// File: main.cpp (PlacePollution)
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Finds columns that look mounted-introduced: a small minority carry a real value and the
// overwhelming majority hold the type's zero value, which is the DefaultValueFor fill.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>
using namespace NoobWarrior::Roblox;
using namespace NoobWarrior::Roblox::DataTypes;
using NoobWarrior::Roblox::BinaryFormat::BinaryRobloxFile;
static bool IsZero(const Property &p) {
    switch (p.Type) {
    case PropertyType::String: { auto *v = p.CastValue<std::string>(); return v && v->empty(); }
    case PropertyType::Bool:   { auto *v = p.CastValue<bool>(); return v && !*v; }
    case PropertyType::Int:    { auto *v = p.CastValue<int32_t>(); return v && *v == 0; }
    case PropertyType::Int64:  { auto *v = p.CastValue<int64_t>(); return v && *v == 0; }
    case PropertyType::Float:  { auto *v = p.CastValue<float>(); return v && *v == 0.0f; }
    case PropertyType::Double: { auto *v = p.CastValue<double>(); return v && *v == 0.0; }
    case PropertyType::Enum:   { auto *v = p.CastValue<uint32_t>(); return v && *v == 0; }
    case PropertyType::Ref:    { auto *v = p.CastValue<int32_t>(); return v && *v < 0; }
    case PropertyType::Content:{ auto *v = p.CastValue<Content>(); return v && v->Uri.empty(); }
    default: return false;
    }
}
int main(int argc, char **argv) {
    std::ifstream in(argv[1], std::ios::binary);
    std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(in),
                                     std::istreambuf_iterator<char>()};
    BinaryRobloxFile file; std::string error;
    if (!file.Load(bytes, &error)) { std::printf("load failed %s\n", error.c_str()); return 1; }
    std::map<std::string, std::map<std::string, std::pair<int,int>>> tally; // cls -> prop -> (n, zero)
    for (const auto &o : file.Objects) {
        if (!o) continue;
        for (const auto &[n, p] : o->GetProperties()) {
            auto &t = tally[o->ClassName][n];
            ++t.first;
            if (IsZero(p)) ++t.second;
        }
    }
    std::printf("%-22s %-26s %6s %6s\n", "CLASS", "PROPERTY", "N", "ZERO");
    for (const auto &[cls, props] : tally)
        for (const auto &[prop, t] : props) {
            if (t.first < 3) continue;
            const int nonZero = t.first - t.second;
            // The signature: nearly everything defaulted, a handful real.
            if (nonZero > 0 && nonZero <= 12 && t.second >= t.first * 0.75)
                std::printf("%-22s %-26s %6d %6d\n", cls.c_str(), prop.c_str(), t.first, t.second);
        }
    return 0;
}
