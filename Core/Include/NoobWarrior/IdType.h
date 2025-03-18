// === noobWarrior ===
// File: IdType.h
// Started by: Hattozo
// Started on: 3/17/2025
// Description:
#pragma once

namespace NoobWarrior {
enum class IdType {
    Asset,
    Badge,
    Bundle,
    DevProduct,
    Group,
    Pass,
    Universe,
    User
};
inline const char *IdTypeAsString(IdType type) {
    switch (type) {
    case IdType::Asset: return "Asset";
    case IdType::Badge: return "Badge";
    case IdType::Bundle: return "Bundle";
    case IdType::DevProduct: return "DevProduct";
    case IdType::Group: return "Group";
    case IdType::Pass: return "Pass";
    case IdType::Universe: return "Universe";
    case IdType::User: return "User";
    default: return "";
    }
}
}