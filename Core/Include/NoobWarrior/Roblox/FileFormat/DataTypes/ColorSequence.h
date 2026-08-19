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
// File: ColorSequence.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.ColorSequence (MaximumADHD/Roblox-File-Format).
#pragma once

#include <NoobWarrior/Roblox/FileFormat/DataTypes/ColorSequenceKeypoint.h>

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace NoobWarrior::Roblox::DataTypes {
struct ColorSequence {
    // The reference rejects anything outside 2..20 keypoints (ColorSequence.cs:64-87). Reported
    // rather than thrown, per house style.
    enum class Validity {
        Ok,
        TooFewKeypoints,
        TooManyKeypoints,
        UnorderedTimes,
        DoesNotStartAtZero,
        DoesNotEndAtOne,
    };

    static constexpr size_t kMinKeypoints = 2;
    static constexpr size_t kMaxKeypoints = 20;

    std::vector<ColorSequenceKeypoint> Keypoints;

    // ColorSequence.cs has no parameterless constructor: a sequence with no keypoints is not a
    // shape the format admits, and a zero-keypoint column is one an engine may refuse to load.
    // Default to what ColorSequence(Color3) builds so a defaulted property is still valid.
    ColorSequence() : ColorSequence(Color3()) {}

    // ColorSequence.cs:20-31.
    ColorSequence(float r, float g, float b) : ColorSequence(Color3(r, g, b)) {}
    explicit ColorSequence(Color3 c) : ColorSequence(c, c) {}
    ColorSequence(Color3 c0, Color3 c1) :
        Keypoints {ColorSequenceKeypoint(0.0f, c0), ColorSequenceKeypoint(1.0f, c1)} {}

    // Deliberately unvalidated: PROP::Read has to be able to hold whatever a file actually
    // contains, however malformed. Prefer Create for keypoints that did not come from a file.
    explicit ColorSequence(std::vector<ColorSequenceKeypoint> keypoints) :
        Keypoints(std::move(keypoints)) {}

    static Validity Validate(const std::vector<ColorSequenceKeypoint> &keypoints) {
        if (keypoints.size() < kMinKeypoints)
            return Validity::TooFewKeypoints;
        if (keypoints.size() > kMaxKeypoints)
            return Validity::TooManyKeypoints;
        for (size_t key = 1; key < keypoints.size(); ++key)
            if (keypoints[key - 1].Time > keypoints[key].Time)
                return Validity::UnorderedTimes;
        // The reference compares the endpoint times with FuzzyEquals (Formatting.cs:105), whose
        // epsilon is 1e-4. Spelled out here so a DataTypes header stays free of the pugixml
        // dependency Utility/Formatting.h carries.
        constexpr float kTimeEpsilon = 1e-4f;
        if (std::fabs(keypoints.front().Time) >= kTimeEpsilon)
            return Validity::DoesNotStartAtZero;
        if (std::fabs(keypoints.back().Time - 1.0f) >= kTimeEpsilon)
            return Validity::DoesNotEndAtOne;
        return Validity::Ok;
    }

    // Stands in for the throwing ColorSequence(ColorSequenceKeypoint[]) constructor.
    static Validity Create(std::vector<ColorSequenceKeypoint> keypoints, ColorSequence &result) {
        const Validity validity = Validate(keypoints);
        if (validity != Validity::Ok)
            return validity;
        result = ColorSequence(std::move(keypoints));
        return Validity::Ok;
    }

    friend bool operator==(const ColorSequence &, const ColorSequence &) = default;
};
}
