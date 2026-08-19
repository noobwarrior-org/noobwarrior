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
// File: NumberSequence.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.NumberSequence (MaximumADHD/Roblox-File-Format).
#pragma once

#include <NoobWarrior/Roblox/FileFormat/DataTypes/NumberSequenceKeypoint.h>

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace NoobWarrior::Roblox::DataTypes {
struct NumberSequence {
    // The reference rejects anything outside 2..20 keypoints (NumberSequence.cs:29-52). Reported
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

    std::vector<NumberSequenceKeypoint> Keypoints;

    // NumberSequence.cs has no parameterless constructor: a sequence with no keypoints is not a
    // shape the format admits, and a zero-keypoint column is one an engine may refuse to load.
    // Default to what NumberSequence(float) builds so a defaulted property is still valid.
    NumberSequence() : NumberSequence(0.0f) {}

    // NumberSequence.cs:15-27.
    explicit NumberSequence(float n) : NumberSequence(n, n) {}
    NumberSequence(float n0, float n1) :
        Keypoints {NumberSequenceKeypoint(0.0f, n0), NumberSequenceKeypoint(1.0f, n1)} {}

    // Deliberately unvalidated: PROP::Read has to be able to hold whatever a file actually
    // contains, however malformed. Prefer Create for keypoints that did not come from a file.
    explicit NumberSequence(std::vector<NumberSequenceKeypoint> keypoints) :
        Keypoints(std::move(keypoints)) {}

    static Validity Validate(const std::vector<NumberSequenceKeypoint> &keypoints) {
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

    // Stands in for the throwing NumberSequence(NumberSequenceKeypoint[]) constructor.
    static Validity Create(std::vector<NumberSequenceKeypoint> keypoints, NumberSequence &result) {
        const Validity validity = Validate(keypoints);
        if (validity != Validity::Ok)
            return validity;
        result = NumberSequence(std::move(keypoints));
        return Validity::Ok;
    }

    friend bool operator==(const NumberSequence &, const NumberSequence &) = default;
};
}
