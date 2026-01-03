// === noobWarrior ===
// File: OverviewWidget.h
// Started by: Hattozo
// Started on: 7/4/2025
// Description:
#pragma once
#include <QVBoxLayout>
#include <QGridLayout>
#include <NoobWarrior/Database/Database.h>
#include <QWidget>

namespace NoobWarrior {
class OverviewWidget : public QWidget {
    Q_OBJECT
public:
    OverviewWidget(Database *db, QWidget *parent = nullptr);
private:
    void InitWidgets();
    Database *mDatabase;
    QVBoxLayout *ToplevelLayout;
    QGridLayout *ContentLayout;
};
}
