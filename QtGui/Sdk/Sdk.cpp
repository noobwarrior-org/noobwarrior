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
#include "Sdk/Backup/BackupDialog.h"
#include "Sdk/Backup/BackupTask.h"
#include "Sdk/Item/ItemOpenSaveDialog.h"
#include "Sdk/Studio/RobloxFilePreviewController.h"
#include "Sdk/Studio/RobloxFilePreviewWindow.h"
#include "Sdk/Project/Wizard/ProjectWizard.h"
#include "Application.h"
#include "Dialog/AuthTokenDialog.h"
#include "Dialog/AboutDialog.h"
#include "ExecuteSqlDialog.h"

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

#define ADD_ITEMTYPE(typeName, iconStr, itemType) \
    QString typeName##_Name = QString::fromStdString(#typeName); \
    auto typeName##_InsertAction = new QAction(QIcon(iconStr), typeName##_Name, mInsertMenu); \
    typeName##_InsertAction->setObjectName("RequireProjectButton"); \
    mInsertMenu->addAction(typeName##_InsertAction); \
    connect(typeName##_InsertAction, &QAction::triggered, [this]() { \
        auto *dbProj = dynamic_cast<EmuDbProject*>(mFocusedProject); \
        if (dbProj != nullptr) { \
            ItemDialog dialog(dbProj->GetDb(), itemType, std::nullopt, this); \
            dialog.exec(); \
        } else QMessageBox::critical(this, "Cannot Insert Item", "The current project is not a valid database.", QMessageBox::Ok); \
    }); \
    mInsertItemTypeActions.push_back(typeName##_InsertAction);

using namespace NoobWarrior;

static QString GetIconFileFromItemType(ItemType type) {
    switch (type) {
    case ItemType::Asset: return ":/images/silk/brick_add.png";
    case ItemType::Badge: return ":/images/silk/medal_gold_add.png";
    case ItemType::Bundle: return ":/images/silk/package_add.png";
    case ItemType::DevProduct: return ":/images/silk/key_add.png";
    case ItemType::Group: return ":/images/silk/group.png";
    case ItemType::Outfit: return ":/images/silk/user_female.png";
    case ItemType::Pass: return ":/images/silk/vcard_add.png";
    case ItemType::Set: return ":/images/silk/bricks.png";
    case ItemType::Universe: return ":/images/silk/world_add.png";
    case ItemType::User: return ":/images/silk/user_add.png";
    default: return ":/images/silk/page_white.png";
    }
};

Sdk::Sdk(QWidget *parent) : QMainWindow(parent),
    mFocusedProject(nullptr),
    mTabWidget(nullptr),
    mWelcomeWidget(nullptr),
    mItemBrowser(nullptr),
    mFileManager(nullptr),
    mNotifications(new NotificationManager(this))
{
    setWindowTitle("noobWarrior SDK");
    setAcceptDrops(true);
    // setWindowState(Qt::WindowMaximized);
    InitMenus();
    InitWidgets();

    statusBar()->setContentsMargins(0, 0, 0, 0);
    statusBar()->addPermanentWidget(new NotificationHistoryButton(mNotifications, this));
}

Sdk::~Sdk() {
    // Backup tasks write into project databases from worker threads; tear them down (their
    // destructor cancels and drains the worker) before the databases they target are freed.
    for (BackupTask* task : findChildren<BackupTask*>())
        delete task;

    // RemoveProject erases from mProjects, so iterate a copy rather than the live vector.
    std::vector<Project*> projects = mProjects;
    for (Project* proj : projects) {
        bool success = RemoveProject(proj);
        if (success) {
            NOOBWARRIOR_FREE_PTR(proj)
        }
    }
}

void Sdk::closeEvent(QCloseEvent *event) {
    // Dirty previews first: destruction delivers no closeEvent to prompt through.
    if (mStudioPreview != nullptr && !mStudioPreview->ConfirmCloseAll()) {
        event->ignore();
        return;
    }
    for (RobloxFilePreviewWindow* previewWindow :
         findChildren<RobloxFilePreviewWindow*>(Qt::FindDirectChildrenOnly)) {
        if (!previewWindow->close()) {
            event->ignore();
            return;
        }
    }

    bool refusedCancelOnOneProject = false;
    // TryToRemoveProject erases from mProjects, so iterate a copy rather than the live vector.
    std::vector<Project*> projects = mProjects;
    for (Project* proj : projects) {
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

    AddRecentProject(project->GetFilePath());

    mTabWidget->setCurrentIndex(mTabWidget->addTab(project->mTabWidget, project->GetTitle()));
    project->OnShown();
    Refresh();
    return true;
}

bool Sdk::AddProjectFromPath(const std::filesystem::path &path) {
    if (Project* existing = GetProjectFromPath(path); existing != nullptr) {
        FocusProject(existing);
        return true;
    }

    std::ifstream file(path, std::ios::binary);
    if (file.fail())
        return false;
    std::string header(6, '\0');
    file.read(header.data(), 6);
    file.close();
    if (header.compare("SQLite") == 0) {
        auto *project = new EmuDbProject(path.string());
        if (!AddProject(project)) {
            NOOBWARRIOR_FREE_PTR(project)
            return false;
        }
        return true;
    }
    QMessageBox::critical(this, "Cannot Open Project", "The file could not be read as a valid project.", QMessageBox::Ok);
    return false;
}

Project* Sdk::GetProjectFromPath(const std::filesystem::path &path) {
    if (path.empty())
        return nullptr;

    for (Project* project : mProjects) {
        std::filesystem::path openPath = project->GetFilePath();
        if (openPath.empty())
            continue;
        std::error_code ec;
        if (std::filesystem::equivalent(openPath, path, ec))
            return project;
        if (ec && openPath == path)
            return project;
    }
    return nullptr;
}

bool Sdk::FocusProject(Project* project) {
    if (project == nullptr || mTabWidget == nullptr)
        return false;
    int index = mTabWidget->indexOf(project->mTabWidget);
    if (index == -1)
        return false;
    mTabWidget->setCurrentIndex(index);
    Refresh();
    return true;
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

    // A backup still running against this project's database must not outlive it; deleting the
    // task cancels the run and drains its worker before we let go of the project.
    for (BackupTask* task : findChildren<BackupTask*>())
        if (task->GetProject() == project)
            delete task;

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

    // Hosted script editors die with the project; IsDirty() knows nothing about their text.
    for (SourceEditorContainer* editor :
         project->GetTabWidget()->findChildren<SourceEditorContainer*>()) {
        if (!editor->IsDirty())
            continue;
        if (QMessageBox::question(this, nullptr,
                QString("\"%1\" hosts script editors with unapplied changes that will be lost. "
                        "Close anyway?").arg(project->GetTitle()),
                QMessageBox::Close | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Close)
            return false;
        break;
    }

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
            SaveProject(project);
close:
        RemoveProject(project);
        return true;
    }
}

bool Sdk::IsProjectOpen(Project* project) {
    return project != nullptr &&
           std::find(mProjects.begin(), mProjects.end(), project) != mProjects.end();
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
        mFocusedProject = project;
        repaint();
    }

    auto *dbProj = dynamic_cast<EmuDbProject*>(mFocusedProject);
    if (dbProj != nullptr) {
        DisableRequiredProjectButtons(false);
    } else DisableRequiredProjectButtons(true);

    if (mItemBrowser != nullptr)
        mItemBrowser->Refresh();

    if (mFileManager != nullptr)
        mFileManager->Reload();
}

ItemBrowserWidget *Sdk::GetItemBrowser() {
    return mItemBrowser;
}

NotificationManager *Sdk::GetNotifications() {
    return mNotifications;
}

QAction* Sdk::GetNewProjectAction() {
    return mNewProjectAction;
}

QAction* Sdk::GetOpenProjectAction() {
    return mOpenProjectAction;
}

QAction* Sdk::GetBackupAction() {
    return mBackupAction;
}

QStringList Sdk::GetRecentProjects() {
    QStringList paths;
    auto recent = gApp->GetCore()->GetRegistry()->GetKeyValue<sol::table>("sdk.recent_projects");
    if (!recent.has_value())
        return paths;
    for (std::size_t index = 1; index <= recent->size(); index++) {
        sol::optional<std::string> path = recent->get<sol::optional<std::string>>(index);
        if (path.has_value() && !path->empty())
            paths << QString::fromStdString(*path);
    }
    return paths;
}

void Sdk::SetRecentProjects(const QStringList &paths) {
    Registry* registry = gApp->GetCore()->GetRegistry();

    int max = registry->GetKeyValue<int>("sdk.max_recent_projects").value_or(10);
    if (max < 0)
        max = 0;

    sol::table tbl = gApp->GetCore()->GetLuaState()->create_table();
    for (const QString &path : paths) {
        if (tbl.size() >= static_cast<std::size_t>(max))
            break;
        tbl.add(path.toStdString());
    }

    registry->SetKeyValue<sol::table>("sdk.recent_projects", tbl);
    registry->Save();
}

void Sdk::AddRecentProject(const std::filesystem::path &path) {
    if (path.empty())
        return;

    QString entry = QString::fromStdString(path.string());
    QStringList paths = GetRecentProjects();
#if defined(_WIN32)
    const Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#endif
    paths.removeIf([&](const QString &other) { return other.compare(entry, sensitivity) == 0; });
    paths.prepend(entry);

    SetRecentProjects(paths);
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
        mOpenPreviewAction = new QAction(QIcon(":/images/silk/chart_organisation.png"), "Open Item in Explorer...");
        mOpenPreviewAction->setObjectName("RequireProjectButton");
        mSavePreviewAction = new QAction(QIcon(":/images/silk/disk.png"), "Save Preview to Database");
        mClosePreviewAction = new QAction(QIcon(":/images/silk/cross.png"), "Close Preview File");
        mFileMenu->addAction(mOpenPreviewAction);
        mFileMenu->addAction(mSavePreviewAction);
        mFileMenu->addAction(mClosePreviewAction);
    mFileMenu->addSeparator();
        mFileMenu->addAction(mExitAction);

    // mEditMenu = menuBar()->addMenu(tr("&Edit"));

    mItemBrowserViewAction = new QAction(QIcon(":/images/silk/application_view_icons.png"), "Content Browser");
    mFileManagerViewAction = new QAction(QIcon(":/images/silk/folder_page.png"), "File Manager");
    mPreviewDockViewAction = new QAction(QIcon(":/images/silk/chart_organisation.png"), "Explorer && Properties");

    mViewMenu = menuBar()->addMenu(tr("&View"));
    mViewMenu->addAction(mItemBrowserViewAction);
    mViewMenu->addAction(mFileManagerViewAction);
    // The Explorer/Properties docks add their own toggle actions here (InitWidgets).

    // mProjectMenu = menuBar()->addMenu(tr("&Project"));

    mInsertMenu = menuBar()->addMenu(tr("&Insert"));

    mToolsMenu = menuBar()->addMenu(tr("&Tools"));

    mExecuteSqlAction = new QAction(QIcon(":/images/silk/database_gear.png"), "Execute SQL");
    mExecuteSqlAction->setObjectName("RequireProjectButton");
    mToolsMenu->addAction(mExecuteSqlAction);

    // Works without a project: it previews files straight from disk, no database involved.
    mPreviewFileAction = new QAction(QIcon(":/images/silk/zoom.png"), "Preview Roblox File...");
    mToolsMenu->addAction(mPreviewFileAction);

    // mPluginsMenu = menuBar()->addMenu(tr("&Plugins"));

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
        if (TryToRemoveFocusedProject()) {
            NOOBWARRIOR_FREE_PTR(focused)
        }
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
                int code = dialog.exec();

                if (code == QDialog::Accepted)
                    goto launchdialog;
            }
        } else {
launchdialog:
            BackupDialog backupDialog(this);
            backupDialog.exec();
        }
    });

    connect(mPreviewFileAction, &QAction::triggered, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Preview Roblox File", QString(),
            "Roblox files (*.rbxl *.rbxlx *.rbxm *.rbxmx);;All files (*.*)");
        if (!path.isEmpty())
            RobloxFilePreviewWindow::OpenFromFile(this, path);
    });

    connect(mExecuteSqlAction, &QAction::triggered, [this]() {
        Project* proj = GetFocusedProject();
        if (proj == nullptr) {
            QMessageBox::critical(this, "Cannot Open Dialog", "A project is not open!");
            return;
        }
        auto emuDbProj = dynamic_cast<EmuDbProject*>(proj);
        if (emuDbProj == nullptr) {
            QMessageBox::critical(this, "Cannot Open Dialog", "This project is not a database!");
            return;
        }
        ExecuteSqlDialog dialog(emuDbProj->GetDb(), this);
        dialog.exec();
        if (OverviewWidget *overview = emuDbProj->GetOverviewWidget(); overview != nullptr)
            overview->RefreshAll();
        Refresh();
    });
}

