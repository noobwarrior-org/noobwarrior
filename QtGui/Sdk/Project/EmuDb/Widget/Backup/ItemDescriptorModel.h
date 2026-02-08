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
// File: ItemDescriptorModel.h
// Started by: Hattozo
// Started on: 12/28/2025
// Description: Qt model that works with ItemDescriptor
#pragma once
#include <NoobWarrior/Backup.h>
#include <QAbstractItemModel>

using namespace NoobWarrior::Backup;

namespace NoobWarrior {
class ItemDescriptorModel : public QAbstractItemModel {
    Q_OBJECT
public:
    ItemDescriptorModel(ItemDescriptor* desc);
    ItemDescriptorModel();
    ~ItemDescriptorModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
private:
    ItemDescriptor* mDescriptor;
    bool mIsUsingAlreadyAllocatedDesc;
};
}