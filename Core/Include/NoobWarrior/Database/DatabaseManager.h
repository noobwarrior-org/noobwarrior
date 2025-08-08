// === noobWarrior ===
// File: DatabaseManager.h
// Started by: Hattozo
// Started on: 3/17/2025
// Description:
#pragma once

#include <NoobWarrior/Database/Database.h>

#include <filesystem>
#include <vector>

namespace NoobWarrior {
class DatabaseManager {
public:
    DatabaseResponse Mount(const std::filesystem::path &filePath, unsigned int priority);
    void Mount(Database *database, unsigned int priority);
    int Unmount(Database *database);

    std::vector<unsigned char> RetrieveAssetData(int64_t id);

    template<typename T>
    bool DoesContentExist(const int64_t &id) {
        for (int i = 0; i < MountedDatabases.size(); i++) {
            Database *database = MountedDatabases[i];
            if (const bool exists = database->DoesContentExist<T>(id))
                return exists;
        }
        return false;
    }
private:
    std::vector<Database*> MountedDatabases;
};
}