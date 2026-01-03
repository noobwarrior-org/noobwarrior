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