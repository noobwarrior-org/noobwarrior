// === noobWarrior ===
// File: DatabaseFileSystem.h
// Started by: Hattozo
// Started on: 12/5/2025
// Description: A IFileSystem implementation for the filesystem seen in noobWarrior's database system.
#pragma once
#include "IFileSystem.h"

namespace NoobWarrior {
class Database;
class DatabaseFileSystem : public IFileSystem {
public:
    DatabaseFileSystem(Database* db);
};
}