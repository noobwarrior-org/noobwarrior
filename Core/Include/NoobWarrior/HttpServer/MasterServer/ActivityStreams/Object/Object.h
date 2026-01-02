// === noobWarrior ===
// File: Object.h
// Started by: Hattozo
// Started on: 12/29/2025
// Description:
#pragma once
#include <nlohmann/json_fwd.hpp>

namespace NoobWarrior::ActivityStreams {
class Object {
public:
    Object();

    nlohmann::json AsJson();
};
}