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
// File: StudioServerPlaceCacheTest.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Prepared Studio server place cache and atomic publication tests.
#include <NoobWarrior/Roblox/FileFormat/StudioServerPlaceCache.h>
#include <NoobWarrior/Roblox/FileFormat/StudioServerPlace.h>
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {
class TemporaryCacheDirectory {
public:
    TemporaryCacheDirectory() {
        Path = std::filesystem::temp_directory_path() /
            ("noobwarrior-studio-cache-test-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(Path);
    }

    ~TemporaryCacheDirectory() {
        std::error_code code;
        std::filesystem::remove_all(Path, code);
    }

    std::filesystem::path Path;
};

std::vector<unsigned char> MinimalXmlPlace(std::string_view marker = {}) {
    const std::string xml = "<?xml version=\"1.0\"?><roblox version=\"4\"><Meta name=\"Marker\">" +
                            std::string(marker) + "</Meta></roblox>";
    return {xml.begin(), xml.end()};
}

std::vector<unsigned char> ReadFile(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
} // namespace

TEST(StudioServerPlaceCache, KeyIncludesEveryBootstrapFieldAndSchema) {
    NoobWarrior::StudioServerBootstrap bootstrap;
    bootstrap.Plans = {"[{\"name\":\"A\"}]"};
    bootstrap.Scripts.push_back({"One", "Script", "return 1", "ServerScriptService", true});
    bootstrap.Scripts.push_back({"Two", "LocalScript", "return 2", "StarterPlayerScripts", false});
    bootstrap.Models.push_back({
        "/models/ModelOne.rbxm", {{"ServerScriptService", "ServerScriptService"}},
        {0x01, 0x02, 0x03}, "RenamedRoot",
    });
    bootstrap.Models.push_back({
        "/models/ModelTwo.rbxm", {{"ReplicatedStorage", "ReplicatedStorage"}},
        {0x04, 0x05}, {},
    });

    const std::string base =
        NoobWarrior::ComputeStudioServerPlaceCacheKey(
            "source-sha256", bootstrap, "version-719|0.719.0.7191339");
    ASSERT_EQ(64u, base.size());
    auto expectChanged = [&](const NoobWarrior::StudioServerBootstrap &changed) {
        EXPECT_NE(base, NoobWarrior::ComputeStudioServerPlaceCacheKey(
                            "source-sha256", changed,
                            "version-719|0.719.0.7191339"));
    };

    auto changed = bootstrap;
    changed.Plans.push_back("[]");
    expectChanged(changed);
    changed = bootstrap;
    changed.Plans[0].push_back('x');
    expectChanged(changed);
    changed = bootstrap;
    changed.Scripts[0].Key.push_back('x');
    expectChanged(changed);
    changed = bootstrap;
    changed.Scripts[0].ClassName = "ModuleScript";
    expectChanged(changed);
    changed = bootstrap;
    changed.Scripts[0].Source.push_back('x');
    expectChanged(changed);
    changed = bootstrap;
    changed.Scripts[0].ParentClassName = "StarterPlayerScripts";
    expectChanged(changed);
    changed = bootstrap;
    changed.Scripts[0].Disabled = false;
    expectChanged(changed);
    changed = bootstrap;
    std::swap(changed.Scripts[0], changed.Scripts[1]);
    expectChanged(changed);
    changed = bootstrap;
    changed.Models[0].SourcePath.push_back('x');
    expectChanged(changed);
    changed = bootstrap;
    changed.Models[0].ParentPath[0].Name.push_back('x');
    expectChanged(changed);
    changed = bootstrap;
    changed.Models[0].ParentPath[0].ClassName.push_back('x');
    expectChanged(changed);
    changed = bootstrap;
    changed.Models[0].SingleRootName.push_back('x');
    expectChanged(changed);
    changed = bootstrap;
    changed.Models[0].Data.push_back(0x04);
    expectChanged(changed);
    changed = bootstrap;
    std::swap(changed.Models[0], changed.Models[1]);
    expectChanged(changed);

    EXPECT_NE(base, NoobWarrior::ComputeStudioServerPlaceCacheKey(
                        "other-source", bootstrap,
                        "version-719|0.719.0.7191339"));
    EXPECT_NE(base, NoobWarrior::ComputeStudioServerPlaceCacheKey(
                        "source-sha256", bootstrap, "version-719|0.719.0.7191339",
                        NoobWarrior::kStudioServerPlaceCacheSchema + 1));
    EXPECT_NE(base, NoobWarrior::ComputeStudioServerPlaceCacheKey(
                        "source-sha256", bootstrap,
                        "version-574|0.574.0.5740446"));
}

TEST(StudioServerPlaceCache, StoresAndPublishesImmutableEntry) {
    TemporaryCacheDirectory temporary;
    const std::filesystem::path cache = temporary.Path / "cache";
    const std::filesystem::path launch = temporary.Path / "Roblox" / "server.rbxl";
    NoobWarrior::StudioServerBootstrap bootstrap;
    bootstrap.Plans = {"[{\"name\":\"C\"}]"};
    const std::string key =
        NoobWarrior::ComputeStudioServerPlaceCacheKey(
            "source-sha256", bootstrap, "studio");
    const std::vector<unsigned char> place = MinimalXmlPlace("cached");

    std::string error;
    uint64_t bytes = 0;
    EXPECT_EQ(NoobWarrior::StudioServerPlaceCacheResponse::Miss,
              NoobWarrior::PublishCachedStudioServerPlace(
                  cache, key, launch, &bytes, &error));
    ASSERT_TRUE(NoobWarrior::StoreStudioServerPlaceCache(cache, key, place, &error)) << error;
    const std::filesystem::path entry =
        NoobWarrior::StudioServerPlaceCacheEntry(cache, key);
    ASSERT_TRUE(std::filesystem::exists(entry));
    const auto oldCacheTime = std::filesystem::file_time_type::clock::now() -
        std::chrono::hours(24);
    std::filesystem::last_write_time(entry, oldCacheTime);

    ASSERT_EQ(NoobWarrior::StudioServerPlaceCacheResponse::Hit,
              NoobWarrior::PublishCachedStudioServerPlace(
                  cache, key, launch, &bytes, &error)) << error;
    EXPECT_EQ(place.size(), bytes);
    EXPECT_EQ(place, ReadFile(launch));

    const std::vector<unsigned char> replacement = MinimalXmlPlace("modified launch file");
    ASSERT_TRUE(NoobWarrior::PublishStudioServerPlace(launch, replacement, &error)) << error;
    ASSERT_EQ(NoobWarrior::StudioServerPlaceCacheResponse::Hit,
              NoobWarrior::PublishCachedStudioServerPlace(
                  cache, key, launch, &bytes, &error)) << error;
    EXPECT_EQ(place, ReadFile(launch));
    EXPECT_GT(std::filesystem::last_write_time(entry), oldCacheTime);

    for (const auto &item : std::filesystem::recursive_directory_iterator(temporary.Path))
        EXPECT_EQ(std::string::npos, item.path().filename().string().find(".tmp-"));
}

TEST(StudioServerPlaceCache, RejectsInvalidOrIncompleteEntries) {
    TemporaryCacheDirectory temporary;
    const std::filesystem::path cache = temporary.Path / "cache";
    const std::filesystem::path launch = temporary.Path / "server.rbxl";
    NoobWarrior::StudioServerBootstrap bootstrap;
    const std::string key =
        NoobWarrior::ComputeStudioServerPlaceCacheKey(
            "source-sha256", bootstrap, "studio");
    std::filesystem::create_directories(cache);
    {
        std::ofstream stream(NoobWarrior::StudioServerPlaceCacheEntry(cache, key),
                             std::ios::binary);
        stream << "not a Roblox place";
    }

    std::string error;
    EXPECT_EQ(NoobWarrior::StudioServerPlaceCacheResponse::Miss,
              NoobWarrior::PublishCachedStudioServerPlace(
                  cache, key, launch, nullptr, &error));
    EXPECT_FALSE(std::filesystem::exists(launch));
}

TEST(StudioServerPlaceCache, SourceHashMatchesSha256Identity) {
    const std::vector<unsigned char> place = MinimalXmlPlace("hash me");
    const std::string hash = NoobWarrior::ComputeStudioServerPlaceSourceHash(place);
    EXPECT_EQ(64u, hash.size());
    EXPECT_NE(hash, NoobWarrior::ComputeStudioServerPlaceSourceHash(
                        MinimalXmlPlace("different")));
}

TEST(StudioServerPlaceCache, EmptyBootstrapStillEnablesHttpService) {
    TemporaryCacheDirectory temporary;
    const std::filesystem::path launch = temporary.Path / "server.rbxl";
    const std::vector<unsigned char> source = MinimalXmlPlace("http-enabled");
    const std::string sourceHash =
        NoobWarrior::ComputeStudioServerPlaceSourceHash(source);
    NoobWarrior::StudioServerBootstrap bootstrap;

    std::string error;
    ASSERT_EQ(NoobWarrior::StudioServerPlacePreparationResponse::Built,
              NoobWarrior::PrepareStudioServerPlace(
                  sourceHash, bootstrap, "studio", temporary.Path / "cache", launch,
                  [&](std::vector<unsigned char> &place, std::string *) {
                      place = source;
                      return true;
                  }, nullptr, &error)) << error;

    std::unique_ptr<NoobWarrior::Roblox::RobloxFile> reopened;
    ASSERT_EQ(NoobWarrior::Roblox::FileResponse::Success,
              NoobWarrior::Roblox::RobloxFile::Open(reopened, ReadFile(launch)));
    NoobWarrior::Roblox::Instance *httpService = nullptr;
    for (NoobWarrior::Roblox::Instance *instance : reopened->GetDescendants()) {
        if (instance->ClassName == "HttpService") {
            httpService = instance;
            break;
        }
    }
    ASSERT_NE(nullptr, httpService);
    EXPECT_TRUE(httpService->GetPropertyValue<bool>("HttpEnabled"));
}

TEST(StudioServerPlaceCache, PreparationSkipsLoaderOnWarmHit) {
    TemporaryCacheDirectory temporary;
    const std::filesystem::path cache = temporary.Path / "cache";
    const std::filesystem::path launch = temporary.Path / "Roblox" / "server.rbxl";
    const std::vector<unsigned char> source = MinimalXmlPlace("source");
    const std::string sourceHash =
        NoobWarrior::ComputeStudioServerPlaceSourceHash(source);
    NoobWarrior::StudioServerBootstrap bootstrap;
    bootstrap.Plans = {"[{\"name\":\"A\"}]"};
    std::atomic_int loads {0};
    auto loader = [&](std::vector<unsigned char> &place, std::string *) {
        ++loads;
        place = source;
        return true;
    };

    std::string error;
    uint64_t bytes = 0;
    EXPECT_EQ(NoobWarrior::StudioServerPlacePreparationResponse::Built,
              NoobWarrior::PrepareStudioServerPlace(
                  sourceHash, bootstrap, "studio-719", cache, launch,
                  loader, &bytes, &error)) << error;
    EXPECT_EQ(1, loads.load());
    ASSERT_GT(bytes, source.size());

    EXPECT_EQ(NoobWarrior::StudioServerPlacePreparationResponse::CacheHit,
              NoobWarrior::PrepareStudioServerPlace(
                  sourceHash, bootstrap, "studio-719", cache, launch,
                  loader, &bytes, &error)) << error;
    EXPECT_EQ(1, loads.load());

    std::unique_ptr<NoobWarrior::Roblox::RobloxFile> reopened;
    EXPECT_EQ(NoobWarrior::Roblox::FileResponse::Success,
              NoobWarrior::Roblox::RobloxFile::Open(reopened, ReadFile(launch)));
}

TEST(StudioServerPlaceCache, TruncatedEntryIsRebuilt) {
    TemporaryCacheDirectory temporary;
    const std::filesystem::path cache = temporary.Path / "cache";
    const std::filesystem::path launch = temporary.Path / "server.rbxl";
    const std::vector<unsigned char> source = MinimalXmlPlace("source");
    const std::string sourceHash =
        NoobWarrior::ComputeStudioServerPlaceSourceHash(source);
    NoobWarrior::StudioServerBootstrap bootstrap;
    bootstrap.Plans = {"[{\"name\":\"A\"}]"};
    int loads = 0;
    auto loader = [&](std::vector<unsigned char> &place, std::string *) {
        ++loads;
        place = source;
        return true;
    };

    std::string error;
    ASSERT_EQ(NoobWarrior::StudioServerPlacePreparationResponse::Built,
              NoobWarrior::PrepareStudioServerPlace(
                  sourceHash, bootstrap, "studio", cache, launch,
                  loader, nullptr, &error)) << error;
    const std::string key = NoobWarrior::ComputeStudioServerPlaceCacheKey(
        sourceHash, bootstrap, "studio");
    const std::filesystem::path entry =
        NoobWarrior::StudioServerPlaceCacheEntry(cache, key);
    const std::vector<unsigned char> prepared = ReadFile(entry);
    ASSERT_GT(prepared.size(), 32u);
    {
        std::ofstream truncated(entry, std::ios::binary | std::ios::trunc);
        truncated.write(reinterpret_cast<const char *>(prepared.data()), 24);
    }

    EXPECT_EQ(NoobWarrior::StudioServerPlacePreparationResponse::Built,
              NoobWarrior::PrepareStudioServerPlace(
                  sourceHash, bootstrap, "studio", cache, launch,
                  loader, nullptr, &error)) << error;
    EXPECT_EQ(2, loads);
    EXPECT_EQ(prepared, ReadFile(entry));
}

TEST(StudioServerPlaceCache, SourceHashMismatchPublishesNothing) {
    TemporaryCacheDirectory temporary;
    const std::filesystem::path launch = temporary.Path / "server.rbxl";
    const std::vector<unsigned char> source = MinimalXmlPlace("source");
    NoobWarrior::StudioServerBootstrap bootstrap;
    auto loader = [&](std::vector<unsigned char> &place, std::string *) {
        place = source;
        return true;
    };

    std::string error;
    EXPECT_EQ(NoobWarrior::StudioServerPlacePreparationResponse::Failed,
              NoobWarrior::PrepareStudioServerPlace(
                  std::string(64, '0'), bootstrap, "studio",
                  temporary.Path / "cache", launch, loader, nullptr, &error));
    EXPECT_NE(std::string::npos, error.find("DataHash"));
    EXPECT_FALSE(std::filesystem::exists(launch));
}

// Content addressing removes the need for a cross-process build lock: every builder of one key
// produces the same bytes, and both the cache store and the launch publication land through an
// atomic rename. A cold cache may therefore be built more than once, which is redundant work
// rather than a correctness problem.
TEST(StudioServerPlaceCache, ConcurrentPreparationConvergesOnIdenticalOutput) {
    TemporaryCacheDirectory temporary;
    const std::filesystem::path cache = temporary.Path / "cache";
    const std::filesystem::path launch = temporary.Path / "server.rbxl";
    const std::vector<unsigned char> source = MinimalXmlPlace("source");
    const std::string sourceHash =
        NoobWarrior::ComputeStudioServerPlaceSourceHash(source);
    NoobWarrior::StudioServerBootstrap bootstrap;
    bootstrap.Plans = {"[{\"name\":\"A\"}]"};
    std::atomic_int loads {0};
    auto loader = [&](std::vector<unsigned char> &place, std::string *) {
        ++loads;
        std::this_thread::yield();
        place = source;
        return true;
    };

    std::array<NoobWarrior::StudioServerPlacePreparationResponse, 4> responses {};
    std::array<std::string, 4> errors;
    std::array<std::thread, 4> threads;
    for (size_t index = 0; index < threads.size(); ++index) {
        threads[index] = std::thread([&, index] {
            responses[index] = NoobWarrior::PrepareStudioServerPlace(
                sourceHash, bootstrap, "studio", cache, launch,
                loader, nullptr, &errors[index]);
        });
    }
    for (std::thread &thread : threads)
        thread.join();

    for (size_t index = 0; index < responses.size(); ++index) {
        EXPECT_NE(NoobWarrior::StudioServerPlacePreparationResponse::Failed,
                  responses[index]) << errors[index];
    }
    EXPECT_GE(loads.load(), 1);
    EXPECT_LE(loads.load(), static_cast<int>(threads.size()));
    EXPECT_GE(std::ranges::count(
                  responses, NoobWarrior::StudioServerPlacePreparationResponse::Built), 1);

    const std::string key = NoobWarrior::ComputeStudioServerPlaceCacheKey(
        sourceHash, bootstrap, "studio");
    const std::vector<unsigned char> prepared =
        ReadFile(NoobWarrior::StudioServerPlaceCacheEntry(cache, key));
    ASSERT_FALSE(prepared.empty());
    EXPECT_EQ(prepared, ReadFile(launch));

    std::unique_ptr<NoobWarrior::Roblox::RobloxFile> reopened;
    EXPECT_EQ(NoobWarrior::Roblox::FileResponse::Success,
              NoobWarrior::Roblox::RobloxFile::Open(reopened, ReadFile(launch)));

    for (const auto &item : std::filesystem::recursive_directory_iterator(temporary.Path))
        EXPECT_EQ(std::string::npos, item.path().filename().string().find(".tmp-"));
}
