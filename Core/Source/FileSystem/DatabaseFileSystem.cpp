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
// File: DatabaseFileSystem.cpp
// Started by: Hattozo
// Started on: 6/27/2025
// Description: A VirtualFileSystem implementation for the filesystem seen in noobWarrior's database system.
#include <NoobWarrior/FileSystem/DatabaseFileSystem.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/SqlDb/Statement.h>
#include <NoobWarrior/Log.h>

#include <sqlite3.h>

#include <algorithm>
#include <sstream>

using namespace NoobWarrior;

#define NWDBFS_NODE_COLUMNS "Id, ParentId, Name, Type, length(Content), CreatedAt, ModifiedAt, ShortcutItemType, ShortcutItemId"

namespace {
void BindParent(Statement &stmt, int pos, const std::optional<int64_t> &parentId) {
    if (parentId)
        stmt.Bind(pos, *parentId);
    else
        stmt.Bind(pos);
}
}

DatabaseFileSystem::DatabaseFileSystem(EmuDb* db) : mDb(db) {
    if (mDb == nullptr || mDb->Fail())
        mFailCode = 1;
}

DatabaseFileSystem::~DatabaseFileSystem() {}

std::unique_ptr<VirtualFileSystem> DatabaseFileSystem::MakeUnique() const {
    return std::make_unique<DatabaseFileSystem>(*this);
}

std::string DatabaseFileSystem::NormalizePath(const std::string &path) {
    std::vector<std::string> parts = SplitPath(path);
    if (parts.empty())
        return "/";
    std::string out;
    for (const std::string &p : parts)
        out += "/" + p;
    return out;
}

std::vector<std::string> DatabaseFileSystem::SplitPath(const std::string &path) {
    std::vector<std::string> parts;
    std::string cur;
    std::stringstream ss(path);
    while (std::getline(ss, cur, '/')) {
        if (cur.empty() || cur == ".")
            continue;
        parts.push_back(cur);
    }
    return parts;
}

std::pair<std::string, std::string> DatabaseFileSystem::SplitParentAndName(const std::string &path) {
    std::vector<std::string> parts = SplitPath(path);
    if (parts.empty())
        return { "/", "" };
    std::string name = parts.back();
    parts.pop_back();
    std::string parent = "/";
    for (const std::string &p : parts)
        parent += (parent == "/" ? "" : "/") + p;
    return { parent, name };
}

DatabaseFileSystem::Node DatabaseFileSystem::ReadNodeRow(Statement &stmt) {
    Node node;
    node.Id = stmt.GetInt64FromColumnIndex(0);
    if (!stmt.IsColumnIndexNull(1))
        node.ParentId = stmt.GetInt64FromColumnIndex(1);
    node.Name = stmt.GetStringFromColumnIndex(2);
    node.Type = static_cast<NodeType>(stmt.GetIntFromColumnIndex(3));
    if (!stmt.IsColumnIndexNull(4))
        node.Size = static_cast<uint64_t>(stmt.GetInt64FromColumnIndex(4));
    node.CreatedAt = stmt.GetInt64FromColumnIndex(5);
    node.ModifiedAt = stmt.GetInt64FromColumnIndex(6);
    if (!stmt.IsColumnIndexNull(7))
        node.ShortcutItemType = stmt.GetIntFromColumnIndex(7);
    if (!stmt.IsColumnIndexNull(8))
        node.ShortcutItemId = stmt.GetInt64FromColumnIndex(8);
    return node;
}

std::optional<DatabaseFileSystem::Node> DatabaseFileSystem::GetNode(int64_t id) {
    if (Fail())
        return std::nullopt;

    Statement stmt = mDb->PrepareStatement("SELECT " NWDBFS_NODE_COLUMNS " FROM FsNode WHERE Id = ?;");
    if (stmt.Fail())
        return std::nullopt;
    stmt.Bind(1, id);

    std::optional<Node> result;
    if (stmt.Step() == SQLITE_ROW)
        result = ReadNodeRow(stmt);
    return result;
}

