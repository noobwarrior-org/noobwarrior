/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
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

DatabaseResponse AssetRepository::Save(const Asset &asset) {
    Statement stmt(mDb, R"(***
    INSERT INTO Asset
    (Id, Snapshot, Name, Description, Created, Updated, ImageId, ImageSnapshot, UserId, GroupId, Type, Public)
    VALUES
    (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    ON CONFLICT (Id, Snapshot)
    DO UPDATE SET
    Id=EXCLUDED.Id, Snapshot=EXCLUDED.Snapshot, LastRecorded=(unixepoch()), Name=EXCLUDED.Name, Description=EXCLUDED.Description,
    Created=EXCLUDED.Created, Updated=EXCLUDED.Updated, ImageId=EXCLUDED.ImageId, ImageSnapshot=EXCLUDED.ImageSnapshot,
    UserId=EXCLUDED.UserId, GroupId=EXCLUDED.GroupId, Type=EXCLUDED.Type, Public=EXCLUDED.Public
    WHERE Id = ?;
    ***)");
    if (stmt.Fail())
        return DatabaseResponse::Failed;
    stmt.Bind(1, asset.Id);
    stmt.Bind(2, asset.Snapshot);
    stmt.Bind(3, asset.Name);
    stmt.Bind(4, asset.Description);
    stmt.Bind(5, asset.Created);
    stmt.Bind(6, asset.Updated);
    stmt.Bind(7, asset.ImageId);
    stmt.Bind(8, asset.ImageSnapshot);
    asset.CreatorType == Roblox::CreatorType::User ? stmt.Bind(9, asset.CreatorId) : stmt.Bind(9);
    asset.CreatorType == Roblox::CreatorType::Group ? stmt.Bind(10, asset.CreatorId) : stmt.Bind(10);
    stmt.Bind(11, static_cast<int>(asset.Type));
    stmt.Bind(12, asset.Public);
    
    mDb->MarkDirty();
    if (stmt.Step() == SQLITE_DONE)
        return DatabaseResponse::Success;
    return DatabaseResponse::Failed;
}

DatabaseResponse AssetRepository::Remove(int64_t id, int snapshot) {
    Statement stmt(mDb, "DELETE FROM Asset WHERE Id = ? AND Snapshot = ?;");
    stmt.Bind(1, id);
    stmt.Bind(2, snapshot);
    mDb->MarkDirty();
    return stmt.Step() == SQLITE_DONE ? DatabaseResponse::Success : DatabaseResponse::Failed;
}

DatabaseResponse AssetRepository::Remove(int64_t id) {
    Statement stmt(mDb, "DELETE FROM Asset WHERE Id = ?;");
    stmt.Bind(1, id);
    mDb->MarkDirty();
    return stmt.Step() == SQLITE_DONE ? DatabaseResponse::Success : DatabaseResponse::Failed;
}

DatabaseResponse AssetRepository::Move(int64_t currentId, int64_t newId) {
    std::optional<Asset> ass = Get(currentId);
    if (!ass.has_value())
        return DatabaseResponse::DoesNotExist;
    ass.value().Id = newId;
    DatabaseResponse remove_res = Remove(currentId);
    if (remove_res != DatabaseResponse::Success) return DatabaseResponse::Failed;
    return Save(ass.value());
}

std::optional<Asset> AssetRepository::Get(int64_t id, int snapshot) {
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
    WHERE Id = ? AND Snapshot = ?;
    ***)");
    stmt.Bind(1, id);
    stmt.Bind(2, snapshot);
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

std::optional<Asset> AssetRepository::Get(int64_t id) {
    return Get(id, 1); // TODO: get latest snapshot instead of using an arbitrary number that may or may not exist.
}

std::vector<Asset> AssetRepository::List() {
    std::vector<Asset> list;
    Statement stmt(mDb, "SELECT * FROM Asset;");
    while (stmt.Step() == SQLITE_ROW) {
        int64_t id = std::get<int64_t>(stmt.GetValueFromColumnIndex(0));
        std::optional<Asset> asset = Get(id);
        if (asset.has_value()) list.push_back(asset.value());
    }
    return list;
}

bool AssetRepository::Exists(int64_t id, int snapshot) {
    Statement stmt(mDb, "SELECT Id FROM Asset WHERE Id = ? AND Snapshot = ?;");
    stmt.Bind(1, id);
    stmt.Bind(2, snapshot);
    return stmt.Step() == SQLITE_ROW;
}

bool AssetRepository::Exists(int64_t id) {
    Statement stmt(mDb, "SELECT Id FROM Asset WHERE Id = ? ORDER BY Snapshot DESC LIMIT 1;");
    stmt.Bind(1, id);
    return stmt.Step() == SQLITE_ROW;
}