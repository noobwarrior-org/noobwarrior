// === noobWarrior ===
// File: IdRecord.h
// Started by: Hattozo
// Started on: N/A
// Description: C-struct representation of an ItemType.
// The reason why these exist in this form is for seamless use of the API for C++ devs.
// The reflection system does not actually* need to depend on these structs.
#pragma once
#include "Record.h"
#include "../ContentImages.h"

namespace NoobWarrior {
struct IdRecord : Record {
    int64_t             Id;
    int                 Snapshot = 1;
    std::string         Name;
};
}
