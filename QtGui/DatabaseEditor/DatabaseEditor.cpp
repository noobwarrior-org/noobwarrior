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

#include <format>
#include <qnamespace.h>

#define CREATE_NEW_DATABASES_IN_MEMORY 1

using namespace NoobWarrior;

DatabaseEditor::DatabaseEditor(QWidget *parent) : QMainWindow(parent),
    mCurrentDatabase(nullptr),
    mOverviewWidget(nullptr),
    mContentBrowser(nullptr)
{
    setWindowTitle("Database Editor - noobWarrior");
    setWindowState(Qt::WindowMaximized);
    InitMenus();
    InitWidgets();
}

DatabaseEditor::~DatabaseEditor() {
    if (mCurrentDatabase) {
        mCurrentDatabase->Close();
        NOOBWARRIOR_FREE_PTR(mCurrentDatabase)
    }
}

void DatabaseEditor::closeEvent(QCloseEvent *event) {
    if (TryToCloseCurrentDatabase()) event->accept(); else event->ignore();
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
            mCurrentDatabase->WriteChangesToDisk();
close:
        mOverviewWidget->deleteLater();
        mCurrentDatabase->Close();
        NOOBWARRIOR_FREE_PTR(mCurrentDatabase)
        mContentBrowser->Refresh();

        for (auto button : findChildren<QAction*>("CreateContentButton"))
            button->setDisabled(true);

        setWindowTitle("Database Editor - noobWarrior");
    }
    return 1;
}

// WARNING: THIS WILL OVERWRITE THE FILE
void DatabaseEditor::TryToCreateFile(const QString &path) {
    // create an empty file of 0 bytes
    FILE *file = fopen(path.toLocal8Bit().constData(), "w");
    if (file == nullptr) {
        QMessageBox::critical(this, "Error", "Cannot create a new file in this directory. It is likely that you do not have sufficient privileges to write to this directory.");
        goto cleanup;
    }
    fprintf(file, ""); // bye bye
    TryToOpenFile(path);
cleanup:
    fclose(file);
}

void DatabaseEditor::TryToOpenFile(const QString &path) {
    if (!TryToCloseCurrentDatabase()) return;
    mCurrentDatabase = new Database(false);
    DatabaseResponse res = mCurrentDatabase->Open(path.toStdString());
    if (res != DatabaseResponse::Success) {
        QMessageBox::critical(this, "Error", QString("Cannot open database \"%1\"\n\nLast Error Received: \"%2\"\nError Code: %3").arg(path, QString::fromStdString(mCurrentDatabase->GetSqliteErrorMsg()), QString::fromStdString(std::format("{:#010x}", (int)res))));
        mCurrentDatabase->Close();
        NOOBWARRIOR_FREE_PTR(mCurrentDatabase)
    }
    setWindowTitle(QString("%1 - Database Editor - noobWarrior").arg(path.compare(":memory:") == 0 ? "Unsaved Database" : path));

    mOverviewWidget = new OverviewWidget(mCurrentDatabase);
    mTabWidget->setCurrentIndex(mTabWidget->addTab(mOverviewWidget, "Overview"));

    mContentBrowser->Refresh();

    for (auto button : findChildren<QAction*>("CreateContentButton"))
        button->setDisabled(false);
}

Database *DatabaseEditor::GetCurrentlyEditingDatabase() {
    return mCurrentDatabase;
}

void DatabaseEditor::InitMenus() {
    QAction *newAct = new QAction("New Database");
    QAction *openAct = new QAction("Open Database");
    QAction *saveAct = new QAction("Save Database");
    QAction *saveAsAct = new QAction("Save Database As");

    mFileMenu = menuBar()->addMenu(tr("&File"));

    newAct->setParent(mFileMenu);
    openAct->setParent(mFileMenu);
    saveAct->setParent(mFileMenu);
    saveAsAct->setParent(mFileMenu);

    mFileMenu->addAction(newAct);
    mFileMenu->addAction(openAct);
    mFileMenu->addAction(saveAct);
    mFileMenu->addAction(saveAsAct);

    connect(newAct, &QAction::triggered, [&]() {
#if CREATE_NEW_DATABASES_IN_MEMORY
        TryToOpenFile();
#else
        QString filePath = QFileDialog::getSaveFileName(
            this,
            "New Database",
            QString::fromStdString(gApp->GetCore()->GetConfig()->GetUserDataDir() / "databases"),
            "noobWarrior Database (*.nwdb)"
        );
        if (!filePath.isEmpty()) TryToCreateFile(filePath);
#endif
    });

    connect(openAct, &QAction::triggered, [&]() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Open Database",
            QString::fromStdString((gApp->GetCore()->GetUserDataDir() / "databases").string()),
            "noobWarrior Database (*.nwdb)"
        );
        if (!filePath.isEmpty()) TryToOpenFile(filePath);
    });
}