std::optional<int64_t> DatabaseFileSystem::ResolvePath(const std::string &path) {
    if (Fail())
        return std::nullopt;

    std::optional<int64_t> currentParent; // empty == root

    for (const std::string &component : SplitPath(path)) {
        Statement stmt = mDb->PrepareStatement(
            "SELECT Id FROM FsNode WHERE ParentId IS ? AND Name = ? COLLATE NOCASE LIMIT 1;");
        if (stmt.Fail())
            return std::nullopt;
        BindParent(stmt, 1, currentParent);
        stmt.Bind(2, component);

        std::optional<int64_t> found;
        if (stmt.Step() == SQLITE_ROW)
            found = stmt.GetInt64FromColumnIndex(0);
        if (!found)
            return std::nullopt;
        currentParent = found;
    }
    return currentParent; // empty when path was "/" (the root has no row)
}

std::optional<DatabaseFileSystem::Node> DatabaseFileSystem::GetNodeByPath(const std::string &path) {
    std::string normalized = NormalizePath(path);
    if (normalized == "/") {
        Node root;
        root.Id = 0;
        root.Type = NodeType::Directory;
        root.Name = "";
        return root;
    }
    std::optional<int64_t> id = ResolvePath(normalized);
    if (!id)
        return std::nullopt;
    return GetNode(*id);
}

std::vector<DatabaseFileSystem::Node> DatabaseFileSystem::ListChildren(const std::optional<int64_t> &parentId) {
    std::vector<Node> out;
    if (Fail())
        return out;

    // Directories first, then by name (case-insensitive), the conventional explorer ordering. The UI
    // is free to re-sort; this is just a sensible default.
    Statement stmt = mDb->PrepareStatement(
        "SELECT " NWDBFS_NODE_COLUMNS " FROM FsNode WHERE ParentId IS ? "
        "ORDER BY (Type = 0) DESC, Name COLLATE NOCASE ASC;");
    if (stmt.Fail())
        return out;
    BindParent(stmt, 1, parentId);

    while (stmt.Step() == SQLITE_ROW)
        out.push_back(ReadNodeRow(stmt));
    return out;
}

std::vector<DatabaseFileSystem::Node> DatabaseFileSystem::ListChildrenByPath(const std::string &path) {
    std::string normalized = NormalizePath(path);
    if (normalized == "/")
        return ListChildren(std::nullopt);
    std::optional<int64_t> id = ResolvePath(normalized);
    if (!id)
        return {};
    return ListChildren(id);
}

bool DatabaseFileSystem::NameExistsInDirectory(const std::optional<int64_t> &parentId, const std::string &name,
                                               std::optional<int64_t> excludeId) {
    if (Fail())
        return false;

    std::string sql = "SELECT 1 FROM FsNode WHERE ParentId IS ? AND Name = ? COLLATE NOCASE";
    if (excludeId)
        sql += " AND Id != ?";
    sql += " LIMIT 1;";

    Statement stmt = mDb->PrepareStatement(sql);
    if (stmt.Fail())
        return false;
    BindParent(stmt, 1, parentId);
    stmt.Bind(2, name);
    if (excludeId)
        stmt.Bind(3, *excludeId);

    return stmt.Step() == SQLITE_ROW;
}

std::string DatabaseFileSystem::MakeUniqueName(const std::optional<int64_t> &parentId, const std::string &desired) {
    if (!NameExistsInDirectory(parentId, desired))
        return desired;

    // Split off an extension so "file.txt" becomes "file - Copy.txt".
    std::string stem = desired;
    std::string ext;
    size_t dot = desired.find_last_of('.');
    if (dot != std::string::npos && dot != 0) {
        stem = desired.substr(0, dot);
        ext = desired.substr(dot);
    }

    for (int i = 1; ; i++) {
        std::string candidate = (i == 1)
            ? stem + " - Copy" + ext
            : stem + " - Copy (" + std::to_string(i) + ")" + ext;
        if (!NameExistsInDirectory(parentId, candidate))
            return candidate;
    }
}

