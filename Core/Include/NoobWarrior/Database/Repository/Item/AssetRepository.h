// === noobWarrior ===
// File: AssetRepository.h
// Started by: Hattozo
// Started on: 11/11/2025
// Description:
#pragma once
#include <NoobWarrior/Database/Repository/Repository.h>
#include <NoobWarrior/Database/Item/Asset.h>

namespace NoobWarrior {
class AssetRepository : public ItemRepository<Asset> {
public:
    AssetRepository(Database *db);
    std::vector<unsigned char> RetrieveData(int64_t id, int snapshot);
    std::vector<unsigned char> RetrieveData(int64_t id);
    
    DatabaseResponse Save(const Asset &asset) override;
    DatabaseResponse Remove(int64_t id, int snapshot) override;
    DatabaseResponse Remove(int64_t id) override;
    DatabaseResponse Move(int64_t currentId, int64_t newId) override;
    std::optional<Asset> Get(int64_t id) override;
    std::optional<Asset> Get(int64_t id, int snapshot) override;
    std::vector<Asset> List() override;

    bool Exists(int64_t id, int snapshot) override;
    bool Exists(int64_t id) override;
};
}