void DatabaseEditor::InitWidgets() {
    mTabWidget = new QTabWidget(this);
    setCentralWidget(mTabWidget);

    auto *hi = new QLabel("Welcome!\nUse the \"Content Browser\" to look at all the available contents of this database\nUse the \"Organizer\" to organize this content in a way that you similarly would in your file manager");
    hi->setFont(QFont("Source Sans Pro", 20));
    hi->setAlignment(Qt::AlignCenter);

    mTabWidget->addTab(hi, "Welcome");

    mFileToolBar = new QToolBar(this);
    mFileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mFileToolBar->setWindowIconText("File");

    auto fileNewButton = new QAction(QIcon(":/images/silk/database_add.png"), "New\nDatabase", mFileToolBar);
    mFileToolBar->addAction(fileNewButton);
    connect(fileNewButton, &QAction::triggered, [&]() {
        TryToOpenFile();
    });

    auto fileOpenButton = new QAction(QIcon(":/images/silk/database_edit.png"), "Open\nDatabase", mFileToolBar);
    mFileToolBar->addAction(fileOpenButton);
    connect(fileOpenButton, &QAction::triggered, [&]() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Open Database",
            QString::fromStdString((gApp->GetCore()->GetUserDataDir() / "databases").string()),
            "noobWarrior Database (*.nwdb)"
        );
        if (!filePath.isEmpty()) TryToOpenFile(filePath);
    });

    auto fileSave = new QAction(QIcon(":/images/silk/database_save.png"), "Save\nDatabase", mFileToolBar);
    mFileToolBar->addAction(fileSave);

    auto fileSaveAs = new QAction(QIcon(":/images/silk/database_save.png"), "Save\nDatabase As...", mFileToolBar);
    mFileToolBar->addAction(fileSaveAs);

    auto fileClose = new QAction(QIcon(":/images/silk/database_delete.png"), "Close Current\nDatabase", mFileToolBar);
    mFileToolBar->addAction(fileClose);
    connect(fileClose, &QAction::triggered, [&]() {
        TryToCloseCurrentDatabase();
    });

    mViewToolBar = new QToolBar(this);
    mViewToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mViewToolBar->setWindowIconText("View");

    auto viewOutlineButton = new QAction(QIcon(":/images/silk/application_view_icons.png"), "Content\nBrowser", mViewToolBar);
    mViewToolBar->addAction(viewOutlineButton);

    auto viewOrganizerButton = new QAction(QIcon(":/images/silk/folder_page.png"), "Organizer\n", mViewToolBar);
    mViewToolBar->addAction(viewOrganizerButton);

    mInsertToolBar = new QToolBar(this);
    mInsertToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mInsertToolBar->setWindowIconText("Insert");

    for (int i = 0; i < 7; i++) {
        auto idType = static_cast<IdType>(i);
        QString idTypeStr = IdTypeAsString(idType);

        auto insertAssetButton = new QAction(QIcon(GetIconForIdType(idType)), QString("Create\n%1").arg(idTypeStr), mInsertToolBar);
        insertAssetButton->setDisabled(true); // Disable until we have a loaded database.
        insertAssetButton->setObjectName("CreateContentButton");
        mInsertToolBar->addAction(insertAssetButton);

        connect(insertAssetButton, &QAction::triggered, [&, idType]() {
            ContentEditorDialog dialog(idType, this);
            dialog.exec();
        });
    }

    addToolBar(Qt::ToolBarArea::TopToolBarArea, mFileToolBar);
    // addToolBarBreak();
    addToolBar(Qt::ToolBarArea::TopToolBarArea, mViewToolBar);
    addToolBar(Qt::ToolBarArea::TopToolBarArea, mInsertToolBar);

    mContentBrowser = new ContentBrowserWidget(this);
    mContentBrowser->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::LeftDockWidgetArea, mContentBrowser);
}
