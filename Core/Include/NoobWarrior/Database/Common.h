// === noobWarrior ===
// File: Common.h
// Started by: Hattozo
// Started on: 11/11/2025
// Description: Just a few common enums and typedef aliases to help you get started.
#pragma once
#include <variant>
#include <string>
#include <vector>
#include <cstdint>

namespace NoobWarrior {
typedef std::variant<std::monostate, int, bool, int64_t, double, std::string, std::vector<unsigned char>> SqlValue;
typedef std::pair<std::string, SqlValue> SqlColumn;
typedef std::vector<SqlColumn> SqlRow;

enum class DatabaseResponse {
    Failed,
    Success,
    CouldNotOpen,
    CouldNotGetVersion,
    CouldNotSetVersion,
    CouldNotCreateTable,
    CouldNotSetKeyValues,
    DidNothing,
    NotInitialized,
    StatementConstraintViolation,
    Busy,
    Misuse,
    DoesNotExist
};
}