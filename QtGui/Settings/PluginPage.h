// === noobWarrior ===
// File: PluginsPage.h
// Started by: Hattozo
// Started on: 12/3/2025
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
class PluginInfobox : public QWidget {
public:
    PluginInfobox(QWidget *parent = nullptr);

};
class PluginPage : public SettingsPage {
public:
    PluginPage(QWidget *parent = nullptr);
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

