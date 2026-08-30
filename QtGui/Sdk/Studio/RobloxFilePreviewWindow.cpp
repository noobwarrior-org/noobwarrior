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
// File: RobloxFilePreviewWindow.cpp
// Started by: Hattozo
// Started on: 8/29/2026
// Description: Standalone window that allows you to preview Roblox files by grouping Explorer and Properties widgets together
#include "RobloxFilePreviewWindow.h"
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include "Application.h"

#include <QFileInfo>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <BusyDialog.h>

using namespace NoobWarrior;

static QString PropertiesCaptionFor(const std::vector<NoobWarrior::Roblox::Instance*>& instances) {
    if (instances.empty())
        return QStringLiteral("Properties");
    if (instances.size() > 1)
        return QString("Properties - %1 items").arg(instances.size());
    return QString("Properties - %1 \"%2\"")
        .arg(QString::fromStdString(instances.front()->ClassName),
             QString::fromStdString(instances.front()->Name));
}

RobloxFilePreviewWindow::RobloxFilePreviewWindow(std::unique_ptr<Roblox::RobloxFile> file,
                                                 const QString& filePath, QWidget* parent)
    : QMainWindow(parent, Qt::Window),
      mFile(std::move(file)),
      mFilePath(filePath)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(QString("%1[*] - Preview").arg(QFileInfo(filePath).fileName()));
    setWindowIcon(QIcon(":/images/silk/zoom.png"));
    resize(400, 760);
    setDockNestingEnabled(true);

    mExplorer = new ExplorerWidget(this);
    mExplorerDock = new QDockWidget("Explorer", this);
    mExplorerDock->setWidget(mExplorer);
    mExplorerDock->setObjectName("PreviewExplorerDock");
    addDockWidget(Qt::RightDockWidgetArea, mExplorerDock);

    mProperties = new PropertiesWidget(this);
    // A file on disk has no database of its own; sounds search every mounted database.
    mProperties->SetAssetDataResolver([](int64_t assetId, std::vector<unsigned char>* out) -> bool {
        EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
        for (EmuDb* db : manager->GetMountedDatabases())
            if (db != nullptr &&
                db->RetrieveDecodedAssetData(assetId, 0, out) == SqlDb::Response::Success && !out->empty())
                return true;
        return false;
    });
    mPropertiesDock = new QDockWidget("Properties", this);
    mPropertiesDock->setWidget(mProperties);
    mPropertiesDock->setObjectName("PreviewPropertiesDock");
    addDockWidget(Qt::RightDockWidgetArea, mPropertiesDock);
    splitDockWidget(mExplorerDock, mPropertiesDock, Qt::Vertical);

    QMenu* fileMenu = menuBar()->addMenu("&File");
    QAction* save = fileMenu->addAction(QIcon(":/images/silk/disk.png"), "Save");
    save->setShortcut(QKeySequence::Save);
    QAction* saveAs = fileMenu->addAction("Save As...");
    connect(save, &QAction::triggered, [this]() { Save(mFilePath); });
    connect(saveAs, &QAction::triggered, [this]() {
        QString path = QFileDialog::getSaveFileName(this, "Save Roblox File", mFilePath,
            "Roblox files (*.rbxl *.rbxlx *.rbxm *.rbxmx);;All files (*.*)");
        if (!path.isEmpty())
            Save(path);
    });

    connect(mExplorer, &ExplorerWidget::SelectionChanged,
            [this](const std::vector<Roblox::Instance*>& instances) {
        mProperties->SetInstances(instances);
        mPropertiesDock->setWindowTitle(PropertiesCaptionFor(instances));
    });
    connect(mProperties, &PropertiesWidget::InstanceEdited,
            [this](Roblox::Instance* instance) {
        mExplorer->RefreshInstanceLabel(instance);
        setWindowModified(true);
        if (instance != nullptr && mPropertiesDock && mProperties->SelectionSize() == 1)
            mPropertiesDock->setWindowTitle(PropertiesCaptionFor({instance}));
    });
    connect(mExplorer, &ExplorerWidget::InstanceDoubleClicked,
            [this](Roblox::Instance* instance) { mExplorer->OpenSourceEditorFor(instance); });
    connect(mExplorer, &ExplorerWidget::TreeEdited, [this](Roblox::RobloxFile* file) {
        setWindowModified(true);
        mExplorer->SetFileDirty(file, true);
    });
    connect(mExplorer, &ExplorerWidget::SubtreeRemoved,
            mExplorer, &ExplorerWidget::CloseSourceEditorsUnder);
    // This window IS the file: root Save goes to its path, root Close closes the window.
    connect(mExplorer, &ExplorerWidget::FileSaveRequested,
            this, [this](Roblox::RobloxFile*) { Save(mFilePath); });
    connect(mExplorer, &ExplorerWidget::FileCloseRequested,
            this, [this](Roblox::RobloxFile*) { close(); });

    mExplorer->AddFile(mFile.get(), QFileInfo(filePath).fileName());
}

void RobloxFilePreviewWindow::closeEvent(QCloseEvent* event) {
    if (mExplorer != nullptr && mExplorer->HasUnappliedSourceEdits()) {
        if (QMessageBox::question(this, "Close Preview",
                "One or more script editors have changes that were never applied; closing the "
                "preview discards them. Close anyway?",
                QMessageBox::Close | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Close) {
            event->ignore();
            return;
        }
    }
    if (isWindowModified()) {
        const auto choice = QMessageBox::question(this, "Close Preview",
            QString("Save changes to \"%1\" before closing?").arg(QFileInfo(mFilePath).fileName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        if (choice == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
        if (choice == QMessageBox::Save) {
            Save(mFilePath);
            if (isWindowModified()) { // still modified = the save failed and warned
                event->ignore();
                return;
            }
        }
    }
    QMainWindow::closeEvent(event);
}

void RobloxFilePreviewWindow::Save(const QString& toPath) {
    // Save() is virtual per format, so an .rbxlx stays XML and an .rbxl stays binary.
    if (mFile->Save(std::string_view(toPath.toStdString())) != Roblox::FileResponse::Success) {
        QMessageBox::warning(this, "Save",
            QString("Could not save to \"%1\".\n%2").arg(toPath,
                QString::fromStdString(mFile->GetLastError())));
        return;
    }
    mFilePath = toPath;
    setWindowTitle(QString("%1[*] - Preview").arg(QFileInfo(toPath).fileName()));
    setWindowModified(false);
    mExplorer->SetFileDirty(mFile.get(), false);
}

void RobloxFilePreviewWindow::OpenFromFile(QWidget* parent, const QString& filePath) {
    BusyDialog busy("Opening preview...", parent != nullptr ? parent->window() : nullptr);
    busy.show();
    QCoreApplication::processEvents(); // deliver the show/layout events
    busy.repaint(); // force the first paint before parsing blocks the event loop

    std::unique_ptr<Roblox::RobloxFile> file;
    if (Roblox::RobloxFile::Open(file, filePath.toStdString()) != Roblox::FileResponse::Success ||
        file == nullptr) {
        busy.close();
        QMessageBox::warning(parent, "Preview",
            QString("\"%1\" could not be parsed as a Roblox place or model.").arg(filePath));
        return;
    }
    auto* window = new RobloxFilePreviewWindow(std::move(file), filePath,
                                               parent != nullptr ? parent->window() : nullptr);
    busy.close();
    window->show();
}