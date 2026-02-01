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
// File: DatabaseEditor.cpp
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Qt window that lets users view and edit a noobWarrior database
#include "DatabaseEditor.h"
#include "Browser/ItemBrowserWidget.h"
#include "Item/ItemDialog.h"
#include "Item/AssetDialog.h"
#include "BackgroundTask/BackgroundTask.h"
#include "BackgroundTask/BackgroundTaskPopupWidget.h"
#include "BackgroundTask/BackgroundTaskStatusBarWidget.h"
#include "Backup/BackupDialog.h"
#include "../Application.h"
#include "../Dialog/AuthTokenDialog.h"
#include "../Dialog/AboutDialog.h"

#include <NoobWarrior/NoobWarrior.h>

#include <QMenuBar>
#include <QLabel>
#include <QSize>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QToolBar>
#include <QPushButton>
#include <QMimeData>

#include <format>
#include <qnamespace.h>

#define ADD_ITEMTYPE(type, dialogType) \
    QString name = QString::fromStdString(#type); \
    auto insertAction = new QAction(QIcon(""), name, mInsertMenu); \
    insertAction->setObjectName("RequiresDatabaseButton"); \
    mInsertMenu->addAction(insertAction); \
    connect(insertAction, &QAction::triggered, [this]() { \
        dialogType dialog(this); \
        dialog.exec(); \
    }); \
    mInsertItemTypeActions.push_back(insertAction);

using namespace NoobWarrior;

DatabaseEditor::DatabaseEditor(QWidget *parent) : QMainWindow(parent),
    mCurrentDatabase(nullptr),
    mTabWidget(nullptr),
    mWelcomeWidget(nullptr),
    mOverviewWidget(nullptr),
    mItemBrowser(nullptr),
    mFileManager(nullptr),
    mBackgroundTaskStatusBarWidget(nullptr)
{
    setWindowTitle("noobWarrior SDK");
    setAcceptDrops(true);
    // setWindowState(Qt::WindowMaximized);
    InitMenus();
    InitStatusBarWidgets();
    InitWidgets();
}

DatabaseEditor::~DatabaseEditor() {
    if (mCurrentDatabase) {
        mCurrentDatabase->Close();
        NOOBWARRIOR_FREE_PTR(mCurrentDatabase)
    }
}

void DatabaseEditor::Refresh() {
    mItemBrowser->Refresh();
}

void DatabaseEditor::closeEvent(QCloseEvent *event) {
    if (TryToCloseCurrentDatabase()) event->accept(); else event->ignore();
}

void DatabaseEditor::paintEvent(QPaintEvent *event) {
    QMainWindow::paintEvent(event);

    // I don't want to have to create all kinds of hooks and callbacks when our database has been marked dirty
    // So we're just checking if it's dirty here in our paint event that Qt gives us. Much easier
    if (mCurrentDatabase != nullptr) {
        setWindowTitle(
            QString("%1%2%3- noobWarrior SDK")
            .arg(!mCurrentDatabase->IsMemory() ? QString::fromStdString(mCurrentDatabase->GetFileName()) : "Unsaved File")
            .arg(mCurrentDatabase->IsDirty() ? "* " : " ")
            .arg((!mCurrentDatabase->IsMemory() ? "- " : "") + QString::fromStdString(mCurrentDatabase->GetTitle()))
        );
    } else setWindowTitle(QString("noobWarrior SDK"));
}

void DatabaseEditor::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DatabaseEditor::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DatabaseEditor::dropEvent(QDropEvent *event) {
    QMainWindow::dropEvent(event);
    if (event->mimeData()->hasUrls()) {
        for (const QUrl &url : event->mimeData()->urls())
            TryToOpenFile(url.toLocalFile());
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

int DatabaseEditor::TryToCloseCurrentDatabase() {
    QMessageBox::StandardButton res;
    if (mCurrentDatabase != nullptr && !mCurrentDatabase->IsDirty())
        goto close;

    if (mCurrentDatabase != nullptr && mCurrentDatabase->IsDirty()) {
        res = QMessageBox::question( this, nullptr,
            QString("Do you want to save changes to \"%1\"?").arg(QString::fromStdString(mCurrentDatabase->GetTitle())),
            QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
            QMessageBox::Yes);
        if (res == QMessageBox::Cancel) {
            return 0;
        }
        if (res == QMessageBox::Yes)
            mSaveDatabaseAction->trigger();
close:
        mOverviewWidget->deleteLater();
        mCurrentDatabase->Close();
        NOOBWARRIOR_FREE_PTR(mCurrentDatabase)
        mItemBrowser->Refresh();

        DisableRequiredDatabaseButtons(true);
    }
    return 1;
}

void DatabaseEditor::TryToOpenFile(const QString &path) {
    if (!TryToCloseCurrentDatabase()) return;

    // noobWarrior core database API calls
    mCurrentDatabase = new Database(false);
    DatabaseResponse res = mCurrentDatabase->Open(path.toStdString());
    if (res != DatabaseResponse::Success) {
        QMessageBox::critical(this, "Error", QString("Cannot open database \"%1\"\n\nLast Error Received: \"%2\"\nError Code: %3").arg(path, QString::fromStdString(mCurrentDatabase->GetSqliteErrorMsg()), QString::fromStdString(std::format("{:#010x}", (int)res))));
        mCurrentDatabase->Close();
        NOOBWARRIOR_FREE_PTR(mCurrentDatabase)
        return;
    }
    if (mCurrentDatabase->IsMemory()) {
        // If we're making a new file, then fill in some defaults (like the author of the database) with the name of the
        // person running the program.
        if (mCurrentDatabase->GetAuthor().empty()) {
            QString name = qgetenv("USER");
            if (name.isEmpty())
                name = qgetenv("USERNAME");
            mCurrentDatabase->SetAuthor(name.toStdString());
        }

        // It got marked as dirty because we programmatically changed the author of the DB.
        // Unmark it so that you won't get the stupid "you forgot to save your changes" screen when closing this empty new file.
        mCurrentDatabase->UnmarkDirty();
    }

    // our own functions
    mOverviewWidget = new OverviewWidget(mCurrentDatabase);
    mTabWidget->setCurrentIndex(mTabWidget->addTab(mOverviewWidget, mOverviewWidget->windowTitle()));

    mItemBrowser->Refresh();

    DisableRequiredDatabaseButtons(false);
}

Database *DatabaseEditor::GetCurrentlyEditingDatabase() {
    return mCurrentDatabase;
}

ItemBrowserWidget *DatabaseEditor::GetItemBrowser() {
    return mItemBrowser;
}

void DatabaseEditor::InitMenus() {
    mNewDatabaseAction = new QAction(QIcon(":/images/silk/database_add.png"), "New Database");
    mOpenDatabaseAction = new QAction(QIcon(":/images/silk/database_edit.png"), "Open Database");

    mSaveDatabaseAction = new QAction(QIcon(":/images/silk/database_save.png"), "Save Database");
    mSaveDatabaseAction->setObjectName("RequiresDatabaseButton");

    mSaveAsDatabaseAction = new QAction(QIcon(":/images/silk/database_save.png"), "Save Database As...");
    mSaveAsDatabaseAction->setObjectName("RequiresDatabaseButton");

    mBackupAction = new QAction(QIcon(":/images/roblox_backup.png"), "Backup from Roblox");
    mBackupAction->setObjectName("RequiresDatabaseButton");

    mCloseDatabaseAction = new QAction(QIcon(":/images/silk/database_delete.png"), "Close Current Database");
    mCloseDatabaseAction->setObjectName("RequiresDatabaseButton");

#if !defined(__APPLE__) // disable this on mac since it creates fucky behaviors and crashes
    mExitAction = new QAction("Exit");
    mExitAction->setShortcut(QKeySequence("Alt+F4"));
    connect(mExitAction, &QAction::triggered, [this]() {
        close();
    });
#endif

    mNewDatabaseAction->setShortcut(QKeySequence("Ctrl+N"));
    mOpenDatabaseAction->setShortcut(QKeySequence("Ctrl+O"));
    mCloseDatabaseAction->setShortcut(QKeySequence("Ctrl+W"));
    mSaveDatabaseAction->setShortcut(QKeySequence("Ctrl+S"));

    mCloseDatabaseAction->setObjectName("RequiresDatabaseButton");
    mSaveDatabaseAction->setObjectName("RequiresDatabaseButton");
    mSaveAsDatabaseAction->setObjectName("RequiresDatabaseButton");

    mFileMenu = menuBar()->addMenu(tr("&File"));
        mFileMenu->addAction(mNewDatabaseAction);
        mFileMenu->addAction(mOpenDatabaseAction);
    mFileMenu->addSeparator();
        mFileMenu->addAction(mSaveDatabaseAction);
        mFileMenu->addAction(mSaveAsDatabaseAction);
    mFileMenu->addSeparator();
        mFileMenu->addAction(mBackupAction);
    mFileMenu->addSeparator();
        mFileMenu->addAction(mCloseDatabaseAction);
    mFileMenu->addSeparator();
        mFileMenu->addAction(mExitAction);

    mEditMenu = menuBar()->addMenu(tr("&Edit"));

    mItemBrowserViewAction = new QAction(QIcon(":/images/silk/application_view_icons.png"), "Content Browser");
    mFileManagerViewAction = new QAction(QIcon(":/images/silk/folder_page.png"), "File Manager");

    mViewMenu = menuBar()->addMenu(tr("&View"));
    mViewMenu->addAction(mItemBrowserViewAction);
    mViewMenu->addAction(mFileManagerViewAction);

    mInsertMenu = menuBar()->addMenu(tr("&Insert"));
    ADD_ITEMTYPE(Asset, AssetDialog)

    mToolsMenu = menuBar()->addMenu(tr("&Tools"));

    mHelpMenu = menuBar()->addMenu(tr("&Help"));

    mAboutQtButton = new QAction(QIcon(":/images/qt_16x16.png"), "About Qt");
    mAboutQtButton->setMenuRole(QAction::AboutQtRole);
    connect(mAboutQtButton, &QAction::triggered, gApp, &QApplication::aboutQt);

    mAboutButton = new QAction(QIcon(":/images/icon16_aa.png"), "About noobWarrior");
    mAboutButton->setMenuRole(QAction::AboutRole);
    connect(mAboutButton, &QAction::triggered, []() {
        AboutDialog dialog;
        dialog.exec();
    });

    mHelpMenu->addAction(mAboutQtButton);
    mHelpMenu->addAction(mAboutButton);

    connect(mNewDatabaseAction, &QAction::triggered, [&]() {
        TryToOpenFile();
    });

    connect(mOpenDatabaseAction, &QAction::triggered, [&]() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Open Database",
            QString::fromStdString((gApp->GetCore()->GetUserDataDir() / "databases").string()),
            "noobWarrior Database (*.nwdb)"
        );
        if (!filePath.isEmpty()) TryToOpenFile(filePath);
    });

    connect(mCloseDatabaseAction, &QAction::triggered, [&]() {
        TryToCloseCurrentDatabase();
    });

    connect(mSaveDatabaseAction, &QAction::triggered, [&]() {
        if (mCurrentDatabase != nullptr) {
            DatabaseResponse save_res = mCurrentDatabase->WriteChangesToDisk();
            if (save_res != DatabaseResponse::Success) {
                QString save_err_msg;
                switch (save_res) {
                default: save_err_msg = "Is this file read-only?"; break;
                case DatabaseResponse::Busy: save_err_msg = "The database seems to be busy."; break;
                case DatabaseResponse::Misuse: save_err_msg = "There was an internal error."; break;
                }
                QMessageBox::critical(this, "Failed To Save Database", QString("The database could not be saved to disk. %1").arg(save_err_msg));
                return;
            }
            if (mCurrentDatabase->IsMemory()) {
                // This database has never been saved to disk. Make the user pick where they want to store it so we can actually save
                mCurrentDatabase->MarkDirty(); // And mark it dirty again just in case the user rejects the save prompt and nothing actually happens.
                QString filePath = QFileDialog::getSaveFileName(
                    this,
                    "Save Database",
                    QString::fromStdString((gApp->GetCore()->GetUserDataDir() / "databases").string()),
                    "noobWarrior Database (*.nwdb)"
                );
                if (!filePath.isEmpty()) {
                    DatabaseResponse res = mCurrentDatabase->SaveAs(filePath.toStdString());
                    if (res != DatabaseResponse::Success) {
                        QMessageBox::critical(this, "Error", QString("Failed to save database to \"%1\"").arg(filePath));
                        return;
                    }
                    mCurrentDatabase->Close();
                    res = mCurrentDatabase->Open(filePath.toStdString());
                    if (res != DatabaseResponse::Success) {
                        QMessageBox::critical(this, "Error", QString("Failed to re-open database \"%1\"\n\nLast Error Received: \"%2\"\nError Code: %3").arg(filePath, QString::fromStdString(mCurrentDatabase->GetSqliteErrorMsg()), QString::fromStdString(std::format("{:#010x}", static_cast<int>(res)))));
                        return;
                    }
                    mCurrentDatabase->UnmarkDirty();
                }
            }
        }
        repaint(); // trigger a repaint, because we update the window title there.
    });

    connect(mBackupAction, &QAction::triggered, [this]() {
        if (!gApp->GetCore()->GetRobloxAuth()->IsLoggedIn()) {
            QMessageBox::StandardButton res = QMessageBox::question(this,
                "Not Logged In",
                "You currently don't have a Roblox account authenticated with noobWarrior.\n\nStarting April 2nd 2025, Roblox requires an account in order to download any assets from their services. Would you like to authenticate your Roblox account with noobWarrior in order to use this feature?",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
            );
            if (res == QMessageBox::Yes) {
                AuthTokenDialog dialog(this);
                dialog.exec();
            }
        }
        BackupDialog backupDialog(this);
        backupDialog.exec();
    });
}

void DatabaseEditor::InitStatusBarWidgets() {
    mBackgroundTaskPopupWidget = new BackgroundTaskPopupWidget(this);
    mBackgroundTaskPopupWidget->hide();

    mBackgroundTaskStatusBarWidget = new BackgroundTaskStatusBarWidget();
    statusBar()->addPermanentWidget(mBackgroundTaskStatusBarWidget);

    mBackgroundTaskPopupWidget->move(mBackgroundTaskStatusBarWidget->pos());

    mBackgroundTasks.SetStatusBarWidget(mBackgroundTaskStatusBarWidget);
    mBackgroundTasks.SetPopupWidget(mBackgroundTaskPopupWidget);
}

void DatabaseEditor::InitWidgets() {
    mTabWidget = new QTabWidget(this);
    setCentralWidget(mTabWidget);

    /*
    auto *hi = new QLabel("New Database  Ctrl-N\nOpen Database  Ctrl-O");
    hi->setFont(QFont(QApplication::font().family(), 20));
    hi->setAlignment(Qt::AlignCenter);
    hi->setWordWrap(true);

    mTabWidget->addTab(hi, "Welcome");
    */

    mWelcomeWidget = new WelcomeWidget();
    mTabWidget->addTab(mWelcomeWidget, "Welcome");

    mStandardToolBar = new QToolBar("Standard", this);
    // mFileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    mStandardToolBar->addAction(mNewDatabaseAction);
    mStandardToolBar->addAction(mOpenDatabaseAction);
    mStandardToolBar->addAction(mSaveDatabaseAction);
    mStandardToolBar->addAction(mCloseDatabaseAction);
    mStandardToolBar->addSeparator();
    mStandardToolBar->addAction(mBackupAction);

    mViewToolBar = new QToolBar("View", this);
    mViewToolBar->addAction(mItemBrowserViewAction);
    mViewToolBar->addAction(mFileManagerViewAction);

    mInsertToolBar = new QToolBar("Insert", this);
    mInsertToolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    for (QAction* itemTypeAction : mInsertItemTypeActions) {
        mInsertToolBar->addAction(itemTypeAction);
    }
    // ADD_ID_TYPE(Asset, ":/images/silk/page_add.png")
    // ADD_ID_TYPE(Badge, ":/images/silk/medal_gold_add.png")
    // ADD_ID_TYPE(User, ":/images/silk/user_add.png")

    DisableRequiredDatabaseButtons(true);

    addToolBar(Qt::ToolBarArea::TopToolBarArea, mStandardToolBar);
    // addToolBarBreak();
    addToolBar(Qt::ToolBarArea::TopToolBarArea, mViewToolBar);
    addToolBar(Qt::ToolBarArea::TopToolBarArea, mInsertToolBar);

    mItemBrowser = new ItemBrowserWidget(this);
    mItemBrowser->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::LeftDockWidgetArea, mItemBrowser);

    mFileManager = new FileManagerWidget(this);
    mFileManager->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::LeftDockWidgetArea, mFileManager);
}

void DatabaseEditor::DisableRequiredDatabaseButtons(bool val) {
    for (auto button : findChildren<QAction*>("RequiresDatabaseButton"))
        button->setDisabled(val); // Disable all buttons that require a database since one isn't loaded right now

    for (auto button : menuBar()->findChildren<QAction*>("RequiresDatabaseButton"))
        button->setDisabled(val); // Disable all buttons that require a database since one isn't loaded right now

    for (auto button : mFileMenu->actions()) {
        if (button->objectName().contains("RequiresDatabaseButton"))
            button->setDisabled(val); // Disable all buttons that require a database since one isn't loaded right now
    }
}