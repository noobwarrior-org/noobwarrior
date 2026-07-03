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
// File: AvatarAppearance.cpp
// Started by: Hattozo
// Started on: 6/18/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/AvatarAppearance.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Roblox/Api/Asset.h>
#include <NoobWarrior/Roblox/DataType/BrickColor.h>
#include <NoobWarrior/Roblox/DataType/Color3.h>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

using namespace NoobWarrior;
using AssetType = Roblox::AssetType;

namespace {

struct WornAsset {
    int64_t Id;
    int     TypeId; // Roblox::AssetType value
};

// The local player's appearance, assembled from the registry once, then formatted for either
// endpoint. Body colors are kept as BrickColor *names* (what the registry stores) and resolved to
// both the exact numeric BrickColor id and the exact hex on demand.
struct Appearance {
    std::vector<WornAsset> Worn;
    std::map<std::string, int64_t> Animations; // state name ("run", "walk", ...) -> animation asset id
    std::string HeadColor, TorsoColor, RightArmColor, LeftArmColor, RightLegColor, LeftLegColor;
    double Height, Width, Head, Depth, Proportion, BodyType;
    std::string AvatarType = "R6"; // "R6" or "R15"
    bool HasShirt = false, HasPants = false;
};

// Exact hex for a BrickColor name (no nearest-match: the registry only ever stores palette names).
std::string HexForBrickName(const std::string& name) {
    for (const auto& entry : Roblox::BrickColor::Palette)
        if (name == entry.name)
            return entry.hex;
    return "#A3A2A5"; // Medium stone grey
}

// Exact numeric BrickColor id for a name; defaults to Medium stone grey (194) for an unknown name.
int BrickColorIdForName(const std::string& name) {
    int n = Roblox::BrickColor::NumberForName(name);
    return n >= 0 ? n : 194;
}

// Resolves a worn asset's Roblox asset type from whichever mounted database holds it, defaulting to
// `fallback` when no mounted database knows the asset.
int ResolveType(Core* core, int64_t id, AssetType fallback) {
    if (auto summary = core->GetEmuDbManager()->GetAssetSummary(id); summary.has_value() && summary->Type != 0)
        return summary->Type;
    return static_cast<int>(fallback);
}

Appearance ReadAppearance(Core* core) {
    Appearance a{};
    Registry* reg = core->GetRegistry();

    auto regId = [&](const char* key) -> int64_t {
        return reg->GetKeyValue<int64_t>(key).value_or(0);
    };
    auto add = [&](int64_t id, int typeId) {
        if (id > 0) a.Worn.push_back({ id, typeId });
    };

    // Clothing (fixed types).
    int64_t shirt = regId("user.appearance.shirt");
    int64_t pants = regId("user.appearance.pants");
    a.HasShirt = shirt > 0;
    a.HasPants = pants > 0;
    add(shirt, static_cast<int>(AssetType::Shirt));
    add(pants, static_cast<int>(AssetType::Pants));
    add(regId("user.appearance.tshirt"), static_cast<int>(AssetType::TShirt));
    add(regId("user.appearance.face"),   static_cast<int>(AssetType::Face));

    // Body-part overrides: prefer the database's real type (could be a Package/DynamicHead),
    // falling back to the classic part type for the slot.
    struct BodySlot { const char* key; AssetType fallback; };
    const BodySlot bodySlots[] = {
        {"user.appearance.body.package",   AssetType::Package},
        {"user.appearance.body.head",      AssetType::Head},
        {"user.appearance.body.torso",     AssetType::Torso},
        {"user.appearance.body.left_arm",  AssetType::LeftArm},
        {"user.appearance.body.right_arm", AssetType::RightArm},
        {"user.appearance.body.left_leg",  AssetType::LeftLeg},
        {"user.appearance.body.right_leg", AssetType::RightLeg},
    };
    for (const auto& slot : bodySlots) {
        int64_t id = regId(slot.key);
        if (id > 0) add(id, ResolveType(core, id, slot.fallback));
    }

    // Animations: each per-state id is both worn as an asset of its animation type AND listed in the
    // animationAssetIds map keyed by state name (the engine reads the map).
    struct AnimSlot { const char* key; const char* name; AssetType type; };
    const AnimSlot animSlots[] = {
        {"user.appearance.animation.climb", "climb", AssetType::ClimbAnimation},
        {"user.appearance.animation.fall",  "fall",  AssetType::FallAnimation},
        {"user.appearance.animation.idle",  "idle",  AssetType::IdleAnimation},
        {"user.appearance.animation.jump",  "jump",  AssetType::JumpAnimation},
        {"user.appearance.animation.run",   "run",   AssetType::RunAnimation},
        {"user.appearance.animation.swim",  "swim",  AssetType::SwimAnimation},
        {"user.appearance.animation.walk",  "walk",  AssetType::WalkAnimation},
    };
    for (const auto& slot : animSlots) {
        int64_t animId = regId(slot.key);
        if (animId > 0) {
            add(animId, static_cast<int>(slot.type));
            a.Animations[slot.name] = animId;
        }
    }

    // Accessories: a Lua array of asset ids whose types are resolved from the databases.
    if (auto table = reg->GetKeyValue<sol::table>("user.appearance.accessories"); table.has_value()) {
        const std::size_t count = table->size();
        for (std::size_t i = 1; i <= count; ++i) {
            sol::object obj = (*table)[i];
            if (obj.get_type() != sol::type::number)
                continue;
            int64_t id = obj.as<int64_t>();
            add(id, ResolveType(core, id, AssetType::Hat));
        }
    }

    // Body colors (BrickColor names, resolved to exact id+hex when formatted).
    auto colorName = [&](const char* key, const char* def) -> std::string {
        return reg->GetKeyValue<std::string>(key).value_or(def);
    };
    a.HeadColor     = colorName("user.appearance.color.head",      "Bright yellow");
    a.TorsoColor    = colorName("user.appearance.color.torso",     "Bright blue");
    a.RightArmColor = colorName("user.appearance.color.right_arm", "Bright yellow");
    a.LeftArmColor  = colorName("user.appearance.color.left_arm",  "Bright yellow");
    a.RightLegColor = colorName("user.appearance.color.right_leg", "Br. yellowish green");
    a.LeftLegColor  = colorName("user.appearance.color.left_leg",  "Br. yellowish green");

    // Scales. The registry stores 0 as "unset"; the multiplicative scales (height/width/head/depth)
    // must be 1.0 in that case or the character renders zero-sized. Proportion/bodyType are additive
    // 0..1 morphs where 0 is the correct default.
    auto multScale = [&](const char* key) -> double {
        double v = reg->GetKeyValue<double>(key).value_or(0.0);
        return v > 0.0 ? v : 1.0;
    };
    a.Height     = multScale("user.appearance.scale.height");
    a.Width      = multScale("user.appearance.scale.width");
    a.Head       = multScale("user.appearance.scale.head");
    a.Depth      = multScale("user.appearance.scale.depth");
    a.Proportion = reg->GetKeyValue<double>("user.appearance.scale.proportion").value_or(0.0);
    a.BodyType   = reg->GetKeyValue<double>("user.appearance.scale.body_type").value_or(0.0);

    a.AvatarType = reg->GetKeyValue<std::string>("user.appearance.avatar_type").value_or("R6");
    if (a.AvatarType != "R6" && a.AvatarType != "R15")
        a.AvatarType = "R6";
    return a;
}

// The integer-id form (classic `bodyColors`): BrickColor palette numbers only. Kept as its own clean
// object so a strict engine-side parser never sees an unexpected field type.
nlohmann::json BodyColorIdsJson(const Appearance& a) {
    nlohmann::json j;
    j["headColorId"]    = BrickColorIdForName(a.HeadColor);
    j["torsoColorId"]   = BrickColorIdForName(a.TorsoColor);
    j["rightArmColorId"]= BrickColorIdForName(a.RightArmColor);
    j["leftArmColorId"] = BrickColorIdForName(a.LeftArmColor);
    j["rightLegColorId"]= BrickColorIdForName(a.RightLegColor);
    j["leftLegColorId"] = BrickColorIdForName(a.LeftLegColor);
    return j;
}

// The hex form (`bodyColor3s`): six-digit RRGGBB strings, no leading '#', matching Roblox. Separate
// object from the id form (Roblox never mixes the two).
nlohmann::json BodyColor3sJson(const Appearance& a) {
    auto stripHash = [](const std::string& hex) {
        return (!hex.empty() && hex.front() == '#') ? hex.substr(1) : hex;
    };
    nlohmann::json j;
    j["headColor3"]     = stripHash(HexForBrickName(a.HeadColor));
    j["torsoColor3"]    = stripHash(HexForBrickName(a.TorsoColor));
    j["rightArmColor3"] = stripHash(HexForBrickName(a.RightArmColor));
    j["leftArmColor3"]  = stripHash(HexForBrickName(a.LeftArmColor));
    j["rightLegColor3"] = stripHash(HexForBrickName(a.RightLegColor));
    j["leftLegColor3"]  = stripHash(HexForBrickName(a.LeftLegColor));
    return j;
}

nlohmann::json ScalesJson(const Appearance& a) {
    nlohmann::json j;
    j["height"]     = a.Height;
    j["width"]      = a.Width;
    j["head"]       = a.Head;
    j["depth"]      = a.Depth;
    j["proportion"] = a.Proportion;
    j["bodyType"]   = a.BodyType;
    return j;
}

nlohmann::json DefaultEmotesJson() {
    nlohmann::json emotes = nlohmann::json::array();
    const std::tuple<int64_t, const char*, int> defaults[] = {
        {3576686446, "Hello",   1},
        {3360686498, "Stadium", 2},
        {3576823880, "Point2",  3},
        {3576968026, "Shrug",   4},
    };
    for (const auto& [id, name, pos] : defaults) {
        nlohmann::json e;
        e["assetId"] = id;
        e["assetName"] = name;
        e["position"] = pos;
        emotes.push_back(e);
    }
    return emotes;
}

nlohmann::json BuildFetchJson(const Appearance& a) {
    nlohmann::json j;
    j["resolvedAvatarType"] = a.AvatarType;
    j["equippedGearVersionIds"] = nlohmann::json::array();
    j["backpackGearVersionIds"] = nlohmann::json::array();

    nlohmann::json assetIds = nlohmann::json::array();
    for (const WornAsset& w : a.Worn) {
        nlohmann::json entry;
        entry["assetId"] = w.Id;
        entry["assetTypeId"] = w.TypeId;
        assetIds.push_back(entry);
    }
    j["assetAndAssetTypeIds"] = assetIds;

    nlohmann::json anims = nlohmann::json::object();
    for (const auto& [name, id] : a.Animations)
        anims[name] = id;
    j["animationAssetIds"] = anims;

    j["bodyColors"] = BodyColorIdsJson(a);
    j["bodyColor3s"] = BodyColor3sJson(a);
    j["scales"] = ScalesJson(a);
    j["emotes"] = nlohmann::json::array();
    return j;
}

// Classic default character (yellow head/arms, blue torso, green legs), nothing worn.
Appearance MakeDefaultAppearance() {
    Appearance a{};
    a.HeadColor = a.RightArmColor = a.LeftArmColor = "Bright yellow";
    a.TorsoColor = "Bright blue";
    a.RightLegColor = a.LeftLegColor = "Br. yellowish green";
    a.Height = a.Width = a.Head = a.Depth = 1.0;
    return a;
}

Appearance MakeGuestAppearance() {
    Appearance a = MakeDefaultAppearance();
    a.HeadColor = a.RightArmColor = a.LeftArmColor = "Institutional white";
    a.TorsoColor = a.RightLegColor = a.LeftLegColor = "Really black";
    return a;
}

// An authenticated user's stored character, read from the master database. Falls through to the
// classic default when the user has no stored appearance (never the local registry).
Appearance ReadAppearanceFromDb(Core* core, int64_t userId) {
    Appearance a = MakeDefaultAppearance();
    EmuDb* db = core->GetEmuDbManager()->GetMasterDatabase();
    if (db == nullptr || db->Fail())
        return a;

    Statement items = db->PrepareStatement("SELECT AssetId FROM UserCharacterItem WHERE Id = ?;");
    items.Bind(1, userId);
    while (items.Step() == SQLITE_ROW) {
        int64_t assetId = items.GetInt64FromColumnIndex(0);
        if (assetId > 0)
            a.Worn.push_back({ assetId, ResolveType(core, assetId, AssetType::Hat) });
    }

    Statement colors = db->PrepareStatement("SELECT BodyPart, Color3 FROM UserCharacterBodyColor WHERE Id = ?;");
    colors.Bind(1, userId);
    while (colors.Step() == SQLITE_ROW) {
        int packed = static_cast<int>(colors.GetInt64FromColumnIndex(1));
        Roblox::Color3 c = Roblox::Color3::fromRGB((packed >> 16) & 0xFF, (packed >> 8) & 0xFF, packed & 0xFF);
        std::string name = Roblox::BrickColor(c).Name();
        switch (static_cast<UserCharacterBodyPart>(colors.GetIntFromColumnIndex(0))) {
        case UserCharacterBodyPart::Head:     a.HeadColor = name; break;
        case UserCharacterBodyPart::Torso:    a.TorsoColor = name; break;
        case UserCharacterBodyPart::RightArm: a.RightArmColor = name; break;
        case UserCharacterBodyPart::LeftArm:  a.LeftArmColor = name; break;
        case UserCharacterBodyPart::RightLeg: a.RightLegColor = name; break;
        case UserCharacterBodyPart::LeftLeg:  a.LeftLegColor = name; break;
        }
    }

    Statement scale = db->PrepareStatement(
        "SELECT CharacterBodyType, CharacterWidth, CharacterHeight, CharacterHead, CharacterProportions FROM User WHERE Id = ?;");
    scale.Bind(1, userId);
    if (scale.Step() == SQLITE_ROW) {
        auto mult = [&](int col) { double v = scale.GetDoubleFromColumnIndex(col); return v > 0.0 ? v : 1.0; };
        if (!scale.IsColumnIndexNull(0)) a.BodyType   = scale.GetDoubleFromColumnIndex(0);
        if (!scale.IsColumnIndexNull(1)) a.Width      = mult(1);
        if (!scale.IsColumnIndexNull(2)) a.Height     = mult(2);
        if (!scale.IsColumnIndexNull(3)) a.Head       = mult(3);
        if (!scale.IsColumnIndexNull(4)) a.Proportion = scale.GetDoubleFromColumnIndex(4);
    }
    return a;
}

}