void Sdk::InitWidgets() {
    mTabWidget = new QTabWidget(this);
    setCentralWidget(mTabWidget);

    connect(mTabWidget, &QTabWidget::currentChanged, [this](int index) {
        Refresh();
    });

    mWelcomeWidget = new WelcomeWidget(this);
    mTabWidget->addTab(mWelcomeWidget, QIcon(":/images/icon16_aa.png"), "Welcome");

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
    mViewToolBar->addAction(mPreviewDockViewAction);

    mInsertToolBar = new QToolBar("Insert", this);
    for (int i = 0; i < ItemTypeCount; i++) {
        ItemType itemType = static_cast<ItemType>(i);
        QString itemTypeName = QString::fromStdString(GetTableNameFromItemType(itemType));
        auto itemTypeInsertAction = new QAction(QIcon(GetIconFileFromItemType(itemType)), itemTypeName, mInsertMenu);
        itemTypeInsertAction->setObjectName("RequireProjectButton");
        mInsertMenu->addAction(itemTypeInsertAction);
        connect(itemTypeInsertAction, &QAction::triggered, [this, itemType]() {
            auto *dbProj = dynamic_cast<EmuDbProject*>(mFocusedProject);
            if (dbProj != nullptr) {
                ItemDialog dialog(dbProj->GetDb(), itemType, std::nullopt, this);
                dialog.exec();
            } else QMessageBox::critical(this, "Cannot Insert Item", "The current project is not a valid database.", QMessageBox::Ok);
        });
        mInsertToolBar->addAction(itemTypeInsertAction);
    }

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

    // The docked Studio panels; files are driven by the File-menu preview actions.
    mStudioPreview = new RobloxFilePreviewController(this);
    mViewMenu->addAction(mStudioPreview->GetExplorerDock()->toggleViewAction());
    mViewMenu->addAction(mStudioPreview->GetPropertiesDock()->toggleViewAction());
    auto updatePreviewActions = [this]() {
        mSavePreviewAction->setEnabled(mStudioPreview->CanSaveAny());
        mClosePreviewAction->setEnabled(mStudioPreview->HasFiles());
    };
    connect(mStudioPreview, &RobloxFilePreviewController::FileStateChanged, this, updatePreviewActions);
    updatePreviewActions();
    connect(mSavePreviewAction, &QAction::triggered, this,
            [this]() { mStudioPreview->SaveActiveFile(); });
    connect(mClosePreviewAction, &QAction::triggered, this,
            [this]() { mStudioPreview->CloseActiveFile(); });
    connect(mOpenPreviewAction, &QAction::triggered, this,
            [this]() { mStudioPreview->OpenFromDatabasePrompt(); });

    // The View toolbar/menu entries toggle each dock widget's visibility (and raise it when shown).
    connect(mItemBrowserViewAction, &QAction::triggered, this, [this]() {
        bool show = !mItemBrowser->isVisible();
        mItemBrowser->setVisible(show);
        if (show) mItemBrowser->raise();
    });
    connect(mFileManagerViewAction, &QAction::triggered, this, [this]() {
        bool show = !mFileManager->isVisible();
        mFileManager->setVisible(show);
        if (show) mFileManager->raise();
    });
    connect(mPreviewDockViewAction, &QAction::triggered, this, [this]() {
        if (mStudioPreview != nullptr)
            mStudioPreview->EnsureVisible();
    });
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