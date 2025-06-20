// === noobWarrior ===
// File: DatabaseEditor.cpp
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Qt window that lets users view and edit a noobWarrior database
#include "DatabaseEditor.h"
#include "OutlineWidget.h"

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

using namespace NoobWarrior;

DatabaseEditor::DatabaseEditor(QWidget *parent) : QMainWindow(parent), mCurrentDatabase(nullptr) {
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
    if (mCurrentDatabase != nullptr && mCurrentDatabase->IsDirty()) {
        QMessageBox::StandardButton res = QMessageBox::question( this, nullptr,
            QString("Do you want to save changes to \"%1\"?").arg(QString::fromStdString(mCurrentDatabase->GetTitle())),
            QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
            QMessageBox::Yes);
        if (res == QMessageBox::Cancel) {
            event->ignore();
        } else {
            if (res == QMessageBox::Yes)
                mCurrentDatabase->WriteChangesToDisk();
            event->accept();
        }
    }
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
    mCurrentDatabase = new Database();
    int val = mCurrentDatabase->Open(path.toStdString());
    if (val != 0) {
        QMessageBox::critical(this, "Error", QString("Cannot open database of file \"%1\"\n\nLast Error Received: \"%2\"\nError Code: %3").arg(path, QString::fromStdString(mCurrentDatabase->GetSqliteErrorMsg()), QString::fromStdString(std::format("{:#010x}", val))));
        mCurrentDatabase->Close();
        NOOBWARRIOR_FREE_PTR(mCurrentDatabase)
    }
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
        // QString filePath = QFileDialog::getSaveFileName(this, "New Database", "./databases/", "noobWarrior Database (*.nwdb)");
        // if (!filePath.isEmpty()) TryToCreateFile(filePath);
        TryToOpenFile();
    });

    connect(openAct, &QAction::triggered, [&]() {
        QString filePath = QFileDialog::getOpenFileName(this, "Open Database", "./databases/", "noobWarrior Database (*.nwdb)");
        if (!filePath.isEmpty()) TryToOpenFile(filePath);
    });
}

void DatabaseEditor::InitWidgets() {
    mTabWidget = new QTabWidget(this);
    setCentralWidget(mTabWidget);

    mFileToolBar = new QToolBar(this);
    mFileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mFileToolBar->setWindowIconText("File");
    auto fileNewButton = new QAction(QIcon(":/images/explorer.png"), "New Database", mFileToolBar);
    fileNewButton->setProperty("textWrapEnabled", true);
    mFileToolBar->addAction(fileNewButton);

    mViewToolBar = new QToolBar(this);
    mViewToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mViewToolBar->setWindowIconText("View");
    auto viewOutlineButton = new QAction(QIcon(":/images/explorer.png"), "Outline", mViewToolBar);
    viewOutlineButton->setProperty("textWrapEnabled", true);
    mViewToolBar->addAction(viewOutlineButton);

    addToolBar(Qt::ToolBarArea::TopToolBarArea, mFileToolBar);
    // addToolBarBreak();
    addToolBar(Qt::ToolBarArea::TopToolBarArea, mViewToolBar);

    auto outlineWidget = new OutlineWidget();
    outlineWidget->setParent(this);
    outlineWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::LeftDockWidgetArea, outlineWidget);
}
