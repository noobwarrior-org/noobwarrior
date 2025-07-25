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

inline const char *GetAddIconForIdType(IdType type) {
    switch (type) {
        case IdType::Asset: return ":/images/silk/page_add.png";
        case IdType::Badge: return ":/images/silk/medal_gold_add.png";
        case IdType::Bundle: return ":/images/silk/package_add.png";
        case IdType::DevProduct: return ":/images/silk/coins_add.png";
        case IdType::Group: return ":/images/silk/group_add.png";
        case IdType::Pass: return ":/images/silk/award_star_add.png";
        case IdType::Universe: return ":/images/silk/world_add.png";
        case IdType::User: return ":/images/silk/user_add.png";
        default: return "";
    }
}

inline const char *GetIconForIdType(IdType type) {
    switch (type) {
        case IdType::Asset: return ":/images/silk/page.png";
        case IdType::Badge: return ":/images/silk/medal_gold_1.png";
        case IdType::Bundle: return ":/images/silk/package.png";
        case IdType::DevProduct: return ":/images/silk/coins.png";
        case IdType::Group: return ":/images/silk/group.png";
        case IdType::Pass: return ":/images/silk/award_star_gold_3.png";
        case IdType::Universe: return ":/images/silk/world.png";
        case IdType::User: return ":/images/silk/user.png";
        default: return "";
    }
}
}