// === noobWarrior ===
// File: DatabaseFileSystem.h
// Started by: Hattozo
// Started on: 12/5/2025
// Description: A VirtualFileSystem implementation for the filesystem seen in noobWarrior's database system.
#pragma once
#include "VirtualFileSystem.h"

namespace NoobWarrior {
class Database;
class DatabaseFileSystem : public VirtualFileSystem {
public:
    DatabaseFileSystem(Database* db);
};
}