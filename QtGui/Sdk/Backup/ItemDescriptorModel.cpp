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
// File: ItemDescriptorModel.cpp
// Started by: Hattozo
// Started on: 12/28/2025
// Description: Qt model that exposes a Backup::ItemDescriptor tree to a QTreeView.
#include "ItemDescriptorModel.h"

using namespace NoobWarrior;
using Backup::ItemDescriptor;

static QString DescribeType(const ItemDescriptor *desc) {
    switch (desc->Type) {
    case ItemType::Universe:   return "Universe";
    case ItemType::Badge:      return "Badge";
    case ItemType::Bundle:     return "Bundle";
    case ItemType::DevProduct: return "DevProduct";
    case ItemType::Group:      return "Group";
    case ItemType::Outfit:     return "Outfit";
    case ItemType::Pass:       return "Pass";
    case ItemType::Set:        return "Set";
    case ItemType::User:       return "User";
    case ItemType::Asset:
        if (desc->AssetType == Roblox::AssetType::None)
            return "Asset";
        return QString::fromStdString(Roblox::AssetTypeAsTranslatableString(desc->AssetType));
    }
    return "Unknown";
}

static int RowOf(const ItemDescriptor *child) {
    const ItemDescriptor *parent = child->GetParent();
    if (parent == nullptr) return 0;
    const auto &siblings = parent->GetChildren();
    for (size_t i = 0; i < siblings.size(); i++)
        if (siblings[i] == child) return static_cast<int>(i);
    return 0;
}

ItemDescriptorModel::ItemDescriptorModel(ItemDescriptor *desc, QObject *parent)
    : QAbstractItemModel(parent), mDescriptor(desc), mOwnsDescriptor(false) {}

ItemDescriptorModel::ItemDescriptorModel(QObject *parent)
    : QAbstractItemModel(parent), mDescriptor(new ItemDescriptor()), mOwnsDescriptor(true) {}

ItemDescriptorModel::~ItemDescriptorModel() {
    if (mOwnsDescriptor) delete mDescriptor;
}

void ItemDescriptorModel::SetRoot(ItemDescriptor *desc, bool takeOwnership) {
    beginResetModel();
    if (mOwnsDescriptor && mDescriptor != desc)
        delete mDescriptor;
    mDescriptor = desc;
    mOwnsDescriptor = takeOwnership;
    endResetModel();
}

ItemDescriptor *ItemDescriptorModel::DescriptorAt(const QModelIndex &index) const {
    if (!index.isValid()) return nullptr;
    return static_cast<ItemDescriptor*>(index.internalPointer());
}

QModelIndex ItemDescriptorModel::index(int row, int column, const QModelIndex &parent) const {
    if (mDescriptor == nullptr) return {};
    if (column < 0 || column >= ColumnCount) return {};

    if (!parent.isValid()) {
        if (row != 0) return {};
        return createIndex(row, column, mDescriptor);
    }

    auto *parentDesc = static_cast<ItemDescriptor*>(parent.internalPointer());
    const auto &kids = parentDesc->GetChildren();
    if (row < 0 || row >= static_cast<int>(kids.size())) return {};
    return createIndex(row, column, kids[row]);
}

QModelIndex ItemDescriptorModel::parent(const QModelIndex &child) const {
    if (!child.isValid()) return {};
    auto *childDesc = static_cast<ItemDescriptor*>(child.internalPointer());
    if (childDesc == mDescriptor) return {};
    auto *parentDesc = childDesc->GetParent();
    if (parentDesc == nullptr) return {};
    return createIndex(RowOf(parentDesc), 0, parentDesc);
}

int ItemDescriptorModel::rowCount(const QModelIndex &parent) const {
    if (mDescriptor == nullptr) return 0;
    if (!parent.isValid()) return 1;
    if (parent.column() != 0) return 0;
    auto *desc = static_cast<ItemDescriptor*>(parent.internalPointer());
    return static_cast<int>(desc->GetChildren().size());
}

int ItemDescriptorModel::columnCount(const QModelIndex &) const {
    return ColumnCount;
}

QVariant ItemDescriptorModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return {};
    auto *desc = static_cast<ItemDescriptor*>(index.internalPointer());
    if (desc == nullptr) return {};

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColumnName: {
            QString name = QString::fromStdString(desc->Name);
            return name.isEmpty() ? QString("(unnamed)") : name;
        }
        case ColumnType: return DescribeType(desc);
        case ColumnId:   return QVariant(static_cast<qlonglong>(desc->Id));
        }
    } else if (role == Qt::ToolTipRole) {
        QString desc_text = QString::fromStdString(desc->Description);
        return desc_text.isEmpty() ? QVariant() : QVariant(desc_text);
    }
    return {};
}

QVariant ItemDescriptorModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
    case ColumnName: return "Name";
    case ColumnType: return "Type";
    case ColumnId:   return "ID";
    }
    return {};
}