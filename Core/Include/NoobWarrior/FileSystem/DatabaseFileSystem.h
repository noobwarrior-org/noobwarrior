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
// File: DatabaseFileSystem.h
// Started by: Hattozo
// Started on: 12/5/2025
// Description: A VirtualFileSystem implementation for the filesystem seen in noobWarrior's database system.
// It is backed by the EmuDb "FsNode" table: a self-referencing tree of directories, plain-content
// "documents", and "shortcuts" that point at a Roblox item (Asset, Badge, ...) elsewhere in the same
// database. Paths are unix-style and absolute ("/", "/folder/file.txt"); the root is the set of rows
// with a NULL ParentId.
#pragma once
#include "VirtualFileSystem.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace NoobWarrior {
class EmuDb;
class Statement;

class DatabaseFileSystem : public VirtualFileSystem {
public:
    // The integer stored in FsNode.Type. Kept in sync with the migration v20 documentation.
    enum class NodeType {
        Directory = 0,
        File      = 1, // a "document": its bytes live in FsNode.Content
        Shortcut  = 2  // points at a Roblox item via (ShortcutItemType, ShortcutItemId)
    };

    // A fully-described FsNode row, richer than the generic FSEntryInfo (which only knows
    // File/Directory). The file manager UI works with these directly.
    struct Node {
        int64_t                 Id          { 0 };
        std::optional<int64_t>  ParentId    {};            // empty == lives in the root
        std::string             Name        {};
        NodeType                Type        { NodeType::File };
        uint64_t                Size        { 0 };          // Content byte length (documents only)
        int64_t                 CreatedAt   { 0 };          // unix epoch seconds
        int64_t                 ModifiedAt  { 0 };          // unix epoch seconds
        std::optional<int>      ShortcutItemType {};        // an ItemType value (shortcuts only)
        std::optional<int64_t>  ShortcutItemId   {};        // referenced item id (shortcuts only)
    };

    DatabaseFileSystem(EmuDb* db);
    ~DatabaseFileSystem() override;

    EmuDb* GetDatabase() const { return mDb; }

    /* ---- VirtualFileSystem interface ---- */
    std::unique_ptr<VirtualFileSystem> MakeUnique() const override;

    FSEntryInfo GetEntryFromPath(const std::string &path) override;
    std::vector<FSEntryInfo> GetEntriesInDirectory(const std::string &path) override;

    FSEntryHandle OpenHandle(const std::string &path) override;
    Response CloseHandle(FSEntryHandle handle) override;
    bool IsHandleEOF(FSEntryHandle handle) override;
    bool ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) override;
    bool ReadHandleLine(FSEntryHandle handle, std::string *buf) override;

    bool EntryExists(const std::string &path) override;
    Response DeleteEntry(const std::string &path) override;

    Response WriteFile(const std::string &path, const std::vector<unsigned char> &data) override;
    Response CreateDirectories(const std::string &path) override;

    /* ---- Database-specific tree API (used by the file manager) ---- */

    // Resolves an absolute unix path to a node. Returns empty for "/" (the synthetic root has no row)
    // or when any path component doesn't exist.
    std::optional<int64_t> ResolvePath(const std::string &path);
    std::optional<Node> GetNode(int64_t id);
    std::optional<Node> GetNodeByPath(const std::string &path);

    // Lists the direct children of a directory. An empty parentId lists the root.
    std::vector<Node> ListChildren(const std::optional<int64_t> &parentId);
    std::vector<Node> ListChildrenByPath(const std::string &path);

    // True if `name` is already taken by a sibling under parentId (case-insensitive). excludeId lets a
    // rename ignore the node being renamed.
    bool NameExistsInDirectory(const std::optional<int64_t> &parentId, const std::string &name,
                               std::optional<int64_t> excludeId = std::nullopt);

    Response CreateDirectory(const std::optional<int64_t> &parentId, const std::string &name, int64_t *outId = nullptr);
    Response CreateDocument(const std::optional<int64_t> &parentId, const std::string &name,
                            const std::vector<unsigned char> &content, int64_t *outId = nullptr);
    Response CreateShortcut(const std::optional<int64_t> &parentId, const std::string &name,
                            int itemType, int64_t itemId, int64_t *outId = nullptr);

    Response RenameNode(int64_t id, const std::string &newName);
    // Reparents a node. Refuses to move a directory into itself or one of its own descendants.
    Response MoveNode(int64_t id, const std::optional<int64_t> &newParentId);
    // Deep-copies a node (recursively, for directories) under destParentId. The copy gets fresh ids
    // and timestamps; if a name collision occurs the copy is suffixed (" - Copy", " - Copy (2)", ...).
    Response CopyNode(int64_t id, const std::optional<int64_t> &destParentId, int64_t *outId = nullptr);
    // Deletes a node, recursively removing a directory's whole subtree.
    Response DeleteNode(int64_t id);

    Response ReadFileContent(int64_t id, std::vector<unsigned char> *out);
    Response WriteFileContent(int64_t id, const std::vector<unsigned char> &content);

    // Collapses duplicate/trailing slashes and guarantees a leading slash. "" / "." become "/".
    static std::string NormalizePath(const std::string &path);
    // Splits a normalized path into its components (no empty entries). "/" yields {}.
    static std::vector<std::string> SplitPath(const std::string &path);
    // The directory portion and final component of a path (parent, leaf).
    static std::pair<std::string, std::string> SplitParentAndName(const std::string &path);

protected:
    // Builds a Node from the currently-stepped row of a "SELECT Id, ParentId, Name, Type,
    // length(Content), CreatedAt, ModifiedAt, ShortcutItemType, ShortcutItemId" statement.
    Node ReadNodeRow(Statement &stmt);

    // Resolves a unique, non-colliding name for a new child under parentId, starting from `desired`.
    std::string MakeUniqueName(const std::optional<int64_t> &parentId, const std::string &desired);

    EmuDb* mDb { nullptr };

    // OpenHandle snapshots a document's bytes; reads walk the snapshot cursor. Simple and safe since
    // documents are small user files held inline in the row.
    struct OpenFile {
        std::vector<unsigned char> Data;
        size_t Cursor { 0 };
    };
    std::map<FSEntryHandle, OpenFile> mHandles;
    FSEntryHandle mNextHandle { 1 };
};
}
