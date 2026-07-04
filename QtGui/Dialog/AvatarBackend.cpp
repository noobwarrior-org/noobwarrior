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
// File: AvatarBackend.cpp
// Started by: Hattozo
// Started on: 7/3/2026
// Description:
#include <cpr/cpr.h> // must precede any header that pulls in windows.h (macro collisions in cpr)

#include "AvatarBackend.h"

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/Roblox/Api/Asset.h>
#include <NoobWarrior/Roblox/DataType/BrickColor.h>
#include <NoobWarrior/Roblox/DataType/Color3.h>

#include <sol/sol.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <set>
#include <utility>

using namespace NoobWarrior;

const std::vector<AvatarSlotDef>& NoobWarrior::AvatarSlots() {
    using AT = Roblox::AssetType;
    static const std::vector<AvatarSlotDef> slots = {
        { "user.appearance.shirt",  static_cast<int>(AT::Shirt) },
        { "user.appearance.pants",  static_cast<int>(AT::Pants) },
        { "user.appearance.tshirt", static_cast<int>(AT::TShirt) },
        { "user.appearance.face",   static_cast<int>(AT::Face) },
        { "user.appearance.body.torso",     static_cast<int>(AT::Torso) },
        { "user.appearance.body.left_arm",  static_cast<int>(AT::LeftArm) },
        { "user.appearance.body.right_arm", static_cast<int>(AT::RightArm) },
        { "user.appearance.body.left_leg",  static_cast<int>(AT::LeftLeg) },
        { "user.appearance.body.right_leg", static_cast<int>(AT::RightLeg) },
        { "user.appearance.animation.walk",  static_cast<int>(AT::WalkAnimation) },
        { "user.appearance.animation.run",   static_cast<int>(AT::RunAnimation) },
        { "user.appearance.animation.fall",  static_cast<int>(AT::FallAnimation) },
        { "user.appearance.animation.jump",  static_cast<int>(AT::JumpAnimation) },
        { "user.appearance.animation.swim",  static_cast<int>(AT::SwimAnimation) },
        { "user.appearance.animation.climb", static_cast<int>(AT::ClimbAnimation) },
        { "user.appearance.animation.idle",  static_cast<int>(AT::IdleAnimation) },
    };
    return slots;
}

const std::vector<std::string>& NoobWarrior::AvatarColorParts() {
    static const std::vector<std::string> parts = {
        "head", "torso", "left_arm", "right_arm", "left_leg", "right_leg"
    };
    return parts;
}

const std::vector<std::string>& NoobWarrior::AvatarScaleKeys() {
    static const std::vector<std::string> keys = {
        "user.appearance.scale.height", "user.appearance.scale.width", "user.appearance.scale.head",
        "user.appearance.scale.depth", "user.appearance.scale.proportion", "user.appearance.scale.body_type"
    };
    return keys;
}

bool NoobWarrior::IsAvatarAccessoryType(int assetType) {
    using AT = Roblox::AssetType;
    switch (static_cast<AT>(assetType)) {
    case AT::Hat:
    case AT::FaceAccessory:
    case AT::NeckAccessory:
    case AT::ShoulderAccessory:
    case AT::FrontAccessory:
    case AT::BackAccessory:
    case AT::WaistAccessory:
    case AT::Gear:
    case AT::HairAccessory:
    case AT::DynamicHead:
    case AT::EmoteAnimation:
        return true;
    default:
        return false;
    }
}

// Escapes LIKE wildcards so a literal search doesn't match everything (ESCAPE '\').
static std::string EscapeLike(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '%' || c == '_' || c == '\\')
            out += '\\';
        out += c;
    }
    return out;
}

LocalRegistryBackend::LocalRegistryBackend(Core* core) : mCore(core) {}

std::string LocalRegistryBackend::Describe() {
    return "your local player";
}

