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
// File: FileManagerWidget.h
// Started by: Hattozo
// Started on: 7/2/2025
// Description: A widget that allows the user to reorganize content in a way they like by putting them into directories.
// This is different from the regular item browser because that organizes stuff on its own according to what it's
// seeing in the SQLite database. This lets you organize it yourself, just like a traditional file manager: tree
// views, folders, documents, and shortcuts to Roblox items, all backed by the database's FsNode table.
#pragma once
#include <NoobWarrior/FileSystem/DatabaseFileSystem.h>

#include <QDockWidget>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QPointer>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "Sdk/Studio/SourceEditorContainer.h"

class QWidget;
class QVBoxLayout;
class QLineEdit;
class QToolButton;
class QStackedWidget;
class QTreeWidget;
class QListWidget;
class QLabel;
class QPoint;

namespace NoobWarrior {
class EmuDb;

class FileManagerWidget : public QDockWidget {
    Q_OBJECT
public:
    enum class ViewMode { Details, List, Columns, Icons };

    FileManagerWidget(QWidget *parent = nullptr);
    ~FileManagerWidget();

    // Navigates to an absolute unix path, recording history. Kept as the public entry point that was
    // here before so existing callers continue to work.
    void Refresh(const QString &address = "/");

    // Re-reads the focused project's database and repopulates the current directory (rebuilding the
    // backing file system if the focused project changed). Does not touch history. Called by the SDK
    // whenever the focused project changes.
    void Reload();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void InitWidgets();

    void FitDetailsColumns();

    EmuDb* GetDatabase();
    DatabaseFileSystem* EnsureFileSystem();

    void NavigateTo(const QString &path, bool pushHistory = true);
    void Populate();
    void GoBack();
    void GoForward();
    void GoUp();
    void UpdateNavButtons();

    void SetViewMode(ViewMode mode);

    std::vector<int64_t> SelectedNodeIds();
    std::optional<int64_t> CurrentDirId();
    QString ChildPath(const QString &name) const;

    void ActivateNode(int64_t id);

    void ShowContextMenu(const QPoint &globalPos, bool onItem);
    void DoNewFolder();
    void DoNewDocument();
    void DoNewShortcut();
    void DoRename(int64_t id);
    void DoDelete(const std::vector<int64_t> &ids);
    void DoCopy(const std::vector<int64_t> &ids, bool cut);
    void DoPaste(const std::optional<int64_t> &destDir);
    void DoDownload(const std::vector<int64_t> &ids);
    void DoOpenDocument(int64_t id);
    void PruneDocumentEditors(DatabaseFileSystem* fs);
    void DoProperties(int64_t id);

    QString NodeTypeText(const DatabaseFileSystem::Node &node);
    QIcon NodeIcon(const DatabaseFileSystem::Node &node);
    void ExportNodeToDisk(int64_t id, const QString &destDir);

    std::unique_ptr<DatabaseFileSystem> mFs;
    EmuDb* mFsDb { nullptr };
    QString mCurrentPath { "/" };
    QStringList mBackStack;
    QStringList mForwardStack;
    ViewMode mViewMode { ViewMode::Details };
    QString mSearchFilter;
    bool mFittingColumns { false };

    using DocumentKey = std::pair<EmuDb*, int64_t>;
    std::map<DocumentKey, QPointer<SourceEditorContainer>> mOpenDocuments;

    std::vector<int64_t> mClipboardIds;
    EmuDb* mClipboardDb { nullptr };
    bool mClipboardCut { false };

    QWidget* MainWidget { nullptr };
    QVBoxLayout* MainLayout { nullptr };

    QToolButton* BackButton { nullptr };
    QToolButton* ForwardButton { nullptr };
    QToolButton* UpButton { nullptr };
    QToolButton* RefreshButton { nullptr };
    QLineEdit* AddressBar { nullptr };
    QLineEdit* SearchBar { nullptr };

    QStackedWidget* ViewStack { nullptr };
    QTreeWidget* DetailsView { nullptr };
    QListWidget* ListView { nullptr };
    QLabel* PlaceholderLabel { nullptr };
};
}
