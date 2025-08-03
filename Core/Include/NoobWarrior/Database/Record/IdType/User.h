// === noobWarrior ===
// File: User.h (Database)
// Started by: Hattozo
// Started on: 8/1/2025
// Description: A C-struct representation of what the database would give you when you ask it for a user.
#pragma once
#include <NoobWarrior/Database/Record/IdRecord.h>
#include <NoobWarrior/Roblox/Api/User.h>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace NoobWarrior {
    struct User : IdRecord {
        static constexpr const char* TableName = "User";
    };
}
