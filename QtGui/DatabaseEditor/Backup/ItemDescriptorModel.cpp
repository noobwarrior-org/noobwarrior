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
// Description: Qt model that works with ItemDescriptor
#include "ItemDescriptorModel.h"

using namespace NoobWarrior;

ItemDescriptorModel::ItemDescriptorModel(ItemDescriptor* desc) :
    mDescriptor(desc),
    mIsUsingAlreadyAllocatedDesc(true)
{}

ItemDescriptorModel::ItemDescriptorModel() :
    mIsUsingAlreadyAllocatedDesc(false)
{
    mDescriptor = ItemDescriptor_New();
}

ItemDescriptorModel::~ItemDescriptorModel() {
    if (!mIsUsingAlreadyAllocatedDesc)
        ItemDescriptor_Destroy(mDescriptor);
}

QModelIndex ItemDescriptorModel::index(int row, int column, const QModelIndex &parent) const {
    
}

QModelIndex ItemDescriptorModel::parent(const QModelIndex &child) const {

}

int ItemDescriptorModel::rowCount(const QModelIndex &parent) const {

}

int ItemDescriptorModel::columnCount(const QModelIndex &parent) const {

}

QVariant ItemDescriptorModel::data(const QModelIndex &index, int role) const {

}