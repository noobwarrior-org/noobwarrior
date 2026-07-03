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
// File: AvatarBackend.h
// Started by: Hattozo
// Started on: 7/3/2026
// Description: Backend interface for avatar editor
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace NoobWarrior {
class Core;

struct AvatarData {
    std::map<std::string, std::string> Colors;
    std::map<std::string, double> Scales;
    std::string AvatarType { "R6" };
    std::map<std::string, int64_t> Slots;
    std::vector<int64_t> Accessories;
};

struct AvatarCatalogItem {
    int64_t Id { 0 };
    std::string Name;
    int AssetType { 0 };
};

struct AvatarCatalogPage {
    std::vector<AvatarCatalogItem> Items;
    int Page { 0 };
    int PageCount { 1 };
};

class AvatarBackend {
public:
    virtual ~AvatarBackend() = default;
    virtual bool Load(AvatarData& out) = 0;
    virtual bool Save(const AvatarData& data) = 0;
    virtual AvatarCatalogPage Catalog(int assetType, const std::string& search, int page) = 0;
    // An asset's preview image bytes (PNG/JPEG/GIF), or empty if unavailable. Used to render a catalog
    // item the client has no local database copy of.
    virtual std::vector<unsigned char> Thumbnail(int64_t assetId) = 0;
    virtual std::string Describe() = 0;
};

struct AvatarSlotDef { std::string RegKey; int AssetType; };
const std::vector<AvatarSlotDef>& AvatarSlots();
const std::vector<std::string>& AvatarColorParts();
const std::vector<std::string>& AvatarScaleKeys();
bool IsAvatarAccessoryType(int assetType);

class LocalRegistryBackend : public AvatarBackend {
public:
    explicit LocalRegistryBackend(Core* core);
    bool Load(AvatarData& out) override;
    bool Save(const AvatarData& data) override;
    AvatarCatalogPage Catalog(int assetType, const std::string& search, int page) override;
    std::vector<unsigned char> Thumbnail(int64_t assetId) override;
    std::string Describe() override;
private:
    Core* mCore;
};

class RemoteAccountBackend : public AvatarBackend {
public:
    RemoteAccountBackend(std::string baseUrl, std::string session, std::string label);
    bool Load(AvatarData& out) override;
    bool Save(const AvatarData& data) override;
    AvatarCatalogPage Catalog(int assetType, const std::string& search, int page) override;
    std::vector<unsigned char> Thumbnail(int64_t assetId) override;
    std::string Describe() override;
private:
    std::string mBaseUrl;
    std::string mSession;
    std::string mLabel;
};
}
