// === noobWarrior ===
// File: ContentBrowserWidget.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Dockable Qt widget that allows the user to explore the contents of an archive in an easily-digestible format
#pragma once

#include <QDockWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>

namespace NoobWarrior {
    class ContentBrowserWidget : public QDockWidget {
        Q_OBJECT

    public:
        ContentBrowserWidget(QWidget *parent = nullptr);
        ~ContentBrowserWidget();

        void Refresh();
    private:
        void InitWidgets();
        void InitPageCounter();
        void GoToPage(int num);
        QWidget *MainWidget;
        QVBoxLayout *MainLayout;
        QLineEdit *SearchBox;
        QListWidget *List;
        QLabel *NoDatabaseFoundLabel;
    };
}
