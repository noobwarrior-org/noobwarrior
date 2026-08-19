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
// File: Vector2.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.Vector2 (MaximumADHD/Roblox-File-Format).
#pragma once

#include <cmath>

namespace NoobWarrior::Roblox::DataTypes {
struct Vector2 {
    float X {};
    float Y {};

    constexpr Vector2() = default;
    constexpr Vector2(float x, float y) : X(x), Y(y) {}

    float Magnitude() const { return std::sqrt(X * X + Y * Y); }
    Vector2 Unit() const {
        const float magnitude = Magnitude();
        return magnitude == 0.0f ? Vector2() : Vector2(X / magnitude, Y / magnitude);
    }

    friend constexpr bool operator==(const Vector2 &, const Vector2 &) = default;
    constexpr Vector2 operator+(const Vector2 &o) const { return {X + o.X, Y + o.Y}; }
    constexpr Vector2 operator-(const Vector2 &o) const { return {X - o.X, Y - o.Y}; }
    constexpr Vector2 operator*(float s) const { return {X * s, Y * s}; }
};
}
