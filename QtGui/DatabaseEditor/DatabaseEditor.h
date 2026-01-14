/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: DatabaseEditor.h
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Qt window that lets users view and edit a noobWarrior database
#pragma once
#include <NoobWarrior/Database/Database.h>

#include "BackgroundTask/BackgroundTask.h"
#include "WelcomeWidget.h"
#include "OverviewWidget.h"
#include "FileManagerWidget.h"

#include <QMainWindow>
#include <QMenu>
#include <QObject>
#include <QToolBar>
#include <QStatusBar>
#include <QScrollArea>

namespace NoobWarrior {
    class ItemBrowserWidget;
    class DatabaseEditor : public QMainWindow {
        Q_OBJECT
        
    public:
        DatabaseEditor(QWidget *parent = nullptr);
        ~DatabaseEditor();

        /**
         * @brief Refreshes all widgets that may have dirty data in them
         */
        void Refresh();

        int TryToCloseCurrentDatabase();
        void TryToOpenFile(const QString &path = ":memory:");

        Database *GetCurrentlyEditingDatabase();

        ItemBrowserWidget *GetItemBrowser();
    protected:
        void closeEvent(QCloseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

        void dragEnterEvent(QDragEnterEvent *event) override;
        void dragMoveEvent(QDragMoveEvent *event) override;
        void dropEvent(QDropEvent *event) override;

        void DisableRequiredDatabaseButtons(bool val);
    private:
        void InitMenus();
        void InitStatusBarWidgets();
        void InitWidgets();

        //////////////////// Menu Bar ////////////////////
        QMenu *mFileMenu;
        QMenu *mEditMenu;
        QMenu *mViewMenu;
        QMenu *mInsertMenu;
        QMenu *mToolsMenu;
        QMenu *mHelpMenu;

        QAction* mNewDatabaseAction;
        QAction* mOpenDatabaseAction;
        
        QAction* mSaveDatabaseAction;
        QAction* mSaveAsDatabaseAction;

        QAction* mCloseDatabaseAction;

        QAction* mBackupAction;

        QAction* mExitAction;

        QAction* mItemBrowserViewAction;
        QAction* mFileManagerViewAction;

        std::vector<QAction*> mInsertItemTypeActions;

        //////////////////// Tool Bars ////////////////////
        QToolBar *mStandardToolBar;
        QToolBar *mViewToolBar;
        QToolBar *mInsertToolBar;

        //////////////////// Tabs ////////////////////
        QTabWidget *mTabWidget;

        WelcomeWidget* mWelcomeWidget;
        OverviewWidget *mOverviewWidget;

        //////////////////// Dockable Widgets ////////////////////
        ItemBrowserWidget *mItemBrowser;
        FileManagerWidget *mFileManager;

        Database *mCurrentDatabase;

        //////////////////// Status Bar ////////////////////
        BackgroundTasks mBackgroundTasks;
        BackgroundTaskStatusBarWidget *mBackgroundTaskStatusBarWidget;
        BackgroundTaskPopupWidget *mBackgroundTaskPopupWidget;
    };
}
