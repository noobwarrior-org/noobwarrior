// === noobWarrior ===
// File: IdRecord.h
// Started by: Hattozo
// Started on: N/A
// Description:
#pragma once
#include "Record.h"

namespace NoobWarrior {
struct IdRecord : Record {
    int64_t             Id;
    int                 Version;
    std::string         Name;
};
}