bool LocalRegistryBackend::Load(AvatarData& out) {
    Registry* reg = mCore->GetRegistry();
    if (reg == nullptr)
        return false;

    for (const std::string& part : AvatarColorParts())
        if (auto v = reg->GetKeyValue<std::string>("user.appearance.color." + part))
            out.Colors[part] = *v;

    for (const std::string& key : AvatarScaleKeys())
        out.Scales[key] = reg->GetKeyValue<double>(key).value_or(0.0);

    out.AvatarType = reg->GetKeyValue<std::string>("user.appearance.avatar_type").value_or("R6");

    for (const AvatarSlotDef& s : AvatarSlots())
        out.Slots[s.RegKey] = reg->GetKeyValue<int64_t>(s.RegKey).value_or(0);

    if (auto table = reg->GetKeyValue<sol::table>("user.appearance.accessories")) {
        const std::size_t count = table->size();
        for (std::size_t i = 1; i <= count; ++i) {
            sol::object obj = (*table)[i];
            if (obj.get_type() == sol::type::number) {
                int64_t id = obj.as<int64_t>();
                if (id > 0)
                    out.Accessories.push_back(id);
            }
        }
    }
    return true;
}

bool LocalRegistryBackend::Save(const AvatarData& data) {
    Registry* reg = mCore->GetRegistry();
    if (reg == nullptr)
        return false;

    for (const auto& [part, name] : data.Colors)
        reg->SetKeyValue("user.appearance.color." + part, name);

    for (const auto& [key, val] : data.Scales)
        reg->SetKeyValue<double>(key, val);

    reg->SetKeyValue("user.appearance.avatar_type", data.AvatarType);

    for (const AvatarSlotDef& s : AvatarSlots()) {
        auto it = data.Slots.find(s.RegKey);
        reg->SetKeyValue<int64_t>(s.RegKey, it != data.Slots.end() ? it->second : 0);
    }

    sol::table table = mCore->GetLuaState()->create_table();
    int n = 0;
    for (int64_t id : data.Accessories)
        table[++n] = id;
    reg->SetKeyValue("user.appearance.accessories", table);
    return true;
}

AvatarCatalogPage LocalRegistryBackend::Catalog(int assetType, const std::string& search, int page) {
    constexpr int kPageSize = 60;
    std::vector<AvatarCatalogItem> items;
    std::set<int64_t> seen;
    std::string escaped = EscapeLike(search);

    EmuDbManager* mgr = mCore->GetEmuDbManager();
    for (EmuDb* db : mgr->GetMountedDatabases()) {
        std::string sql = "SELECT Id, Name FROM Asset WHERE Type = ?";
        if (!search.empty())
            sql += " AND Name LIKE ? ESCAPE '\\'";
        sql += ";";
        Statement stmt = db->PrepareStatement(sql);
        stmt.Bind(1, assetType);
        if (!search.empty())
            stmt.Bind(2, "%" + escaped + "%");
        while (stmt.Step() == SQLITE_ROW) {
            int64_t id = stmt.GetInt64FromColumnIndex(0);
            if (seen.insert(id).second)
                items.push_back({ id, stmt.GetStringFromColumnIndex(1), assetType });
        }
    }

    std::sort(items.begin(), items.end(),
              [](const AvatarCatalogItem& a, const AvatarCatalogItem& b) { return a.Name < b.Name; });

    AvatarCatalogPage out;
    int total = static_cast<int>(items.size());
    out.PageCount = std::max(1, (total + kPageSize - 1) / kPageSize);
    out.Page = std::clamp(page, 0, out.PageCount - 1);
    int start = out.Page * kPageSize;
    int end = std::min(total, start + kPageSize);
    for (int i = start; i < end; i++)
        out.Items.push_back(items[i]);
    return out;
}

ThumbnailFetcher LocalRegistryBackend::MakeThumbnailFetcher() {
    return {}; // the local player renders straight from the mounted databases; no async fetch needed
}

// ------------------------------------------------------------------------------------------------
// RemoteAccountBackend — a DB-backed account on a server, over HTTP.

// AvatarData color part -> the avatar-fetch bodyColor3s key.
static const std::vector<std::pair<std::string, std::string>>& ColorPartToFetchKey() {
    static const std::vector<std::pair<std::string, std::string>> m = {
        { "head", "headColor3" }, { "torso", "torsoColor3" },
        { "left_arm", "leftArmColor3" }, { "right_arm", "rightArmColor3" },
        { "left_leg", "leftLegColor3" }, { "right_leg", "rightLegColor3" },
    };
    return m;
}

