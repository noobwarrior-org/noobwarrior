// === noobWarrior ===
// File: DatabasePage.h
// Started by: Hattozo
// Started on: 7/2/2025
// Description:
#pragma once
#include "SettingsPage.h"

#include <QWidget>
#include <QGridLayout>
#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>

namespace NoobWarrior {
class DatabasePage : public SettingsPage {
    Q_OBJECT
public:
    DatabasePage(QWidget *parent = nullptr);
    void InitWidgets();
    const QString GetTitle() override;
    const QString GetDescription() override;
    const QIcon GetIcon() override;
private:
    QGridLayout* mGridLayout;

    QFrame* mAvailableFrame;
    QVBoxLayout* mAvailableLayout;
    QLabel* mAvailableLabel;
    QListWidget* mAvailableList;

    QFrame* mSelectedFrame;
    QVBoxLayout* mSelectedLayout;
    QLabel* mSelectedLabel;
    QListWidget* mSelectedList;
};
}
