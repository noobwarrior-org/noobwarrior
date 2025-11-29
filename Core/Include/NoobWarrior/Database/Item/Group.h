// === noobWarrior ===
// File: Group.h (Database)
// Started by: Hattozo
// Started on: 9/20/2025
// Description: A C-struct representation of what the database would give you when you ask it for a group.
#pragma once
#include <NoobWarrior/Database/Item/OwnedItem.h>

#include <cstdint>

namespace NoobWarrior {
struct Group : OwnedItem {
    /* These numbers are meant for viewing only and cannot be modified through AddContent(). */
    int64_t                     MemberCount;

    /* This however, can be modified. */
    int64_t                     Historical_MemberCount;
    int64_t                     Funds;

    std::string                 Shout;
    int64_t                     ShoutUserId;
    int64_t                     ShoutTimestamp;

    bool                        EnemyDeclarationsEnabled;
};
}
