// === noobWarrior ===
// File: DatabaseEditor.cpp
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Qt window that lets users view and edit a noobWarrior database
#include "DatabaseEditor.h"
#include "ContentEditorDialog.h"
#include "ContentBrowserWidget.h"
#include "../Application.h"

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

#define ADD_ID_TYPE(IdType, iconPath) QString IdType##_Str = IdType::TableName; \
    \
    auto IdType##_InsertAction = new QAction(QIcon(iconPath), QString("Create\n%1").arg(IdType##_Str), mInsertToolBar); \
    IdType##_InsertAction->setObjectName("RequiresDatabaseButton"); \
    mInsertToolBar->addAction(IdType##_InsertAction); \
    \
    connect(IdType##_InsertAction, &QAction::triggered, [&, this]() { \
        ContentEditorDialog<IdType> dialog(this); \
        dialog.exec(); \
    });

using namespace NoobWarrior;

DatabaseEditor::DatabaseEditor(QWidget *parent) : QMainWindow(parent),
    mCurrentDatabase(nullptr),
    mOverviewWidget(nullptr),
    mContentBrowser(nullptr),
    mBackgroundTasksStatusBarWidget(nullptr)
{
    setWindowTitle("Database Editor - noobWarrior");
    setAcceptDrops(true);
    // setWindowState(Qt::WindowMaximized);
    InitMenus();
    InitStatusBarWidgets();
    InitWidgets();

    BackgroundTask *theItem = mBackgroundTasks.AddTask({
        .Title = "Pooping",
        .Caption = "Some pooping is happening right now",
    });
    mBackgroundTasks.UpdateTask(theItem, 0.5, "/Users/Poop/");
}

DatabaseEditor::~DatabaseEditor() {
    if (mCurrentDatabase) {
        mCurrentDatabase->Close();
        NOOBWARRIOR_FREE_PTR(mCurrentDatabase)
    }
}

void DatabaseEditor::Refresh() {
    mContentBrowser->Refresh();
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
            QString("%1%2 - Database Editor - noobWarrior")
            .arg(QString::fromStdString(mCurrentDatabase->GetTitle()))
            .arg(mCurrentDatabase->IsDirty() ? "*" : "")
        );
    } else setWindowTitle(QString("Database Editor - noobWarrior"));
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
        mContentBrowser->Refresh();

        for (auto button : findChildren<QAction*>("RequiresDatabaseButton"))
            button->setDisabled(true);
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
        QString name = qgetenv("USER");
        if (name.isEmpty())
            name = qgetenv("USERNAME");
        mCurrentDatabase->SetAuthor(name.toStdString());

        // It got marked as dirty because we programmatically changed the author of the DB.
        // Unmark it so that you won't get the stupid "you forgot to save your changes" screen when closing this empty new file.
        mCurrentDatabase->UnmarkDirty();
    }

    // our own functions
    mOverviewWidget = new OverviewWidget(mCurrentDatabase);
    mTabWidget->setCurrentIndex(mTabWidget->addTab(mOverviewWidget, mOverviewWidget->windowTitle()));

    mContentBrowser->Refresh();

    for (auto button : findChildren<QAction*>("RequiresDatabaseButton"))
        button->setDisabled(false);
}

Database *DatabaseEditor::GetCurrentlyEditingDatabase() {
    return mCurrentDatabase;
}

ContentBrowserWidget *DatabaseEditor::GetContentBrowser() {
    return mContentBrowser;
}

void DatabaseEditor::InitMenus() {
    mNewDatabaseAction = new QAction("New Database");
    mOpenDatabaseAction = new QAction("Open Database");
    mCloseDatabaseAction = new QAction("Close Current Database");
    mSaveDatabaseAction = new QAction("Save Database");
    mSaveAsDatabaseAction = new QAction("Save Database As");

    mNewDatabaseAction->setShortcut(QKeySequence(tr("Ctrl+N")));
    mOpenDatabaseAction->setShortcut(QKeySequence(tr("Ctrl+O")));
    mCloseDatabaseAction->setShortcut(QKeySequence(tr("Ctrl+W")));
    mSaveDatabaseAction->setShortcut(QKeySequence(tr("Ctrl+S")));

    mCloseDatabaseAction->setObjectName("RequiresDatabaseButton");
    mSaveDatabaseAction->setObjectName("RequiresDatabaseButton");
    mSaveAsDatabaseAction->setObjectName("RequiresDatabaseButton");

    mFileMenu = menuBar()->addMenu(tr("&File"));
    mFileMenu->addAction(mNewDatabaseAction);
    mFileMenu->addAction(mOpenDatabaseAction);
    mFileMenu->addAction(mCloseDatabaseAction);
    mFileMenu->addAction(mSaveDatabaseAction);
    mFileMenu->addAction(mSaveAsDatabaseAction);

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
            mCurrentDatabase->WriteChangesToDisk();
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
}