/* ------------------------------------------------------------------ */
/* Mutations                                                          */
/* ------------------------------------------------------------------ */

VirtualFileSystem::Response DatabaseFileSystem::CreateDirectory(const std::optional<int64_t> &parentId,
                                                                const std::string &name, int64_t *outId) {
    if (Fail())
        return Response::FileSystemFailed;
    if (name.empty())
        return Response::Failed;
    if (NameExistsInDirectory(parentId, name))
        return Response::Failed;

    Statement stmt = mDb->PrepareStatement(
        "INSERT INTO FsNode (ParentId, Name, Type, Content, CreatedAt, ModifiedAt) "
        "VALUES (?, ?, 0, NULL, unixepoch(), unixepoch());");
    if (stmt.Fail())
        return Response::Failed;
    BindParent(stmt, 1, parentId);
    stmt.Bind(2, name);

    if (stmt.Step() != SQLITE_DONE)
        return Response::Failed;
    if (outId)
        *outId = sqlite3_last_insert_rowid(mDb->Get());
    mDb->MarkDirty();
    return Response::Success;
}

VirtualFileSystem::Response DatabaseFileSystem::CreateDocument(const std::optional<int64_t> &parentId,
                                                              const std::string &name,
                                                              const std::vector<unsigned char> &content,
                                                              int64_t *outId) {
    if (Fail())
        return Response::FileSystemFailed;
    if (name.empty())
        return Response::Failed;
    if (NameExistsInDirectory(parentId, name))
        return Response::Failed;

    Statement stmt = mDb->PrepareStatement(
        "INSERT INTO FsNode (ParentId, Name, Type, Content, CreatedAt, ModifiedAt) "
        "VALUES (?, ?, 1, ?, unixepoch(), unixepoch());");
    if (stmt.Fail())
        return Response::Failed;
    BindParent(stmt, 1, parentId);
    stmt.Bind(2, name);
    stmt.Bind(3, content);

    if (stmt.Step() != SQLITE_DONE)
        return Response::Failed;
    if (outId)
        *outId = sqlite3_last_insert_rowid(mDb->Get());
    mDb->MarkDirty();
    return Response::Success;
}

VirtualFileSystem::Response DatabaseFileSystem::CreateShortcut(const std::optional<int64_t> &parentId,
                                                              const std::string &name,
                                                              int itemType, int64_t itemId, int64_t *outId) {
    if (Fail())
        return Response::FileSystemFailed;
    if (name.empty())
        return Response::Failed;
    if (NameExistsInDirectory(parentId, name))
        return Response::Failed;

    Statement stmt = mDb->PrepareStatement(
        "INSERT INTO FsNode (ParentId, Name, Type, Content, CreatedAt, ModifiedAt, ShortcutItemType, ShortcutItemId) "
        "VALUES (?, ?, 2, NULL, unixepoch(), unixepoch(), ?, ?);");
    if (stmt.Fail())
        return Response::Failed;
    BindParent(stmt, 1, parentId);
    stmt.Bind(2, name);
    stmt.Bind(3, itemType);
    stmt.Bind(4, itemId);

    if (stmt.Step() != SQLITE_DONE)
        return Response::Failed;
    if (outId)
        *outId = sqlite3_last_insert_rowid(mDb->Get());
    mDb->MarkDirty();
    return Response::Success;
}

