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
// File: Vector3.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.Vector3 (MaximumADHD/Roblox-File-Format).
#pragma once

#include <cmath>

namespace NoobWarrior::Roblox::DataTypes {
// NormalId ordering matches Enum.NormalId, which CFrame's orientation ids are derived from.
enum class NormalId {
    Right = 0,
    Top,
    Back,
    Left,
    Bottom,
    Front,
    Unknown = -1,
};

struct Vector3 {
    float X {};
    float Y {};
    float Z {};

    constexpr Vector3() = default;
    constexpr Vector3(float x, float y, float z) : X(x), Y(y), Z(z) {}

    float Magnitude() const { return std::sqrt(X * X + Y * Y + Z * Z); }
    Vector3 Unit() const {
        const float magnitude = Magnitude();
        return magnitude == 0.0f ? Vector3() : Vector3(X / magnitude, Y / magnitude, Z / magnitude);
    }
    constexpr float Dot(const Vector3 &o) const { return X * o.X + Y * o.Y + Z * o.Z; }
    constexpr Vector3 Cross(const Vector3 &o) const {
        return {Y * o.Z - Z * o.Y, Z * o.X - X * o.Z, X * o.Y - Y * o.X};
    }

    // Returns the axis this vector points down, or Unknown when it is not a unit axis vector.
    constexpr NormalId ToNormalId() const {
        if (X == 1.0f && Y == 0.0f && Z == 0.0f) return NormalId::Right;
        if (X == 0.0f && Y == 1.0f && Z == 0.0f) return NormalId::Top;
        if (X == 0.0f && Y == 0.0f && Z == 1.0f) return NormalId::Back;
        if (X == -1.0f && Y == 0.0f && Z == 0.0f) return NormalId::Left;
        if (X == 0.0f && Y == -1.0f && Z == 0.0f) return NormalId::Bottom;
        if (X == 0.0f && Y == 0.0f && Z == -1.0f) return NormalId::Front;
        return NormalId::Unknown;
    }

    static constexpr Vector3 FromNormalId(NormalId normal) {
        switch (normal) {
        case NormalId::Right:  return { 1.0f,  0.0f,  0.0f};
        case NormalId::Top:    return { 0.0f,  1.0f,  0.0f};
        case NormalId::Back:   return { 0.0f,  0.0f,  1.0f};
        case NormalId::Left:   return {-1.0f,  0.0f,  0.0f};
        case NormalId::Bottom: return { 0.0f, -1.0f,  0.0f};
        case NormalId::Front:  return { 0.0f,  0.0f, -1.0f};
        default:               return {};
        }
    }

    friend constexpr bool operator==(const Vector3 &, const Vector3 &) = default;
    constexpr Vector3 operator+(const Vector3 &o) const { return {X + o.X, Y + o.Y, Z + o.Z}; }
    constexpr Vector3 operator-(const Vector3 &o) const { return {X - o.X, Y - o.Y, Z - o.Z}; }
    constexpr Vector3 operator*(float s) const { return {X * s, Y * s, Z * s}; }
};
}
