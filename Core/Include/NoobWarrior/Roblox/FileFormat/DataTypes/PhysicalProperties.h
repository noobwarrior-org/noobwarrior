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
// File: PhysicalProperties.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.PhysicalProperties (MaximumADHD/Roblox-File-Format).
#pragma once

#include <cstdint>

namespace NoobWarrior::Roblox::DataTypes {
struct PhysicalProperties {
    bool CustomPhysics {};
    // Raw MaterialBitFlags byte (see Utility/MaterialInfo.h). Preserved so a re-encode matches the source file; upstream
    // RobloxFiles drops it, which silently discards AcousticAbsorption on round-trip.
    uint8_t Flags {};
    float Density {};
    float Friction {};
    float Elasticity {};
    float FrictionWeight {};
    float ElasticityWeight {};
    float AcousticAbsorption {1.0f};

    constexpr PhysicalProperties() = default;
    constexpr PhysicalProperties(float density, float friction, float elasticity,
                                 float frictionWeight, float elasticityWeight) :
        CustomPhysics(true), Density(density), Friction(friction), Elasticity(elasticity),
        FrictionWeight(frictionWeight), ElasticityWeight(elasticityWeight) {}

    friend constexpr bool operator==(const PhysicalProperties &,
                                     const PhysicalProperties &) = default;
};
}
