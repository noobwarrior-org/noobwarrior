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