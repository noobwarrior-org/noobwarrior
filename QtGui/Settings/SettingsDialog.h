// === noobWarrior ===
// File: SettingsDialog.h
// Started by: Hattozo
// Started on: 3/22/2025
// Description:
#pragma once
#include "SettingsPage.h"

#include <NoobWarrior/Config.h>
#include <QDialog>
#include <QFrame>
#include <QListWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QStackedWidget>

namespace NoobWarrior {
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(QWidget *parent = nullptr);
    void InitWidgets();
    void InitPages();
    void AddPage(SettingsPage *page);
private:
    QListWidget *ListWidget;
    QStackedWidget *StackedWidget;
    std::vector<SettingsPage*> Pages;
};
}
