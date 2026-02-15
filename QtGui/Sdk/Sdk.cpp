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
// File: Sdk.cpp
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Qt window that lets users view and edit noobWarrior databases and plugins
#include "Sdk.h"
#include "NoobWarrior/SqlDb/SqlDb.h"
#include "Sdk/Project/EmuDb/EmuDbProject.h"
#include "Sdk/Item/Browser/ItemBrowserWidget.h"
#include "Sdk/Item/ItemDialog.h"
#include "Sdk/Item/AssetDialog.h"
#include "Sdk/Backup/BackupDialog.h"
#include "Sdk/Project/Wizard/ProjectWizard.h"
#include "Sdk/BackgroundTask/BackgroundTask.h"
#include "Sdk/BackgroundTask/BackgroundTaskPopupWidget.h"
#include "Sdk/BackgroundTask/BackgroundTaskStatusBarWidget.h"
#include "Application.h"
#include "Dialog/AuthTokenDialog.h"
#include "Dialog/AboutDialog.h"

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
#include <fstream>
#include <qnamespace.h>

#define ADD_ITEMTYPE(type, dialogType) \
    QString type##_Name = QString::fromStdString(#type); \
    auto type##_InsertAction = new QAction(QIcon(""), type##_Name, mInsertMenu); \
    type##_InsertAction->setObjectName("RequireProjectButton"); \
    mInsertMenu->addAction(type##_InsertAction); \
    connect(type##_InsertAction, &QAction::triggered, [this]() { \
        auto *dbProj = dynamic_cast<EmuDbProject*>(mFocusedProject); \
        if (dbProj != nullptr) { \
            dialogType dialog(this); \
            dialog.exec(); \
        } else QMessageBox::critical(this, "Cannot Insert Item", "The current project is not a valid database.", QMessageBox::Ok); \
    }); \
    mInsertItemTypeActions.push_back(type##_InsertAction);

using namespace NoobWarrior;

Sdk::Sdk(QWidget *parent) : QMainWindow(parent),
    mFocusedProject(nullptr),
    mTabWidget(nullptr),
    mWelcomeWidget(nullptr),
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

Sdk::~Sdk() {
    for (Project* proj : mProjects) {
        bool success = RemoveProject(proj);
        if (success) {
            NOOBWARRIOR_FREE_PTR(proj)
        }
    }
}

void Sdk::closeEvent(QCloseEvent *event) {
    bool refusedCancelOnOneProject = false;
    for (Project* proj : mProjects) {
        if (TryToRemoveProject(proj)) {
            NOOBWARRIOR_FREE_PTR(proj)
        } else {
            refusedCancelOnOneProject = true;
            break;
        }
    }
    !refusedCancelOnOneProject ? event->accept() : event->ignore();
}

void Sdk::paintEvent(QPaintEvent *event) {
    QMainWindow::paintEvent(event);

    if (mFocusedProject != nullptr) {
        QString title = mFocusedProject->GetTitle() + (mFocusedProject->IsDirty() ? "*" : "");
        setWindowTitle(
            QString("%1 - noobWarrior SDK")
            .arg(title)
        );
        int tabIndex = mTabWidget->indexOf(mFocusedProject->mTabWidget);
        if (tabIndex != -1) {
            mTabWidget->setTabText(tabIndex, title);
        }
    } else setWindowTitle(QString("noobWarrior SDK"));
}

void Sdk::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void Sdk::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void Sdk::dropEvent(QDropEvent *event) {
    QMainWindow::dropEvent(event);
    if (event->mimeData()->hasUrls()) {
        for (const QUrl &url : event->mimeData()->urls())
            AddProjectFromPath(std::filesystem::path(url.toLocalFile().toStdString()));
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

bool Sdk::AddProject(Project* project) {
    if (project == nullptr)
        return false;

    auto it = std::find(mProjects.begin(), mProjects.end(), project);
    if (it != mProjects.end()) {
        Out("Sdk", "Tried adding project but it is already parented to the SDK!");
        return false;
    }

    if (project->Fail()) {
        Out("Sdk", "Tried adding project but it is in fail mode! Msg: \"{}\"", project->GetOpenFailMsg().toStdString());
        QMessageBox::critical(this, "Failed To Open Project", QString("The project could not be opened.\n%1").arg(project->GetOpenFailMsg()));
        return false;
    }
    project->mSdk = this;

    mProjects.push_back(project);
    mFocusedProject = project;

    mTabWidget->setCurrentIndex(mTabWidget->addTab(project->mTabWidget, project->GetTitle()));
    project->OnShown();
    Refresh();
    return true;
}

bool Sdk::AddProjectFromPath(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (file.fail())
        return false;
    std::string header(6, '\0');
    file.read(header.data(), 6);
    file.close();
    if (header.compare("SQLite") == 0) {
        auto *project = new EmuDbProject(path);
        AddProject(project);
        return true;
    }
    QMessageBox::critical(this, "Cannot Open Project", "The file could not be read as a valid project.", QMessageBox::Ok);
    return false;
}

/* Note: this function doesnt free it from memory. Call the C++ destructor on the project itself if you want that */
bool Sdk::RemoveProject(Project* project) {
    if (project == nullptr)
        return false;

    auto it = std::find(mProjects.begin(), mProjects.end(), project);
    if (it == mProjects.end()) {
        Out("Sdk", "Tried removing project but it isn't parented to the SDK!");
        return false;
    }
    mProjects.erase(it);

    project->OnHidden();
    int index = mTabWidget->indexOf(project->mTabWidget);
    if (index != -1) {
        mTabWidget->removeTab(index);
    }

    if (project == mFocusedProject)
        mFocusedProject = nullptr;

    Refresh();
    return true;
}

/* Note: this function doesnt free it from memory. Call the C++ destructor on the project itself if you want that */
bool Sdk::TryToRemoveProject(Project* project) {
    if (project == nullptr)
        return false;

    QMessageBox::StandardButton res;
    if (!project->IsDirty())
        goto close;
    else {
        res = QMessageBox::question( this, nullptr,
            QString("Do you want to save changes to \"%1\"?").arg(project->GetTitle()),
            QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
            QMessageBox::Yes);
        if (res == QMessageBox::Cancel) {
            return false;
        }
        if (res == QMessageBox::Yes)
            mSaveProjectAction->trigger();
close:
        RemoveProject(project);
        return true;
    }
}

bool Sdk::SaveProject(Project* project) {
    if (project == nullptr)
        return false;

    bool success = project->Save();
    if (!success) {
        QString msg = project->GetSaveFailMsg();
        QMessageBox::critical(this, "Failed To Save Database", QString("The database could not be saved to disk. %1").arg(msg));
        return false;
    }
    return success;
}

bool Sdk::TryToRemoveFocusedProject() {
    return TryToRemoveProject(mFocusedProject);
}

bool Sdk::SaveFocusedProject() {
    return SaveProject(mFocusedProject);
}

Project* Sdk::GetFocusedProject() {
    return mFocusedProject;
}

void Sdk::Refresh() {
    if (mTabWidget != nullptr) {
        QWidget* widget = mTabWidget->widget(mTabWidget->currentIndex());
        QVariant qvariant = widget->property("Project");
        Project* project = !qvariant.isNull() ? qvariant.value<Project*>() : nullptr;

        mFocusedProject = !qvariant.isNull() ? qvariant.value<Project*>() : nullptr;
        repaint();
    }

    auto *dbProj = dynamic_cast<EmuDbProject*>(mFocusedProject);
    if (dbProj != nullptr) {
        DisableRequiredProjectButtons(false);
    } else DisableRequiredProjectButtons(true);

    if (mItemBrowser != nullptr)
        mItemBrowser->Refresh();
}

ItemBrowserWidget *Sdk::GetItemBrowser() {
    return mItemBrowser;
}

void Sdk::InitMenus() {
    mNewProjectAction = new QAction(QIcon(":/images/silk/page_white_add.png"), "New Project");
    mOpenProjectAction = new QAction(QIcon(":/images/silk/page_white_edit.png"), "Open Project");

    mSaveProjectAction = new QAction(QIcon(":/images/silk/page_white_copy.png"), "Save Project");
    mSaveProjectAction->setObjectName("RequireProjectButton");

    mSaveAsProjectAction = new QAction(QIcon(":/images/silk/page_white_copy.png"), "Save Project As...");
    mSaveAsProjectAction->setObjectName("RequireProjectButton");

    mBackupAction = new QAction(QIcon(":/images/roblox_backup.png"), "Backup from Roblox");
    mBackupAction->setObjectName("RequireProjectButton");

    mCloseProjectAction = new QAction(QIcon(":/images/silk/page_white_delete.png"), "Close Current Project");
    mCloseProjectAction->setObjectName("RequireProjectButton");

#if !defined(__APPLE__) // disable this on mac since it creates fucky behaviors and crashes
    mExitAction = new QAction("Exit");
    mExitAction->setShortcut(QKeySequence("Alt+F4"));
    connect(mExitAction, &QAction::triggered, [this]() {
        close();
    });
#endif

    mNewProjectAction->setShortcut(QKeySequence("Ctrl+N"));
    mOpenProjectAction->setShortcut(QKeySequence("Ctrl+O"));
    mCloseProjectAction->setShortcut(QKeySequence("Ctrl+W"));
    mSaveProjectAction->setShortcut(QKeySequence("Ctrl+S"));

    mCloseProjectAction->setObjectName("RequireProjectButton");
    mSaveProjectAction->setObjectName("RequireProjectButton");
    mSaveAsProjectAction->setObjectName("RequireProjectButton");

    mFileMenu = menuBar()->addMenu(tr("&File"));
        mFileMenu->addAction(mNewProjectAction);
        mFileMenu->addAction(mOpenProjectAction);
    mFileMenu->addSeparator();
        mFileMenu->addAction(mSaveProjectAction);
        mFileMenu->addAction(mSaveAsProjectAction);
    mFileMenu->addSeparator();
        mFileMenu->addAction(mBackupAction);
    mFileMenu->addSeparator();
        mFileMenu->addAction(mCloseProjectAction);
    mFileMenu->addSeparator();
        mFileMenu->addAction(mExitAction);

    mEditMenu = menuBar()->addMenu(tr("&Edit"));

    mItemBrowserViewAction = new QAction(QIcon(":/images/silk/application_view_icons.png"), "Content Browser");
    mFileManagerViewAction = new QAction(QIcon(":/images/silk/folder_page.png"), "File Manager");

    mViewMenu = menuBar()->addMenu(tr("&View"));
    mViewMenu->addAction(mItemBrowserViewAction);
    mViewMenu->addAction(mFileManagerViewAction);

    mProjectMenu = menuBar()->addMenu(tr("&Project"));

    mInsertMenu = menuBar()->addMenu(tr("&Insert"));
    ADD_ITEMTYPE(Asset, AssetDialog)

    mToolsMenu = menuBar()->addMenu(tr("&Tools"));

    mPluginsMenu = menuBar()->addMenu(tr("&Plugins"));

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

    connect(mNewProjectAction, &QAction::triggered, [&]() {
        // TryToOpenFile();
        ProjectWizard wizard(this);
        wizard.exec();
    });

    connect(mOpenProjectAction, &QAction::triggered, [&]() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Open Database",
            QString::fromStdString((gApp->GetCore()->GetUserDataDir() / "databases").string()),
            "noobWarrior Database (*.nwdb)"
        );
        if (!filePath.isEmpty()) AddProjectFromPath(std::filesystem::path(filePath.toStdString()));
    });

    connect(mCloseProjectAction, &QAction::triggered, [&]() {
        Project* focused = mFocusedProject; // save reference to pointer because removing it will set mFocusedProject to nullptr without freeing it
        TryToRemoveFocusedProject();
        NOOBWARRIOR_FREE_PTR(focused)
    });

    connect(mSaveProjectAction, &QAction::triggered, [&]() {
        SaveFocusedProject();
        repaint(); // trigger a repaint, because we update the window title there.
    });

    connect(mBackupAction, &QAction::triggered, [this]() {
        if (!gApp->GetCore()->GetRbxKeychain()->IsLoggedIn()) {
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

void Sdk::InitStatusBarWidgets() {
    mBackgroundTaskPopupWidget = new BackgroundTaskPopupWidget(this);
    mBackgroundTaskPopupWidget->hide();

    mBackgroundTaskStatusBarWidget = new BackgroundTaskStatusBarWidget();
    statusBar()->addPermanentWidget(mBackgroundTaskStatusBarWidget);

    mBackgroundTaskPopupWidget->move(mBackgroundTaskStatusBarWidget->pos());

    mBackgroundTasks.SetStatusBarWidget(mBackgroundTaskStatusBarWidget);
    mBackgroundTasks.SetPopupWidget(mBackgroundTaskPopupWidget);
}

void Sdk::InitWidgets() {
    mTabWidget = new QTabWidget(this);
    setCentralWidget(mTabWidget);

    connect(mTabWidget, &QTabWidget::currentChanged, [this](int index) {
        Refresh();
    });

    mWelcomeWidget = new WelcomeWidget();
    mTabWidget->addTab(mWelcomeWidget, "Welcome");

    mStandardToolBar = new QToolBar("Standard", this);
    // mFileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    mStandardToolBar->addAction(mNewProjectAction);
    mStandardToolBar->addAction(mOpenProjectAction);
    mStandardToolBar->addAction(mSaveProjectAction);
    mStandardToolBar->addAction(mCloseProjectAction);
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

    DisableRequiredProjectButtons(true);

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

void Sdk::DisableRequiredProjectButtons(bool val) {
    for (auto button : findChildren<QAction*>("RequireProjectButton"))
        button->setDisabled(val); // Disable all buttons that require a database since one isn't loaded right now

    for (auto button : menuBar()->findChildren<QAction*>("RequireProjectButton"))
        button->setDisabled(val); // Disable all buttons that require a database since one isn't loaded right now

    for (auto button : mFileMenu->actions()) {
        if (button->objectName().contains("RequireProjectButton"))
            button->setDisabled(val); // Disable all buttons that require a database since one isn't loaded right now
    }
}