VirtualFileSystem::Response DatabaseFileSystem::RenameNode(int64_t id, const std::string &newName) {
    if (Fail())
        return Response::FileSystemFailed;
    if (newName.empty())
        return Response::Failed;

    std::optional<Node> node = GetNode(id);
    if (!node)
        return Response::NotFound;
    if (NameExistsInDirectory(node->ParentId, newName, id))
        return Response::Failed;

    Statement stmt = mDb->PrepareStatement("UPDATE FsNode SET Name = ?, ModifiedAt = unixepoch() WHERE Id = ?;");
    if (stmt.Fail())
        return Response::Failed;
    stmt.Bind(1, newName);
    stmt.Bind(2, id);
    if (stmt.Step() != SQLITE_DONE)
        return Response::Failed;
    mDb->MarkDirty();
    return Response::Success;
}

VirtualFileSystem::Response DatabaseFileSystem::MoveNode(int64_t id, const std::optional<int64_t> &newParentId) {
    if (Fail())
        return Response::FileSystemFailed;

    std::optional<Node> node = GetNode(id);
    if (!node)
        return Response::NotFound;

    if (newParentId) {
        if (*newParentId == id)
            return Response::Failed;
        std::optional<int64_t> walk = newParentId;
        while (walk) {
            if (*walk == id)
                return Response::Failed;
            std::optional<Node> n = GetNode(*walk);
            if (!n)
                break;
            walk = n->ParentId;
        }
    }

    if (NameExistsInDirectory(newParentId, node->Name, id))
        return Response::Failed; // a sibling already owns that name in the destination

    Statement stmt = mDb->PrepareStatement("UPDATE FsNode SET ParentId = ?, ModifiedAt = unixepoch() WHERE Id = ?;");
    if (stmt.Fail())
        return Response::Failed;
    BindParent(stmt, 1, newParentId);
    stmt.Bind(2, id);
    if (stmt.Step() != SQLITE_DONE)
        return Response::Failed;
    mDb->MarkDirty();
    return Response::Success;
}

VirtualFileSystem::Response DatabaseFileSystem::CopyNode(int64_t id, const std::optional<int64_t> &destParentId,
                                                         int64_t *outId) {
    if (Fail())
        return Response::FileSystemFailed;

    std::optional<Node> node = GetNode(id);
    if (!node)
        return Response::NotFound;

    // Block copying a directory into its own subtree (would recurse forever).
    if (node->Type == NodeType::Directory && destParentId) {
        std::optional<int64_t> walk = destParentId;
        while (walk) {
            if (*walk == id)
                return Response::Failed;
            std::optional<Node> n = GetNode(*walk);
            if (!n)
                break;
            walk = n->ParentId;
        }
    }

    std::string name = MakeUniqueName(destParentId, node->Name);

    switch (node->Type) {
    case NodeType::Directory: {
        int64_t newId = 0;
        Response r = CreateDirectory(destParentId, name, &newId);
        if (r != Response::Success)
            return r;
        for (const Node &child : ListChildren(id)) {
            Response cr = CopyNode(child.Id, newId);
            if (cr != Response::Success)
                return cr;
        }
        if (outId)
            *outId = newId;
        return Response::Success;
    }
    case NodeType::File: {
        std::vector<unsigned char> content;
        ReadFileContent(id, &content);
        return CreateDocument(destParentId, name, content, outId);
    }
    case NodeType::Shortcut:
        return CreateShortcut(destParentId, name,
                              node->ShortcutItemType.value_or(0),
                              node->ShortcutItemId.value_or(0), outId);
    }
    return Response::Failed;
}

VirtualFileSystem::Response DatabaseFileSystem::DeleteNode(int64_t id) {
    if (Fail())
        return Response::FileSystemFailed;

    std::optional<Node> node = GetNode(id);
    if (!node)
        return Response::NotFound;

    // Recursively delete a directory's children first (FsNode has no ON DELETE CASCADE).
    if (node->Type == NodeType::Directory) {
        for (const Node &child : ListChildren(id)) {
            Response r = DeleteNode(child.Id);
            if (r != Response::Success)
                return r;
        }
    }

    Statement stmt = mDb->PrepareStatement("DELETE FROM FsNode WHERE Id = ?;");
    if (stmt.Fail())
        return Response::Failed;
    stmt.Bind(1, id);
    if (stmt.Step() != SQLITE_DONE)
        return Response::Failed;
    mDb->MarkDirty();
    return Response::Success;
}

