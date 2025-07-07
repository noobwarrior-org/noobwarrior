// === noobWarrior ===
// File: DatabaseEditor.h
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Qt window that lets users view and edit a noobWarrior database
#pragma once

#include <NoobWarrior/Database/Database.h>

#include "OverviewWidget.h"
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

        int TryToCloseCurrentDatabase();
        void TryToOpenFile(const QString &path = ":memory:");

        Database *GetCurrentlyEditingDatabase();
    protected:
        void closeEvent(QCloseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;
    private:
        void InitMenus();
        void InitWidgets();

        //////////////////// Menu Bar ////////////////////
        QMenu *mFileMenu;

        QAction *mNewDatabaseAction;
        QAction *mOpenDatabaseAction;
        QAction *mCloseDatabaseAction;
        QAction *mSaveDatabaseAction;
        QAction *mSaveAsDatabaseAction;

        //////////////////// Tool Bars ////////////////////
        QToolBar *mFileToolBar;
        QToolBar *mViewToolBar;
        QToolBar *mInsertToolBar;
        QTabWidget *mTabWidget;
        OverviewWidget *mOverviewWidget;
        ContentBrowserWidget *mContentBrowser;

        Database *mCurrentDatabase;
    };
}