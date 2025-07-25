// === noobWarrior ===
// File: OrganizerWidget.h
// Started by: Hattozo
// Started on: 7/2/2025
// Description:
#pragma once
#include <QDockWidget>
#include <QVBoxLayout>

namespace NoobWarrior {
class OrganizerWidget : public QDockWidget {
    Q_OBJECT
public:
    OrganizerWidget(QWidget *parent = nullptr);
private:
    void InitWidgets();

    QWidget* MainWidget;
    QVBoxLayout* MainLayout;
};
}