VirtualFileSystem::Response DatabaseFileSystem::ReadFileContent(int64_t id, std::vector<unsigned char> *out) {
    if (Fail())
        return Response::FileSystemFailed;
    if (out == nullptr)
        return Response::Failed;
    out->clear();

    Statement stmt = mDb->PrepareStatement("SELECT Content FROM FsNode WHERE Id = ?;");
    if (stmt.Fail())
        return Response::Failed;
    stmt.Bind(1, id);

    if (stmt.Step() != SQLITE_ROW)
        return Response::NotFound;
    if (!stmt.IsColumnIndexNull(0))
        *out = stmt.GetBlobFromColumnIndex(0);
    return Response::Success;
}

VirtualFileSystem::Response DatabaseFileSystem::WriteFileContent(int64_t id, const std::vector<unsigned char> &content) {
    if (Fail())
        return Response::FileSystemFailed;

    std::optional<Node> node = GetNode(id);
    if (!node)
        return Response::NotFound;
    if (node->Type != NodeType::File)
        return Response::Failed; // only documents hold inline content

    Statement stmt = mDb->PrepareStatement("UPDATE FsNode SET Content = ?, ModifiedAt = unixepoch() WHERE Id = ?;");
    if (stmt.Fail())
        return Response::Failed;
    stmt.Bind(1, content);
    stmt.Bind(2, id);
    if (stmt.Step() != SQLITE_DONE)
        return Response::Failed;
    mDb->MarkDirty();
    return Response::Success;
}

FSEntryInfo DatabaseFileSystem::GetEntryFromPath(const std::string &path) {
    FSEntryInfo entry {};
    entry.Owner = this;
    std::string normalized = NormalizePath(path);
    entry.Path = normalized;

    std::optional<Node> node = GetNodeByPath(normalized);
    if (!node) {
        entry.Exists = false;
        return entry;
    }
    entry.Exists = true;
    entry.Type = node->Type == NodeType::Directory ? FSEntryInfo::Type::Directory : FSEntryInfo::Type::File;
    entry.Size = node->Size;
    entry.Name = node->Name;
    return entry;
}

std::vector<FSEntryInfo> DatabaseFileSystem::GetEntriesInDirectory(const std::string &path) {
    std::vector<FSEntryInfo> entries;
    std::string normalized = NormalizePath(path);
    std::string prefix = (normalized == "/") ? "/" : normalized + "/";

    for (const Node &node : ListChildrenByPath(normalized)) {
        FSEntryInfo entry {};
        entry.Owner = this;
        entry.Exists = true;
        entry.Type = node.Type == NodeType::Directory ? FSEntryInfo::Type::Directory : FSEntryInfo::Type::File;
        entry.Size = node.Size;
        entry.Name = node.Name;
        entry.Path = prefix + node.Name;
        entries.push_back(entry);
    }
    return entries;
}

FSEntryHandle DatabaseFileSystem::OpenHandle(const std::string &path) {
    if (Fail())
        return 0;
    std::optional<Node> node = GetNodeByPath(path);
    if (!node || node->Type == NodeType::Directory)
        return 0;

    OpenFile file;
    if (node->Type == NodeType::File)
        ReadFileContent(node->Id, &file.Data);
    // A shortcut has no inline bytes; it opens as an empty stream.

    FSEntryHandle id = mNextHandle++;
    mHandles.emplace(id, std::move(file));
    return id;
}

VirtualFileSystem::Response DatabaseFileSystem::CloseHandle(FSEntryHandle handle) {
    auto it = mHandles.find(handle);
    if (it == mHandles.end())
        return Response::InvalidHandle;
    mHandles.erase(it);
    return Response::Success;
}

