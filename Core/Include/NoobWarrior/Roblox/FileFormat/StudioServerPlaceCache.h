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
// File: StudioServerPlaceCache.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Content-addressed cache and atomic publication for launch-only Studio places.
#pragma once

#include <NoobWarrior/PluginDataModel.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace NoobWarrior {
// Increment whenever mutation or bootstrap composition changes so an older prepared place is
// never reused after an upgrade merely because its logical plugin inputs stayed the same.
inline constexpr uint32_t kStudioServerPlaceCacheSchema = 8;

enum class StudioServerPlaceCacheResponse {
    Hit,
    Miss,
    Failed,
};

enum class StudioServerPlacePreparationResponse {
    CacheHit,
    Built,
    Failed,
};

using StudioServerPlaceLoader =
    std::function<bool(std::vector<unsigned char> &place, std::string *error)>;
// Called immediately before disk publication work. It must not re-enter this cache API.
using StudioServerPlacePublishCallback = std::function<void(uint64_t placeBytes)>;

// sourceSha256 is AssetData.DataHash, which identifies the original uncompressed place bytes.
std::string ComputeStudioServerPlaceCacheKey(
    std::string_view sourceSha256, const StudioServerBootstrap &bootstrap,
    std::string_view studioFingerprint,
    uint32_t schemaVersion = kStudioServerPlaceCacheSchema);

std::string ComputeStudioServerPlaceSourceHash(std::span<const unsigned char> place);

std::filesystem::path StudioServerPlaceCacheEntry(
    const std::filesystem::path &cacheDirectory, std::string_view cacheKey);

// Publishes an existing immutable cache entry to launchPath using an adjacent temporary file and
// atomic replacement. A missing entry, or one whose length disagrees with its recorded size, is
// reported as Miss so the caller rebuilds it.
StudioServerPlaceCacheResponse PublishCachedStudioServerPlace(
    const std::filesystem::path &cacheDirectory, std::string_view cacheKey,
    const std::filesystem::path &launchPath, uint64_t *placeBytes = nullptr,
    std::string *error = nullptr,
    const StudioServerPlacePublishCallback &publishCallback = {});

// Stores an immutable cache entry atomically. It does not modify the Studio launch path.
bool StoreStudioServerPlaceCache(
    const std::filesystem::path &cacheDirectory, std::string_view cacheKey,
    std::span<const unsigned char> place, std::string *error = nullptr);

// Atomically replaces launchPath with place. Concurrent publishers are serialized across
// noobWarrior processes on Windows because Studio uses one fixed server.rbxl path.
bool PublishStudioServerPlace(
    const std::filesystem::path &launchPath, std::span<const unsigned char> place,
    std::string *error = nullptr);

// Checks the cache before invoking loader. Entries are content-addressed, so concurrent builders
// of one key produce identical bytes; the loaded source is verified against sourceSha256 before it
// is mutated and cached.
StudioServerPlacePreparationResponse PrepareStudioServerPlace(
    std::string_view sourceSha256, const StudioServerBootstrap &bootstrap,
    std::string_view studioFingerprint, const std::filesystem::path &cacheDirectory,
    const std::filesystem::path &launchPath, const StudioServerPlaceLoader &loader,
    uint64_t *placeBytes = nullptr, std::string *error = nullptr,
    const StudioServerPlacePublishCallback &publishCallback = {});
}
