// === noobWarrior ===
// File: OwnedIdRecord.h
// Started by: Hattozo
// Started on: N/A
// Description:
#pragma once
#include "IdRecord.h"
#include "../../Roblox/Api/Asset.h"

namespace NoobWarrior {
struct OwnedIdRecord : IdRecord {
    Roblox::CreatorType CreatorType;
    int64_t             CreatorId;
    std::string         Description;
    int64_t             Created;
    int64_t             Updated;
    int64_t             ImageId;
    int                 ImageVersion;
};
}