bool DatabaseFileSystem::IsHandleEOF(FSEntryHandle handle) {
    auto it = mHandles.find(handle);
    if (it == mHandles.end())
        return false;
    return it->second.Cursor >= it->second.Data.size();
}

bool DatabaseFileSystem::ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) {
    auto it = mHandles.find(handle);
    if (it == mHandles.end() || buf == nullptr)
        return false;
    OpenFile &file = it->second;
    if (file.Cursor >= file.Data.size())
        return false;

    size_t remaining = file.Data.size() - file.Cursor;
    size_t take = std::min(static_cast<size_t>(size), remaining);
    buf->assign(file.Data.begin() + file.Cursor, file.Data.begin() + file.Cursor + take);
    file.Cursor += take;
    return file.Cursor < file.Data.size();
}

bool DatabaseFileSystem::ReadHandleLine(FSEntryHandle handle, std::string *buf) {
    auto it = mHandles.find(handle);
    if (it == mHandles.end() || buf == nullptr)
        return false;
    OpenFile &file = it->second;
    if (file.Cursor >= file.Data.size())
        return false;

    std::string line;
    while (file.Cursor < file.Data.size()) {
        char c = static_cast<char>(file.Data[file.Cursor++]);
        if (c == '\n')
            break;
        if (c != '\r')
            line.push_back(c);
    }
    *buf = std::move(line);
    return true;
}

bool DatabaseFileSystem::EntryExists(const std::string &path) {
    std::string normalized = NormalizePath(path);
    if (normalized == "/")
        return true;
    return ResolvePath(normalized).has_value();
}

VirtualFileSystem::Response DatabaseFileSystem::DeleteEntry(const std::string &path) {
    if (Fail())
        return Response::FileSystemFailed;
    std::optional<int64_t> id = ResolvePath(path);
    if (!id)
        return Response::NotFound;
    return DeleteNode(*id);
}

VirtualFileSystem::Response DatabaseFileSystem::WriteFile(const std::string &path, const std::vector<unsigned char> &data) {
    if (Fail())
        return Response::FileSystemFailed;

    auto [parentPath, name] = SplitParentAndName(path);
    if (name.empty())
        return Response::Failed;

    Response dirRes = CreateDirectories(parentPath);
    if (dirRes != Response::Success)
        return dirRes;

    std::optional<int64_t> parentId = ResolvePath(parentPath); // empty == root
    std::optional<Node> existing = GetNodeByPath(NormalizePath(path));
    if (existing) {
        if (existing->Type == NodeType::Directory)
            return Response::Failed; // can't overwrite a directory with a file
        return WriteFileContent(existing->Id, data);
    }
    return CreateDocument(parentId, name, data);
}

VirtualFileSystem::Response DatabaseFileSystem::CreateDirectories(const std::string &path) {
    if (Fail())
        return Response::FileSystemFailed;

    std::optional<int64_t> currentParent; // root
    for (const std::string &component : SplitPath(path)) {
        // Reuse an existing directory of this name, or create it.
        Statement stmt = mDb->PrepareStatement(
            "SELECT Id, Type FROM FsNode WHERE ParentId IS ? AND Name = ? COLLATE NOCASE LIMIT 1;");
        if (stmt.Fail())
            return Response::Failed;
        BindParent(stmt, 1, currentParent);
        stmt.Bind(2, component);

        std::optional<int64_t> foundId;
        int foundType = -1;
        if (stmt.Step() == SQLITE_ROW) {
            foundId = stmt.GetInt64FromColumnIndex(0);
            foundType = stmt.GetIntFromColumnIndex(1);
        }

        if (foundId) {
            if (foundType != static_cast<int>(NodeType::Directory))
                return Response::Failed; // a file is in the way of the directory path
            currentParent = foundId;
        } else {
            int64_t newId = 0;
            Response r = CreateDirectory(currentParent, component, &newId);
            if (r != Response::Success)
                return r;
            currentParent = newId;
        }
    }
    return Response::Success;
}
