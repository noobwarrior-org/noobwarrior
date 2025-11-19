// === noobWarrior ===
// File: Group.h (Database)
// Started by: Hattozo
// Started on: 9/20/2025
// Description: A C-struct representation of what the database would give you when you ask it for a group.
// Please note: All nullable values should be wrapped in std::optional, for the sake of everyone's sanity.
// Doesn't matter if it's a integer or whatever. As long as there's no NOT NULL clause in the schema, wrap its C++ equivalent in std::optional. Please.
#pragma once
#include <NoobWarrior/Database/Item/OwnedItem.h>

#include <cstdint>

namespace NoobWarrior {
struct Group : OwnedItem {
    /* These numbers are meant for viewing only and cannot be modified through AddContent(). */
    int64_t                     MemberCount;

    /* This however, can be modified. */
    std::optional<int64_t>                     Historical_MemberCount;
    int64_t                     Funds;

    std::optional<std::string>                 Shout;
    std::optional<int64_t>                     ShoutUserId;
    std::optional<int64_t>                     ShoutTimestamp;

    bool                        EnemyDeclarationsEnabled;
};
}
