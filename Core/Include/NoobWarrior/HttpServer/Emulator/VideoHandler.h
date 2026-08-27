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
// File: VideoHandler.h
// Started by: Hattozo
// Started on: 8/26/2026
// Description: Serves the HLS playlist and segments of a video asset, from /video/v1/{assetId}/{hash}/{file}.
// This handler only sends files. Cutting the video into segments is VideoTranscoder's job.
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>

#include <cstdint>
#include <filesystem>
#include <string>

namespace NoobWarrior {
class ServerEmulator;

class VideoHandler : public Handler {
public:
    explicit VideoHandler(ServerEmulator *srv);

    void OnRequest(evhttp_request *req, void *userdata) override;

    static std::string ResolvePlaylistPath(ServerEmulator *emu, int64_t assetId, int64_t version);

    static std::string BuildPlaylistPath(int64_t assetId, const std::string &hash);
    static std::string BuildUriPrefix(int64_t assetId, const std::string &hash);

    static bool AcrRequestsHls(const char *encoded);
private:
    void ServeFile(evhttp_request *req, const std::filesystem::path &path, const std::string &fileName,
                   const std::string &uriPrefix);

    ServerEmulator *mServerEmulator;
};
}
