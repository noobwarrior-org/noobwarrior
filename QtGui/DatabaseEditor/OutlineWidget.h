// === noobWarrior ===
// File: OutlineWidget.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Dockable Qt widget that allows the user to explore the contents of an archive in an easily-digestible format
#pragma once

#include <QDockWidget>
#include <QLineEdit>

namespace NoobWarrior {
    class OutlineWidget : public QDockWidget {
        Q_OBJECT

    public:
        OutlineWidget(QWidget *parent = nullptr);
        ~OutlineWidget();
    private:
        QLineEdit *mSearchBox;
    };
}