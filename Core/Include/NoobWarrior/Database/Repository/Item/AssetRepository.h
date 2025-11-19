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
    
    DatabaseResponse SaveItem(const Asset &asset) override;
    DatabaseResponse RemoveItem(int64_t id) override;
    DatabaseResponse MoveItem(int64_t currentId, int64_t newId) override;
    DatabaseResponse RemoveItemSnapshot(int64_t id, int snapshot) override;
    std::optional<Asset> GetItemById(int64_t id) override;
    std::optional<Asset> GetItemById(int64_t id, int snapshot) override;
    std::vector<Asset> ListItems() override;
};
}