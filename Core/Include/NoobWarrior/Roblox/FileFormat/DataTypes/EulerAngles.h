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
// File: EulerAngles.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.EulerAngles (MaximumADHD/Roblox-File-Format).
#pragma once

namespace NoobWarrior::Roblox::DataTypes {
struct EulerAngles {
    float Yaw {};
    float Pitch {};
    float Roll {};

    constexpr EulerAngles() = default;
    constexpr EulerAngles(float yaw, float pitch, float roll) :
        Yaw(yaw), Pitch(pitch), Roll(roll) {}

    friend constexpr bool operator==(const EulerAngles &, const EulerAngles &) = default;
};
}
