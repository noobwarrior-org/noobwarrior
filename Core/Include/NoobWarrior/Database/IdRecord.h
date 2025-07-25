#pragma once
#include "Record.h"

namespace NoobWarrior {
struct IdRecord : Record {
    int64_t             Id;
    int                 Version;
    std::string         Name;
    std::string         Description;
    int64_t             Created;
    int64_t             Updated;
    int64_t             Icon;
};
}
