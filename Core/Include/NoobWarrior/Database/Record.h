#pragma once
#include <string>

namespace NoobWarrior {
struct Record {
    virtual ~Record() = default;
    int64_t             FirstRecorded;
    int64_t             LastRecorded;
};
}
