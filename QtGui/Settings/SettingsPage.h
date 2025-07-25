// === noobWarrior ===
// File: SettingsPage.h
// Started by: Hattozo
// Started on: 7/2/2025
// Description:
#pragma once
#include <QFormLayout>
#include <QFrame>
#include <QVBoxLayout>
#include <QWidget>

namespace NoobWarrior {
class SettingsPage : public QWidget {
    Q_OBJECT
public:
    SettingsPage(QWidget *parent = nullptr);
    void Init();
    virtual const QString GetTitle() = 0;
    virtual const QString GetDescription() = 0;
    virtual const QIcon GetIcon() = 0;
protected:
    QVBoxLayout *Layout;
};
}