// Full scale registry key -> the avatar-fetch scales key (registry "body_type" == fetch "bodyType").
static std::string ScaleFetchKey(const std::string& fullKey) {
    static const std::string prefix = "user.appearance.scale.";
    std::string s = fullKey.rfind(prefix, 0) == 0 ? fullKey.substr(prefix.size()) : fullKey;
    return s == "body_type" ? "bodyType" : s;
}

static std::string HexToBrickName(std::string hex) {
    if (!hex.empty() && hex.front() == '#')
        hex = hex.substr(1);
    if (hex.size() < 6)
        return "";
    try {
        int r = std::stoi(hex.substr(0, 2), nullptr, 16);
        int g = std::stoi(hex.substr(2, 2), nullptr, 16);
        int b = std::stoi(hex.substr(4, 2), nullptr, 16);
        return Roblox::BrickColor(Roblox::Color3::fromRGB(r, g, b)).Name();
    } catch (...) {
        return "";
    }
}

static std::string BrickNameToHex(const std::string& name) {
    int packed = Roblox::BrickColor::PackedRgbForNumber(Roblox::BrickColor::NumberForName(name));
    if (packed < 0)
        return "";
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%06X", packed & 0xFFFFFF);
    return buf;
}

RemoteAccountBackend::RemoteAccountBackend(std::string baseUrl, std::string session, std::string label)
    : mBaseUrl(std::move(baseUrl)), mSession(std::move(session)), mLabel(std::move(label)) {
    while (!mBaseUrl.empty() && mBaseUrl.back() == '/')
        mBaseUrl.pop_back();
}

std::string RemoteAccountBackend::Describe() {
    return mLabel;
}

bool RemoteAccountBackend::Load(AvatarData& out) {
    cpr::Response res = cpr::Get(
        cpr::Url{ mBaseUrl + "/emu/v1/avatar/mine" },
        cpr::Header{ { "Cookie", ".LOGINSESSION=" + mSession } },
        cpr::Timeout{ std::chrono::milliseconds(8000) },
        cpr::VerifySsl{ false });
    if (res.error.code != cpr::ErrorCode::OK || res.status_code != 200)
        return false;
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(res.text);
    } catch (...) {
        return false;
    }

    std::map<int, std::string> typeToSlot;
    for (const AvatarSlotDef& s : AvatarSlots()) {
        typeToSlot[s.AssetType] = s.RegKey;
        out.Slots[s.RegKey] = 0;
    }
    if (j.contains("assetAndAssetTypeIds") && j["assetAndAssetTypeIds"].is_array()) {
        for (const auto& e : j["assetAndAssetTypeIds"]) {
            int64_t id = e.value("assetId", static_cast<int64_t>(0));
            int type = e.value("assetTypeId", 0);
            if (id <= 0)
                continue;
            auto it = typeToSlot.find(type);
            if (it != typeToSlot.end())
                out.Slots[it->second] = id;
            else
                out.Accessories.push_back(id);
        }
    }

    if (j.contains("bodyColor3s") && j["bodyColor3s"].is_object()) {
        const auto& c = j["bodyColor3s"];
        for (const auto& [part, fetchKey] : ColorPartToFetchKey()) {
            if (c.contains(fetchKey) && c[fetchKey].is_string()) {
                std::string name = HexToBrickName(c[fetchKey].get<std::string>());
                if (!name.empty())
                    out.Colors[part] = name;
            }
        }
    }

    if (j.contains("scales") && j["scales"].is_object()) {
        const auto& sc = j["scales"];
        for (const std::string& fullKey : AvatarScaleKeys()) {
            std::string fetchKey = ScaleFetchKey(fullKey);
            if (sc.contains(fetchKey) && sc[fetchKey].is_number())
                out.Scales[fullKey] = sc[fetchKey].get<double>();
        }
    }

    out.AvatarType = j.value("resolvedAvatarType", std::string("R6"));
    return true;
}

