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
// File: RobloxFilePreviewController.h
// Started by: Hattozo
// Started on: 8/29/2026
// Description: The SDK-docked Studio panels and the files loaded into them.
#pragma once
#include <NoobWarrior/Roblox/Api/Asset.h>
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>

#include <QDockWidget>
#include <QMainWindow>

#include <functional>
#include <memory>
#include <vector>

#include "ExplorerWidget.h"
#include "PropertiesWidget.h"

namespace NoobWarrior {
class EmuDb;

void ShowDockedPreviewForDatabaseAsset(QMainWindow* host, std::function<EmuDb*()> dbGetter,
                                       int64_t assetId);

const std::vector<Roblox::AssetType>& PreviewableAssetTypes();
bool IsPreviewableAssetType(Roblox::AssetType type);

class RobloxFilePreviewController : public QObject {
    Q_OBJECT
public:
    explicit RobloxFilePreviewController(QMainWindow* host);

    bool LoadFile(const QString& key, const QString& title, const std::vector<unsigned char>& data,
                  std::function<bool(const std::vector<unsigned char>&)> save,
                  AssetDataResolver resolver);

    bool HasFiles() const { return !mFiles.empty(); }
    bool CanSaveAny() const;

    bool SaveActiveFile();
    bool CloseActiveFile();

    bool SaveFile(Roblox::RobloxFile* file);
    bool CloseFile(Roblox::RobloxFile* file);

    void OpenFromDatabasePrompt();
    void OpenFromFilePrompt();
    
    bool ConfirmCloseAll();

    void EnsureVisible();

    QDockWidget* GetExplorerDock() const { return mExplorerDock; }
    QDockWidget* GetPropertiesDock() const { return mPropertiesDock; }
signals:
    void FileStateChanged(); // load/close/save/dirty, hosts refresh menu enablement
private:
    struct Entry {
        QString Key;
        QString Title;
        std::unique_ptr<Roblox::RobloxFile> File;
        std::function<bool(const std::vector<unsigned char>&)> Save;
        AssetDataResolver Resolver;
        bool Dirty { false };
    };

    Entry* EntryFor(Roblox::RobloxFile* file);
    Entry* ActiveEntry();
    bool SaveEntry(Entry& entry);
    bool ConfirmCloseEntry(Entry& entry);
    void SetDirty(Roblox::RobloxFile* file, bool dirty);

    std::vector<std::unique_ptr<Entry>> mFiles;
    ExplorerWidget* mExplorer;
    PropertiesWidget* mProperties;
    QDockWidget* mExplorerDock;
    QDockWidget* mPropertiesDock;
};
}
