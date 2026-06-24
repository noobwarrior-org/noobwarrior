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
// File: PluginTreeWidget.h
// Started by: Hattozo
// Started on: 6/23/2026
// Description:
#pragma once

#include <NoobWarrior/Plugin.h>
#include <QTreeWidget>

namespace NoobWarrior {
class PluginTreeWidget : public QTreeWidget {
    Q_OBJECT
public:
    PluginTreeWidget(QWidget* parent = nullptr);
    ~PluginTreeWidget();
    void Refresh(int flags); // i smoke flags every night
private slots:
    void OnItemChanged(QTreeWidgetItem* item, int column);
private:
    // Set while Refresh() is repopulating items so the programmatic setCheckState() calls don't get
    // mistaken for the user toggling the enable checkbox.
    bool mRefreshing = false;
};
}