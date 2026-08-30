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
// File: ExplorerWidget.h
// Started by: Hattozo
// Started on: 8/29/2026
// Description: A poor replication of Roblox Studio's Explorer widget
#pragma once
#include <NoobWarrior/Roblox/Api/Asset.h>
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>

#include <QDockWidget>
#include <QLineEdit>
#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QTreeWidget>
#include <QWidget>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "SourceEditorContainer.h"

namespace NoobWarrior {
class CodeEditorWidget;
class EmuDb;

QIcon StudioIconForClassName(const std::string& className, bool isService);

class ExplorerWidget : public QWidget {
    Q_OBJECT
public:
    explicit ExplorerWidget(QWidget* parent = nullptr);

    // Each open file is a root node; the instances stay owned by their RobloxFile.
    void AddFile(Roblox::RobloxFile* file, const QString& title);
    void RemoveFile(Roblox::RobloxFile* file);
    void FocusFile(Roblox::RobloxFile* file);
    // Appends/removes the dirty asterisk on the file's root node.
    void SetFileDirty(Roblox::RobloxFile* file, bool dirty);

    // The file owning the current selection; with exactly one file open, that one.
    Roblox::RobloxFile* ActiveFile() const;
    Roblox::RobloxFile* FileForInstance(Roblox::Instance* instance) const;

    // Re-reads one instance's display name after a rename.
    void RefreshInstanceLabel(Roblox::Instance* instance);

    // Where script sources open. Unset = a standalone editor window.
    void SetSourceEditorHost(SourceEditorTabHost host) { mSourceEditorHost = std::move(host); }

    // Shows Open-from entries on the blank-space context menu.
    void SetOpenMenuEnabled(bool enabled) { mOpenMenuEnabled = enabled; }

    // Opens the instance's Source; an already-open script refocuses its tab/window instead.
    void OpenSourceEditorFor(Roblox::Instance* instance);

    // True while any script editor opened from here holds unapplied changes.
    bool HasUnappliedSourceEdits() const;
    bool HasUnappliedSourceEditsUnder(Roblox::Instance* subtreeRoot) const;

    // Force-closes editors under `subtreeRoot` so they never outlive their instance.
    void CloseSourceEditorsUnder(Roblox::Instance* subtreeRoot);

    void CloseAllSourceEditors();
signals:
    void SelectionChanged(std::vector<NoobWarrior::Roblox::Instance*> instances);
    void InstanceDoubleClicked(NoobWarrior::Roblox::Instance* instance);
    // `file` now differs from its saved form.
    void TreeEdited(NoobWarrior::Roblox::RobloxFile* file);
    // Emitted just BEFORE a subtree is destroyed, while the instances are still walkable.
    void SubtreeRemoved(NoobWarrior::Roblox::Instance* instance);
    // Root-node / blank-space context-menu requests, handled by the host.
    void FileSaveRequested(NoobWarrior::Roblox::RobloxFile* file);
    void FileCloseRequested(NoobWarrior::Roblox::RobloxFile* file);
    void OpenFromDatabaseRequested();
    void OpenFromFileRequested();
private:
    void ApplyFilter(const QString& text);
    QTreeWidgetItem* AddInstanceRecursive(Roblox::Instance* instance, QTreeWidgetItem* parentItem);
    void RefreshFileNode(Roblox::RobloxFile* file);
    QTreeWidgetItem* RootItemForFile(Roblox::RobloxFile* file) const;
    Roblox::RobloxFile* FileForItem(QTreeWidgetItem* item) const;
    void ShowContextMenu(const QPoint& point);
    void EmitSelectionChanged();
    QList<QTreeWidgetItem*> TopMostSelectedItems() const;
    Roblox::Instance* InstanceForItem(QTreeWidgetItem* item) const;
    void OpenSourceEditor(Roblox::Instance* instance, const std::string& propName);

    QLineEdit* mFilter;
    QTreeWidget* mTree;
    std::vector<Roblox::RobloxFile*> mFiles;
    bool mOpenMenuEnabled { false };
    SourceEditorTabHost mSourceEditorHost;
    std::map<Roblox::Instance*, QPointer<SourceEditorContainer>> mSourceEditors;
};
}
