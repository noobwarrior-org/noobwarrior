// === noobWarrior ===
// File: AssetRequest.h
// Started by: Hattozo
// Started on: 3/5/2025
// Description:
#pragma once

#include "Roblox/Api/Asset.h"

#include <string>
#include <ostream>
#include <vector>

namespace NoobWarrior {
    enum class AssetFileNameStyle {
        Raw, // File name is retrieved from the server that is hosting the file. In this case you will get a MD5 hash, since that is how Roblox indexes files.
        AssetId,
        AssetName
    };

    enum AssetFlags {
        DA_PRESERVE_AUTHORS = 1 << 0, // Sets the Authors metadata to be the name of the Asset's creator
        DA_PRESERVE_DATECREATED = 1 << 1, // Sets the Date Created metadata to be the Asset's time of creation.
        DA_PRESERVE_DATEMODIFIED = 1 << 2, // Sets the Date Modified metadata to be the time the Asset was last updated.
        DA_FIND_ASSET_IDS_IN_MODEL = 1 << 3, // If the Asset is a Model or a Place, it parses the file and checks for any asset URLs/IDs located within scripts and any properties.
        DA_DISABLE_TEMP_DIR = 1 << 4 // Prevents noobWarrior from temporarily storing the downloaded assets in a temp directory, instead it will just always store them in the output directory from the get-go.
    };

    struct DownloadAssetArgs {
        int Flags {};
        AssetFileNameStyle FileNameStyle {};
        std::ostream *OutStream {};
        std::string OutDir {};
        std::vector<int64_t> Id {};
        std::vector<uint64_t> Version {};
    };

    /**
     * @brief Lets you download a batch of Roblox assets to a directory.
     */
    int DownloadAssets(DownloadAssetArgs args);
    // std::future<char*> DownloadAssetAsync(DownloadAssetArgs);

    int GetAssetDetails(int64_t id, Roblox::AssetDetails *details);
}