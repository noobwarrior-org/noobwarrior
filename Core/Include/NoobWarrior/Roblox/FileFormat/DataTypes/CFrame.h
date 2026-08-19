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
// File: CFrame.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.CFrame (MaximumADHD/Roblox-File-Format).
#pragma once

#include <NoobWarrior/Roblox/FileFormat/DataTypes/Quaternion.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/Vector3.h>

#include <array>

namespace NoobWarrior::Roblox::DataTypes {
// Components are laid out as Roblox serializes them: position, then the 3x3 rotation matrix in
// row-major order (R00..R22).
struct CFrame {
    std::array<float, 12> Components {{0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1}};

    constexpr CFrame() = default;
    explicit constexpr CFrame(const std::array<float, 12> &components) : Components(components) {}
    constexpr CFrame(Vector3 position) {
        Components = {position.X, position.Y, position.Z, 1, 0, 0, 0, 1, 0, 0, 0, 1};
    }

    constexpr Vector3 Position() const { return {Components[0], Components[1], Components[2]}; }
    constexpr Vector3 XVector() const { return {Components[3], Components[6], Components[9]}; }
    constexpr Vector3 YVector() const { return {Components[4], Components[7], Components[10]}; }
    constexpr Vector3 ZVector() const { return {Components[5], Components[8], Components[11]}; }

    constexpr bool IsAxisAligned() const {
        return XVector().ToNormalId() != NormalId::Unknown &&
               YVector().ToNormalId() != NormalId::Unknown &&
               ZVector().ToNormalId() != NormalId::Unknown;
    }

    // Roblox packs axis-aligned rotations into a single byte. Anything else serializes the full
    // 3x3 matrix, which -1 signals.
    constexpr int GetOrientId() const {
        if (!IsAxisAligned())
            return -1;
        const int x = static_cast<int>(XVector().ToNormalId());
        const int y = static_cast<int>(YVector().ToNormalId());
        const int orientId = (6 * x) + y;
        return (x % 3) == (y % 3) ? -1 : orientId;
    }

    static constexpr CFrame FromOrientId(int orientId) {
        const Vector3 r0 = Vector3::FromNormalId(static_cast<NormalId>(orientId / 6));
        const Vector3 r1 = Vector3::FromNormalId(static_cast<NormalId>(orientId % 6));
        const Vector3 r2 = r0.Cross(r1);
        return CFrame(std::array<float, 12> {
            0, 0, 0,
            r0.X, r1.X, r2.X,
            r0.Y, r1.Y, r2.Y,
            r0.Z, r1.Z, r2.Z,
        });
    }

    friend constexpr bool operator==(const CFrame &, const CFrame &) = default;
};
}
