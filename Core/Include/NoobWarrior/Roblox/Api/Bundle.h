// === noobWarrior ===
// File: Bundle.h (Roblox API)
// Started by: Hattozo
// Started on: 9/20/2025
// Description: A C-struct representation of what the Roblox API would give you when you ask it for a bundle.
#pragma once

namespace NoobWarrior::Roblox {
    constexpr int BundleTypeCount = 5;
    // Note: "None" is a fallback if the asset does not have a type. It's not a real value in the Roblox API
    enum class BundleType { None = 0, BodyParts = 1, Animations = 2, Shoes = 3, DynamicHead = 4, DynamicHeadAvatar = 5 };
}