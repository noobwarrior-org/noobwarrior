// === noobWarrior ===
// File: Record.h
// Started by: Hattozo
// Started on: N/A
// Description:
#pragma once
#include <string>
#include <tuple>
#include <cstdint>

namespace NoobWarrior {
struct Record {
    virtual ~Record() = default;
    int64_t             FirstRecorded;
    int64_t             LastRecorded;

    static constexpr const char* TableName = "?";
};
}
