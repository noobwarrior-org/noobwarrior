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
// File: BackupTreeView.cpp
// Started by: Hattozo
// Started on: 12/28/2025
// Description: Preconfigured QTreeView for displaying an ItemDescriptorModel tree.
#include "BackupTreeView.h"

#include <QHeaderView>

using namespace NoobWarrior;

BackupTreeView::BackupTreeView(QWidget *parent) : QTreeView(parent) {
    mModel = new ItemDescriptorModel(this);
    setModel(mModel);

    setAlternatingRowColors(true);
    setUniformRowHeights(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    header()->setSectionResizeMode(ItemDescriptorModel::ColumnName, QHeaderView::Stretch);
    header()->setSectionResizeMode(ItemDescriptorModel::ColumnType, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(ItemDescriptorModel::ColumnId,   QHeaderView::ResizeToContents);
}

void BackupTreeView::SetDescriptor(Backup::ItemDescriptor *desc) {
    mModel->SetRoot(desc, false);
    expandAll();
}