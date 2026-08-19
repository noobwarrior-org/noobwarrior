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
// File: main.cpp (DiffPlace)
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Differential check: what did preparing a place do to the instances it already had?
// Reports properties invented, dropped, or value-changed, aggregated by class+property.
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>
using namespace NoobWarrior::Roblox;
using namespace NoobWarrior::Roblox::DataTypes;
using NoobWarrior::Roblox::BinaryFormat::BinaryRobloxFile;

static std::string Path(Instance *i) {
    std::string p;
    for (Instance *n = i; n; n = n->GetParent()) {
        if (n->Name.empty() || p.size() > 300) break;
        p = "/" + n->Name + p;
    }
    return p;
}
static std::string Show(const Property &p) {
    char b[128];
    if (auto *v = p.CastValue<std::string>()) return "\"" + v->substr(0, 32) + "\"";
    if (auto *v = p.CastValue<bool>()) return *v ? "true" : "false";
    if (auto *v = p.CastValue<int32_t>()) { std::snprintf(b, sizeof b, "%d", *v); return b; }
    if (auto *v = p.CastValue<int64_t>()) { std::snprintf(b, sizeof b, "%lld", (long long)*v); return b; }
    if (auto *v = p.CastValue<uint32_t>()) { std::snprintf(b, sizeof b, "%u", *v); return b; }
    if (auto *v = p.CastValue<float>()) { std::snprintf(b, sizeof b, "%g", *v); return b; }
    if (auto *v = p.CastValue<double>()) { std::snprintf(b, sizeof b, "%g", *v); return b; }
    if (auto *v = p.CastValue<Content>()) return "content:" + v->Uri;
    if (auto *v = p.CastValue<ContentId>()) return "contentid:" + v->Uri;
    return "<opaque>";
}
static void Index(BinaryRobloxFile &f, std::map<std::string, Instance *> &out,
                  std::set<std::string> &dupes) {
    for (const auto &o : f.Objects) {
        if (!o) continue;
        const std::string p = Path(o.get());
        if (!out.emplace(p, o.get()).second) dupes.insert(p);
    }
}
int main(int argc, char **argv) {
    auto read = [](const char *p) {
        std::ifstream in(p, std::ios::binary);
        return std::vector<unsigned char>{std::istreambuf_iterator<char>(in),
                                          std::istreambuf_iterator<char>()};
    };
    BinaryRobloxFile a, b;
    std::string e;
    if (!a.Load(read(argv[1]), &e)) { std::printf("source load: %s\n", e.c_str()); return 1; }
    if (!b.Load(read(argv[2]), &e)) { std::printf("prepared load: %s\n", e.c_str()); return 1; }
    std::printf("source=%zu objects  prepared=%zu objects\n\n", a.Objects.size(), b.Objects.size());

    std::map<std::string, Instance *> A, B;
    std::set<std::string> dupA, dupB;
    Index(a, A, dupA); Index(b, B, dupB);

    std::map<std::string, int> invented, dropped, changed;
    std::map<std::string, std::string> sample;
    int matched = 0;
    for (const auto &[path, ia] : A) {
        if (dupA.count(path) || dupB.count(path)) continue;
        auto found = B.find(path);
        if (found == B.end()) continue;
        Instance *ib = found->second;
        if (ib->ClassName != ia->ClassName) continue;
        ++matched;
        const auto &pa = ia->GetProperties();
        const auto &pb = ib->GetProperties();
        for (const auto &[n, v] : pb) {
            const std::string key = ia->ClassName + "." + n;
            auto it = pa.find(n);
            if (it == pa.end()) {
                if (!invented[key]++) sample[key] = "now " + Show(v);
            } else {
                const std::string x = Show(it->second), y = Show(v);
                if (x != y && x != "<opaque>") {
                    if (!changed[key]++) sample[key] = x + " -> " + y;
                }
            }
        }
        for (const auto &[n, v] : pa)
            if (!pb.count(n)) dropped[ia->ClassName + "." + n]++;
    }
    std::printf("matched %d instances by path\n\n", matched);
    auto report = [&](const char *title, std::map<std::string, int> &m) {
        std::printf("--- %s (%zu kinds) ---\n", title, m.size());
        std::vector<std::pair<int, std::string>> v;
        for (const auto &[k, n] : m) v.push_back({n, k});
        std::sort(v.rbegin(), v.rend());
        for (size_t i = 0; i < v.size() && i < 40; ++i)
            std::printf("  %6d  %-42s %s\n", v[i].first, v[i].second.c_str(),
                        sample.count(v[i].second) ? sample[v[i].second].c_str() : "");
        std::printf("\n");
    };
    report("INVENTED properties", invented);
    report("CHANGED values", changed);
    report("DROPPED properties", dropped);
    return 0;
}