bool RemoteAccountBackend::Save(const AvatarData& data) {
    nlohmann::json j;

    nlohmann::json worn = nlohmann::json::array();
    for (const auto& [regKey, id] : data.Slots)
        if (id > 0)
            worn.push_back({ { "assetId", id }, { "assetTypeId", 0 } });
    for (int64_t id : data.Accessories)
        if (id > 0)
            worn.push_back({ { "assetId", id }, { "assetTypeId", 0 } });
    j["assetAndAssetTypeIds"] = worn;

    nlohmann::json colors = nlohmann::json::object();
    for (const auto& [part, fetchKey] : ColorPartToFetchKey()) {
        auto it = data.Colors.find(part);
        if (it != data.Colors.end()) {
            std::string hex = BrickNameToHex(it->second);
            if (!hex.empty())
                colors[fetchKey] = hex;
        }
    }
    j["bodyColor3s"] = colors;

    nlohmann::json scales = nlohmann::json::object();
    for (const auto& [fullKey, val] : data.Scales)
        scales[ScaleFetchKey(fullKey)] = val;
    j["scales"] = scales;
    j["resolvedAvatarType"] = data.AvatarType;

    cpr::Response res = cpr::Post(
        cpr::Url{ mBaseUrl + "/emu/v1/avatar/mine" },
        cpr::Header{ { "Cookie", ".LOGINSESSION=" + mSession }, { "Content-Type", "application/json" } },
        cpr::Body{ j.dump() },
        cpr::Timeout{ std::chrono::milliseconds(8000) },
        cpr::VerifySsl{ false });
    return res.error.code == cpr::ErrorCode::OK && res.status_code == 200;
}

AvatarCatalogPage RemoteAccountBackend::Catalog(int assetType, const std::string& search, int page) {
    AvatarCatalogPage out;
    // Only send non-empty params: cpr emits an empty value as a valueless key (`search`, not `search=`),
    // which makes libevent's evhttp_parse_query fail the whole query server-side (dropping type/page too).
    cpr::Parameters params{ { "type", std::to_string(assetType) }, { "page", std::to_string(page) } };
    if (!search.empty())
        params.Add({ "search", search });
    cpr::Response res = cpr::Get(
        cpr::Url{ mBaseUrl + "/emu/v1/avatar/catalog" },
        params,
        cpr::Header{ { "Cookie", ".LOGINSESSION=" + mSession } },
        cpr::Timeout{ std::chrono::milliseconds(8000) },
        cpr::VerifySsl{ false });
    if (res.error.code != cpr::ErrorCode::OK || res.status_code != 200)
        return out;
    try {
        nlohmann::json j = nlohmann::json::parse(res.text);
        out.Page = j.value("page", 0);
        out.PageCount = j.value("pageCount", 1);
        if (j.contains("items") && j["items"].is_array())
            for (const auto& e : j["items"])
                out.Items.push_back({ e.value("id", static_cast<int64_t>(0)),
                                      e.value("name", std::string()),
                                      e.value("assetTypeId", assetType) });
    } catch (...) {
    }
    return out;
}

static std::vector<unsigned char> HttpFetchThumbnail(const std::string& baseUrl, const std::string& session, int64_t assetId) {
    cpr::Response res = cpr::Get(
        cpr::Url{ baseUrl + "/emu/v1/avatar/thumbnail" },
        cpr::Parameters{ { "id", std::to_string(assetId) } },
        cpr::Header{ { "Cookie", ".LOGINSESSION=" + session } },
        cpr::Timeout{ std::chrono::milliseconds(8000) },
        cpr::VerifySsl{ false });
    if (res.error.code != cpr::ErrorCode::OK || res.status_code != 200)
        return {};
    return std::vector<unsigned char>(res.text.begin(), res.text.end());
}

ThumbnailFetcher RemoteAccountBackend::MakeThumbnailFetcher() {
    // Capture the URL + session by value so the callable is independent of this backend's lifetime and can
    // be run on a background thread even after the backend/dialog is gone.
    return [base = mBaseUrl, session = mSession](int64_t assetId) {
        return HttpFetchThumbnail(base, session, assetId);
    };
}
