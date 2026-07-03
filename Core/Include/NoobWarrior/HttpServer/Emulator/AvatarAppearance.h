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
// File: AvatarAppearance.h
// Started by: Hattozo
// Started on: 6/18/2026
// Description: Builds the avatar JSON the engine consumes (the /v1/avatar and /v1.1/avatar-fetch
//              endpoints) from the local player's appearance stored in the registry (user.appearance.*),
//              resolving each worn asset's type from the mounted databases.
#pragma once
#include <nlohmann/json_fwd.hpp>

namespace NoobWarrior {
class Core;
namespace AvatarAppearance {
    // The body of /v1/avatar: the authenticated user's own avatar (used by the avatar editor and to
    // load the local character).
    nlohmann::json BuildAvatarJson(Core* core);

    // The body of /v1.1/avatar-fetch (a.k.a. /v2/avatar/avatar-fetch): the appearance the engine
    // applies when spawning a player's character. Built from the local registry appearance.
    nlohmann::json BuildAvatarFetchJson(Core* core);

    // avatar-fetch for an authenticated user, built from their DB-stored character (auth mode); the
    // local registry appearance is never consulted. Falls back to a default avatar if the user has no
    // stored character.
    nlohmann::json BuildAvatarFetchJsonForUser(Core* core, int64_t userId);

    // avatar-fetch for a guest: a plain black/white placeholder character.
    nlohmann::json BuildGuestAvatarFetchJson();
}
}
