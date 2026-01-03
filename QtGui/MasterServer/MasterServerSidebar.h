// === noobWarrior ===
// File: MasterServerSidebar.h
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Widget that contains a list of master servers
#pragma once
#include <QDockWidget>
#include <QTreeView>
#include <QStandardItemModel>

namespace NoobWarrior {
class MasterServerSidebar : public QDockWidget {
    Q_OBJECT
public:
    MasterServerSidebar(QWidget* parent = nullptr);
private:
    void InitWidgets();

    QTreeView* mView;
    QStandardItemModel* mModel;
};
}