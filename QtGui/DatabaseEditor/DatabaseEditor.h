// === noobWarrior ===
// File: DatabaseEditor.h
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Qt window that lets users view and edit a noobWarrior database
#pragma once

#include <NoobWarrior/Database/Database.h>

#include "ContentBrowserWidget.h"

#include <QMainWindow>
#include <QMenu>
#include <QObject>
#include <QToolBar>

namespace NoobWarrior {
    class DatabaseEditor : public QMainWindow {
        Q_OBJECT
        
    public:
        DatabaseEditor(QWidget *parent = nullptr);
        ~DatabaseEditor();

        void TryToCreateFile(const QString &path);
        void TryToOpenFile(const QString &path = ":memory:");

        Database *GetCurrentlyEditingDatabase();
    protected:
        void closeEvent(QCloseEvent *event) override;
    private:
        void InitMenus();
        void InitWidgets();

        QMenu *mFileMenu;
        QToolBar *mFileToolBar;
        QToolBar *mViewToolBar;
        QToolBar *mInsertToolBar;
        QTabWidget *mTabWidget;

        Database *mCurrentDatabase;
    };
}