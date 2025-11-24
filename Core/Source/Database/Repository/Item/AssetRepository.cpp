// === noobWarrior ===
// File: AssetRepository.cpp
// Started by: Hattozo
// Started on: 11/11/2025
// Description:
#include <NoobWarrior/Database/Repository/Item/AssetRepository.h>
#include <NoobWarrior/Database/Database.h>
#include <NoobWarrior/Database/Common.h>
#include <NoobWarrior/Roblox/Api/User.h>

using namespace NoobWarrior;

AssetRepository::AssetRepository(Database *db) : ItemRepository<Asset>(db) {

}

std::vector<unsigned char> AssetRepository::RetrieveData(int64_t id, int snapshot) {
    return {};
}

std::vector<unsigned char> AssetRepository::RetrieveData(int64_t id) {
    return {};
}

DatabaseResponse AssetRepository::SaveItem(const Asset &asset) {
    Statement stmt(mDb, R"(***
    INSERT INTO Asset
    (Id, Snapshot, Name, Description, Created, Updated, Type, ImageId, ImageSnapshot, UserId, GroupId, Public)
    VALUES
    (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    ON CONFLICT (Id, Snapshot)
    DO UPDATE SET
    Id=EXCLUDED.Id, Snapshot=EXCLUDED.Snapshot, Name=EXCLUDED.Name, Description=EXCLUDED.Description,
    Created=EXCLUDED.Created, Updated=EXCLUDED.Updated, Type=EXCLUDED.Type, ImageId=EXCLUDED.ImageId,
    ImageSnapshot=EXCLUDED.ImageSnapshot, UserId=EXCLUDED.UserId, GroupId=EXCLUDED.GroupId,
    Public=EXCLUDED.Public
    WHERE Id = ?;
    ***)");
    if (stmt.Failed())
        return DatabaseResponse::Failed;
    stmt.Bind(1, asset.Id);
    stmt.Bind(2, asset.Snapshot);
    stmt.Bind(3, asset.Name);
    asset.Description.has_value() ? stmt.Bind(4, asset.Description.value()) : stmt.Bind(4);
    asset.Created.has_value() ? stmt.Bind(5, asset.Created.value()) : stmt.Bind(5);
    asset.Updated.has_value() ? stmt.Bind(6, asset.Updated.value()) : stmt.Bind(6);
    stmt.Bind(7, static_cast<int>(asset.Type));
    asset.ImageId.has_value() ? stmt.Bind(8, asset.ImageId.value()) : stmt.Bind(8);
    asset.ImageSnapshot.has_value() ? stmt.Bind(9, asset.ImageSnapshot.value()) : stmt.Bind(9);
    asset.CreatorType == Roblox::CreatorType::User && asset.CreatorId.has_value() ? stmt.Bind(10, asset.CreatorId.value()) : stmt.Bind(10);
    asset.CreatorType == Roblox::CreatorType::Group && asset.CreatorId.has_value() ? stmt.Bind(11, asset.CreatorId.value()) : stmt.Bind(11);
    asset.Public.has_value() ? stmt.Bind(12, asset.Public.value()) : stmt.Bind(12);
    
    if (stmt.Step() == SQLITE_DONE)
        return DatabaseResponse::Success;
    return DatabaseResponse::Failed;
}

DatabaseResponse AssetRepository::RemoveItem(int64_t id) {

    return DatabaseResponse::Success;
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
    // asset.CreatorType = std::get<int>(columnName["UserId"]);
    return asset;
}