#pragma once
#include "IdRecord.h"
#include "../Roblox/Api/Asset.h"

namespace NoobWarrior {
struct OwnedIdRecord : IdRecord {
    Roblox::CreatorType CreatorType;
    int64_t             CreatorId;
};
}