void DatabaseEditor::InitStatusBarWidgets() {
    mBackgroundTasksStatusBarWidget = new BackgroundTasksStatusBarWidget();
    statusBar()->addPermanentWidget(mBackgroundTasksStatusBarWidget);
    mBackgroundTasks.SetStatusBarWidget(mBackgroundTasksStatusBarWidget);
}

void DatabaseEditor::InitWidgets() {
    mTabWidget = new QTabWidget(this);
    setCentralWidget(mTabWidget);

    auto *hi = new QLabel("New Database  Ctrl-N\nOpen Database  Ctrl-O");
    hi->setFont(QFont(QApplication::font().family(), 14));
    hi->setAlignment(Qt::AlignCenter);
    hi->setWordWrap(true);

    mTabWidget->addTab(hi, "Welcome");

    mFileToolBar = new QToolBar(this);
    mFileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mFileToolBar->setWindowIconText("File");

    auto fileNewButton = new QAction(QIcon(":/images/silk/database_add.png"), "New\nDatabase", mFileToolBar);
    mFileToolBar->addAction(fileNewButton);
    connect(fileNewButton, &QAction::triggered, [&]() {
        mNewDatabaseAction->trigger();
    });

    auto fileOpenButton = new QAction(QIcon(":/images/silk/database_edit.png"), "Open\nDatabase", mFileToolBar);
    mFileToolBar->addAction(fileOpenButton);
    connect(fileOpenButton, &QAction::triggered, [&]() {
        mOpenDatabaseAction->trigger();
    });

    auto fileSave = new QAction(QIcon(":/images/silk/database_save.png"), "Save\nDatabase", mFileToolBar);
    fileSave->setObjectName("RequiresDatabaseButton");
    mFileToolBar->addAction(fileSave);

    auto fileSaveAs = new QAction(QIcon(":/images/silk/database_save.png"), "Save\nDatabase As...", mFileToolBar);
    fileSaveAs->setObjectName("RequiresDatabaseButton");
    mFileToolBar->addAction(fileSaveAs);

    auto fileClose = new QAction(QIcon(":/images/silk/database_delete.png"), "Close Current\nDatabase", mFileToolBar);
    fileClose->setObjectName("RequiresDatabaseButton");
    mFileToolBar->addAction(fileClose);
    connect(fileClose, &QAction::triggered, [&]() {
        mCloseDatabaseAction->trigger();
    });

    mViewToolBar = new QToolBar(this);
    mViewToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mViewToolBar->setWindowIconText("View");

    auto viewOutlineButton = new QAction(QIcon(":/images/silk/application_view_icons.png"), "Content\nBrowser", mViewToolBar);
    viewOutlineButton->setObjectName("RequiresDatabaseButton");
    mViewToolBar->addAction(viewOutlineButton);

    auto viewOrganizerButton = new QAction(QIcon(":/images/silk/folder_page.png"), "Organizer\n", mViewToolBar);
    viewOrganizerButton->setObjectName("RequiresDatabaseButton");
    mViewToolBar->addAction(viewOrganizerButton);

    mInsertToolBar = new QToolBar(this);
    mInsertToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mInsertToolBar->setWindowIconText("Insert");

    ADD_ID_TYPE(Asset, ":/images/silk/page_add.png")
    ADD_ID_TYPE(Badge, ":/images/silk/medal_gold_add.png")
    ADD_ID_TYPE(User, ":/images/silk/user_add.png")

    for (auto button : findChildren<QAction*>("RequiresDatabaseButton"))
        button->setDisabled(true); // Disable all buttons that require a database since one isn't loaded right now

    for (auto button : menuBar()->findChildren<QAction*>("RequiresDatabaseButton"))
        button->setDisabled(true); // Disable all buttons that require a database since one isn't loaded right now

    addToolBar(Qt::ToolBarArea::TopToolBarArea, mFileToolBar);
    // addToolBarBreak();
    addToolBar(Qt::ToolBarArea::TopToolBarArea, mViewToolBar);
    addToolBar(Qt::ToolBarArea::TopToolBarArea, mInsertToolBar);

    mContentBrowser = new ContentBrowserWidget(this);
    mContentBrowser->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::LeftDockWidgetArea, mContentBrowser);

    mOrganizer = new OrganizerWidget(this);
    mOrganizer->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::LeftDockWidgetArea, mOrganizer);
}
