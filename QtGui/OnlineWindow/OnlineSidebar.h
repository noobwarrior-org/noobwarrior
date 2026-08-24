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
// File: OnlineSidebar.h
// Started by: Hattozo
// Started on: 8/23/2026
// Description: Widget that contains a list of master servers and their pages
#pragma once
#include <QDockWidget>
#include <QStandardItemModel>
#include <QTreeView>

namespace NoobWarrior {
// What a selected sidebar row points at. Each master server contributes one MasterRoot row with a
// Servers/Profile/Workshop child under it; the window keeps one page per kind.
enum class OnlineNodeKind {
    None,
    Favorites,
    Recents,
    MasterRoot,
    Servers,
    Profile,
    Workshop
};

class OnlineSidebar : public QDockWidget {
    Q_OBJECT
public:
    explicit OnlineSidebar(QWidget *parent = nullptr);

    // Rebuilds the tree from MasterServerStore, keeping the current selection when it still exists.
    void Reload();
    // Shows the Add Master Server dialog and adds what it resolves. Also driven from the toolbar.
    void PromptAddMaster();

signals:
    // masterUrl is empty for the Favorites and Recents rows.
    void NodeSelected(OnlineNodeKind kind, const QString &masterUrl);
    // A master was added or removed, or its session changed; open pages should re-read their state.
    void MastersChanged();

private:
    void InitWidgets();
    void OnSelectionChanged(const QModelIndex &current);
    void OnContextMenu(const QPoint &pos);
    void RemoveMaster(const QString &masterUrl);
    // Re-runs the /fed/v1/info handshake so a renamed master picks up its new name.
    void RefreshBranding(const QString &masterUrl);

    QStandardItem *AppendMaster(const QString &url, const QString &name, const QString &tooltip);

    QTreeView *mView { nullptr };
    QStandardItemModel *mModel { nullptr };
    // Restored across a Reload() so adding a master doesn't throw the user back to the top.
    QString mSelectedMaster;
    OnlineNodeKind mSelectedKind { OnlineNodeKind::None };
};
}
