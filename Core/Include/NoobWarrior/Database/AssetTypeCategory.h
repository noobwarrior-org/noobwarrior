// === noobWarrior ===
// File: AssetTypeCategory.h
// Started by: Hattozo
// Started on: 7/25/2025
// Description: A more generalized version of the AssetType enum
#pragma once

namespace NoobWarrior {
constexpr int AssetTypeCategoryCount = 1;
enum class AssetTypeCategory {
    DevelopmentItem,
    AvatarItem
};

inline const char *AssetTypeCategoryAsTranslatableString(AssetTypeCategory type) {
    switch (type) {
        case AssetTypeCategory::DevelopmentItem: return "Development Item";
        case AssetTypeCategory::AvatarItem: return "Avatar Item";
        default: return "Unknown";
    }
}
}