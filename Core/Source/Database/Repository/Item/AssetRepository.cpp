// === noobWarrior ===
// File: AssetRepository.cpp
// Started by: Hattozo
// Started on: 11/11/2025
// Description:
#include <NoobWarrior/Database/Repository/Item/AssetRepository.h>
#include <NoobWarrior/Database/Database.h>

using namespace NoobWarrior;

AssetRepository::AssetRepository(Database *db) : ItemRepository<Asset>(db) {

}

DatabaseResponse AssetRepository::SaveItem(const Asset &asset) {
    Statement stmt(mDb, R"(***
    
    ***)");
}

std::optional<Asset> AssetRepository::GetItemById(int64_t id) {
    Statement stmt(mDb, R"(***
    SELECT
        *
    FROM
        Asset
    CROSS JOIN
        AssetData
    CROSS JOIN
        AssetHistorical
    CROSS JOIN
        AssetMicrotransaction
    CROSS JOIN
        AssetMisc
    CROSS JOIN
        AssetPlaceThumbnail
    WHERE Id = ?;
    ***)");
    stmt.Bind(1, id);
    if (stmt.Step() != SQLITE_ROW)
        return std::nullopt;
    std::map<std::string, SqlValue> columnMap = stmt.GetColumnMap();
    Asset asset {};
    asset.Id = id;
    asset.Snapshot = std::get<int>(columnMap["Snapshot"]);
    asset.FirstRecorded = std::get<int64_t>(columnMap["FirstRecorded"]);
    asset.LastRecorded = std::get<int64_t>(columnMap["LastRecorded"]);
    asset.Name = std::get<std::string>(columnMap["Name"]);
    if (columnMap["UserId"].index() == 0) {
        
    }
    asset.CreatorType = std::get<int>(columnName["UserId"]);
    return asset;
}