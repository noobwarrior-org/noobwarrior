// === noobWarrior ===
// File: FileManagerWidget.h
// Started by: Hattozo
// Started on: 7/2/2025
// Description:
#pragma once
#include <QDockWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QString>

namespace NoobWarrior {
class FileManagerWidget : public QDockWidget {
    Q_OBJECT
public:
    FileManagerWidget(QWidget *parent = nullptr);
    void Refresh(const QString &address = "/");
private:
    void InitWidgets();
    
    QWidget* MainWidget;
    QVBoxLayout* MainLayout;

    QLineEdit* AddressBar;
};
}
