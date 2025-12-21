// === noobWarrior ===
// File: PluginDialog.h
// Started by: Hattozo
// Started on: 12/8/2025
// Description:
#pragma once
#include <NoobWarrior/Plugin.h>

#include <QDialog>
#include <QVBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>

namespace NoobWarrior {
class PluginDialog : public QDialog {
    Q_OBJECT
public:
    PluginDialog(QWidget *parent = nullptr);
    void InitWidgets();
private:
    QVBoxLayout *mLayout;
    QTreeView *mView;
    QStandardItemModel *mModel;
};
}