nlohmann::json AvatarAppearance::BuildAvatarJson(Core* core) {
    Appearance a = ReadAppearance(core);

    nlohmann::json j;
    j["scales"] = ScalesJson(a);
    j["playerAvatarType"] = a.AvatarType;

    j["bodyColors"] = BodyColorIdsJson(a);
    j["bodyColor3s"] = BodyColor3sJson(a);

    nlohmann::json assets = nlohmann::json::array();
    for (const WornAsset& w : a.Worn) {
        nlohmann::json asset;
        asset["id"] = w.Id;
        asset["name"] = "";
        asset["currentVersionId"] = w.Id;
        asset["assetType"]["id"] = w.TypeId;
        asset["assetType"]["name"] = Roblox::AssetTypeAsTranslatableString(static_cast<AssetType>(w.TypeId));
        assets.push_back(asset);
    }
    j["assets"] = assets;

    j["defaultShirtApplied"] = !a.HasShirt;
    j["defaultPantsApplied"] = !a.HasPants;
    j["emotes"] = DefaultEmotesJson();
    return j;
}

nlohmann::json AvatarAppearance::BuildAvatarFetchJson(Core* core) {
    return BuildFetchJson(ReadAppearance(core));
}

nlohmann::json AvatarAppearance::BuildAvatarFetchJsonForUser(Core* core, int64_t userId) {
    return BuildFetchJson(ReadAppearanceFromDb(core, userId));
}

nlohmann::json AvatarAppearance::BuildGuestAvatarFetchJson() {
    return BuildFetchJson(MakeGuestAppearance());
}
