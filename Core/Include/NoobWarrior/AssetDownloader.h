// === noobWarrior ===
// File: AssetDownloader.h
// Started by: Hattozo
// Started on: 3/5/2025
// Description:
#pragma once

#include <string>
#include <ostream>

namespace NoobWarrior {
    enum class AssetFileNameStyle {
        Raw, // File name is retrieved from the server that is hosting the file. In this case you will get a MD5 hash, since that is how Roblox indexes files.
        AssetId,
        AssetName
    };

    enum AssetFlags {
        DA_PRESERVE_AUTHORS = 1 << 0,
        DA_PRESERVE_DATECREATED = 1 << 1, // Sets the Date Created metadata to be the asset's time of creation.
        DA_PRESERVE_DATEMODIFIED = 1 << 2, // Sets the Date Modified metadata to be the time the asset was last updated.
        DA_FIND_ASSET_IDS_IN_MODEL = 1 << 3 // If the Asset is a Model or a Place, it parses the file and checks for any asset URLs/IDs located within scripts and any properties.
    };

    typedef struct {
        int Flags;
        AssetFileNameStyle FileNameStyle;
        std::ostream *OutStream;
        std::string OutDir;
        std::vector<int64_t> Id;
        std::vector<uint64_t> Version;
    } DownloadAssetArgs;

    /**
     * Dead simple function that lets you download Roblox assets by passing a struct with a C++ vector array containing ID's
     */
    int DownloadAssets(DownloadAssetArgs args);
    // std::future<char*> DownloadAssetAsync(DownloadAssetArgs);
}