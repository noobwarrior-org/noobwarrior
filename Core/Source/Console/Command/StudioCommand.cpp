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
// File: StudioCommand.cpp
// Started by: Hattozo
// Started on: 5/17/2026
// Description:
#include <NoobWarrior/Console/Command/StudioCommand.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;

void StudioCommand::Main(CommandContext& ctx) {
    ctx.Reply("Launching Roblox Studio...");
    ctx.GetCore()->LaunchEngine({
        .Engine = {
            .Architecture = EngineArchitecture::x86_64,
            .Type = EngineType::Roblox,
            .Side = EngineSide::Studio,
            // .Hash = "ef266da340bc4058",
            // .Version = "0.463.0.417004"
            .Hash = "c2e4d104afaf449c",
            .Version = "0.574.0.5740446"
        },
        .Ip = "",
        .Port = std::nullopt,
        .PlaceId = std::nullopt
    });
}