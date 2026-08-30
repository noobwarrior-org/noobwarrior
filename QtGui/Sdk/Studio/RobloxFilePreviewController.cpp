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
// File: RobloxFilePreviewController.cpp
// Started by: Hattozo
// Started on: 8/29/2026
// Description: See RobloxFilePreviewController.h
#include "RobloxFilePreviewController.h"
#include "Application.h"
#include "Sdk/Item/ItemOpenSaveDialog.h"
#include "Sdk/Project/EmuDb/EmuDbProject.h"
#include "Sdk/Project/Project.h"
#include "Sdk/Sdk.h"

#include <BusyDialog.h>

#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTabWidget>

#include <algorithm>

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

RobloxFilePreviewController::RobloxFilePreviewController(QMainWindow* host) : QObject(host) {
    setObjectName("RobloxFilePreviewController");

    mExplorer = new ExplorerWidget(host);
    mExplorer->SetOpenMenuEnabled(true);
    mExplorerDock = new QDockWidget("Explorer", host);
    mExplorerDock->setWidget(mExplorer);
    mExplorerDock->setObjectName("SdkPreviewExplorerDock");
    host->addDockWidget(Qt::RightDockWidgetArea, mExplorerDock);

    mProperties = new PropertiesWidget(host);
    mPropertiesDock = new QDockWidget("Properties", host);
    mPropertiesDock->setWidget(mProperties);
    mPropertiesDock->setObjectName("SdkPreviewPropertiesDock");
    host->addDockWidget(Qt::RightDockWidgetArea, mPropertiesDock);
    host->splitDockWidget(mExplorerDock, mPropertiesDock, Qt::Vertical);

    // Try each open file's resolver.
    mProperties->SetAssetDataResolver([this](int64_t assetId, std::vector<unsigned char>* out) {
        for (auto& entry : mFiles)
            if (entry->Resolver && entry->Resolver(assetId, out) && !out->empty())
                return true;
        return false;
    });

    QPointer<QMainWindow> hostGuard(host);
    mExplorer->SetSourceEditorHost([hostGuard](QWidget* editor, const QString& title) -> bool {
        auto* sdk = qobject_cast<Sdk*>(hostGuard.data());
        if (sdk == nullptr)
            return false;
        Project* project = sdk->GetFocusedProject();
        if (project == nullptr || project->GetTabWidget() == nullptr)
            return false;
        QTabWidget* tabs = project->GetTabWidget();
        tabs->setCurrentIndex(tabs->addTab(editor, editor->windowIcon(), title));
        return true;
    });

    connect(mExplorer, &ExplorerWidget::SelectionChanged,
            [this](const std::vector<Roblox::Instance*>& instances) {
        mProperties->SetInstances(instances);
        mPropertiesDock->setWindowTitle(PropertiesCaptionFor(instances));
    });
    connect(mProperties, &PropertiesWidget::InstanceEdited,
            [this](Roblox::Instance* instance) {
        mExplorer->RefreshInstanceLabel(instance);
        SetDirty(mExplorer->FileForInstance(instance), true);
        if (instance != nullptr && mProperties->SelectionSize() == 1)
            mPropertiesDock->setWindowTitle(PropertiesCaptionFor({instance}));
    });
    connect(mExplorer, &ExplorerWidget::InstanceDoubleClicked,
            [this](Roblox::Instance* instance) { mExplorer->OpenSourceEditorFor(instance); });
    connect(mExplorer, &ExplorerWidget::TreeEdited,
            [this](Roblox::RobloxFile* file) { SetDirty(file, true); });
    connect(mExplorer, &ExplorerWidget::SubtreeRemoved,
            mExplorer, &ExplorerWidget::CloseSourceEditorsUnder);
    connect(mExplorer, &ExplorerWidget::FileSaveRequested,
            this, [this](Roblox::RobloxFile* file) { SaveFile(file); });
    connect(mExplorer, &ExplorerWidget::FileCloseRequested,
            this, [this](Roblox::RobloxFile* file) { CloseFile(file); });
    connect(mExplorer, &ExplorerWidget::OpenFromDatabaseRequested,
            this, &RobloxFilePreviewController::OpenFromDatabasePrompt);
    connect(mExplorer, &ExplorerWidget::OpenFromFileRequested,
            this, &RobloxFilePreviewController::OpenFromFilePrompt);
}

RobloxFilePreviewController::Entry* RobloxFilePreviewController::EntryFor(Roblox::RobloxFile* file) {
    for (auto& entry : mFiles)
        if (entry->File.get() == file)
            return entry.get();
    return nullptr;
}

RobloxFilePreviewController::Entry* RobloxFilePreviewController::ActiveEntry() {
    return EntryFor(mExplorer->ActiveFile());
}

void RobloxFilePreviewController::SetDirty(Roblox::RobloxFile* file, bool dirty) {
    Entry* entry = EntryFor(file);
    if (entry == nullptr)
        return;
    entry->Dirty = dirty;
    mExplorer->SetFileDirty(file, dirty);
    emit FileStateChanged();
}

bool RobloxFilePreviewController::CanSaveAny() const {
    return std::any_of(mFiles.begin(), mFiles.end(),
                       [](const auto& entry) { return static_cast<bool>(entry->Save); });
}

bool RobloxFilePreviewController::LoadFile(const QString& key, const QString& title,
                                           const std::vector<unsigned char>& data,
                                           std::function<bool(const std::vector<unsigned char>&)> save,
                                           AssetDataResolver resolver) {
    if (!key.isEmpty()) {
        for (auto& entry : mFiles) {
            if (entry->Key == key) {
                mExplorer->FocusFile(entry->File.get());
                EnsureVisible();
                return true;
            }
        }
    }

    std::unique_ptr<Roblox::RobloxFile> file;
    if (Roblox::RobloxFile::Open(file, data) != Roblox::FileResponse::Success || file == nullptr) {
        QMessageBox::warning(mExplorerDock, "Preview",
            QString("\"%1\" could not be parsed as a Roblox place or model.").arg(title));
        return false;
    }

    auto entry = std::make_unique<Entry>();
    entry->Key = key;
    entry->Title = title;
    entry->File = std::move(file);
    entry->Save = std::move(save);
    entry->Resolver = std::move(resolver);
    Roblox::RobloxFile* raw = entry->File.get();
    mFiles.push_back(std::move(entry));

    mExplorer->AddFile(raw, title);
    mExplorer->FocusFile(raw);
    EnsureVisible();
    emit FileStateChanged();
    return true;
}

bool RobloxFilePreviewController::SaveEntry(Entry& entry) {
    if (!entry.Save) {
        QMessageBox::warning(mPropertiesDock, "Save",
            QString("\"%1\" has no save destination.").arg(entry.Title));
        return false;
    }
    std::vector<unsigned char> bytes;
    if (entry.File->Save(bytes) != Roblox::FileResponse::Success) {
        QMessageBox::warning(mPropertiesDock, "Save",
            QString("Could not re-serialize \"%1\".\n%2")
                .arg(entry.Title, QString::fromStdString(entry.File->GetLastError())));
        return false;
    }
    if (!entry.Save(bytes)) {
        QMessageBox::warning(mPropertiesDock, "Save",
            QString("\"%1\" could not be saved.").arg(entry.Title));
        return false;
    }
    SetDirty(entry.File.get(), false);
    return true;
}

bool RobloxFilePreviewController::ConfirmCloseEntry(Entry& entry) {
    Roblox::RobloxFile* file = entry.File.get();
    if (mExplorer->HasUnappliedSourceEditsUnder(file)) {
        if (QMessageBox::question(mPropertiesDock, "Close File",
                QString("\"%1\" has script editors with changes that were never applied; closing "
                        "discards them. Close anyway?").arg(entry.Title),
                QMessageBox::Close | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Close)
            return false;
    }
    if (!entry.Dirty)
        return true;
    if (entry.Save) {
        const auto choice = QMessageBox::question(mPropertiesDock, "Close File",
            QString("Save \"%1\" before closing?").arg(entry.Title),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        if (choice == QMessageBox::Cancel)
            return false;
        if (choice == QMessageBox::Save && !SaveEntry(entry))
            return false;
        return true;
    }
    return QMessageBox::question(mPropertiesDock, "Close File",
        QString("\"%1\" has edits but no save destination; closing discards them. Close anyway?")
            .arg(entry.Title),
        QMessageBox::Close | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Close;
}

bool RobloxFilePreviewController::SaveFile(Roblox::RobloxFile* file) {
    Entry* entry = EntryFor(file);
    return entry != nullptr && SaveEntry(*entry);
}

bool RobloxFilePreviewController::CloseFile(Roblox::RobloxFile* file) {
    Entry* entry = EntryFor(file);
    if (entry == nullptr)
        return true;
    if (!ConfirmCloseEntry(*entry))
        return false;
    mExplorer->CloseSourceEditorsUnder(file);
    mExplorer->RemoveFile(file);
    std::erase_if(mFiles, [file](const auto& e) { return e->File.get() == file; });
    emit FileStateChanged();
    return true;
}

bool RobloxFilePreviewController::SaveActiveFile() {
    Entry* entry = ActiveEntry();
    if (entry == nullptr) {
        QMessageBox::information(mExplorerDock, "Save",
            "Click the file (or something inside it) in the Explorer first.");
        return false;
    }
    return SaveEntry(*entry);
}

bool RobloxFilePreviewController::CloseActiveFile() {
    Entry* entry = ActiveEntry();
    if (entry == nullptr) {
        QMessageBox::information(mExplorerDock, "Close File",
            "Click the file (or something inside it) in the Explorer first.");
        return false;
    }
    return CloseFile(entry->File.get());
}

bool RobloxFilePreviewController::ConfirmCloseAll() {
    for (auto& entry : mFiles)
        if (!ConfirmCloseEntry(*entry))
            return false;
    return true;
}

void RobloxFilePreviewController::OpenFromDatabasePrompt() {
    auto* sdk = qobject_cast<Sdk*>(parent());
    auto* project = sdk != nullptr ? dynamic_cast<EmuDbProject*>(sdk->GetFocusedProject()) : nullptr;
    if (project == nullptr) {
        QMessageBox::information(mExplorerDock, "Explorer",
            "Open a database project first, then pick a place or model to preview.");
        return;
    }
    std::optional<int64_t> id = ItemOpenSaveDialog::GetOpenAssetId(mExplorerDock, project->GetDb(),
                                                                   PreviewableAssetTypes());
    if (!id.has_value())
        return;
    QPointer<Sdk> guard(sdk);
    Project* chosenProject = project;
    ShowDockedPreviewForDatabaseAsset(qobject_cast<QMainWindow*>(parent()),
        [guard, chosenProject]() -> EmuDb* {
            if (!guard || !guard->IsProjectOpen(chosenProject))
                return nullptr;
            return static_cast<EmuDbProject*>(chosenProject)->GetDb();
        }, id.value());
}

void RobloxFilePreviewController::OpenFromFilePrompt() {
    QString path = QFileDialog::getOpenFileName(mExplorerDock, "Open Roblox File", QString(),
        "Roblox files (*.rbxl *.rbxlx *.rbxm *.rbxmx);;All files (*.*)");
    if (path.isEmpty())
        return;

    QFile in(path);
    if (!in.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(mExplorerDock, "Open Roblox File",
            QString("Could not read \"%1\".").arg(path));
        return;
    }
    const QByteArray raw = in.readAll();
    std::vector<unsigned char> data(raw.begin(), raw.end());

    // Disk-backed: save writes the bytes back to the same path.
    auto save = [path](const std::vector<unsigned char>& bytes) -> bool {
        QFile out(path);
        if (!out.open(QIODevice::WriteOnly))
            return false;
        return out.write(reinterpret_cast<const char*>(bytes.data()),
                         static_cast<qint64>(bytes.size())) == static_cast<qint64>(bytes.size());
    };
    auto resolver = [](int64_t assetId, std::vector<unsigned char>* out) -> bool {
        EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
        for (EmuDb* db : manager->GetMountedDatabases())
            if (db != nullptr &&
                db->RetrieveDecodedAssetData(assetId, 0, out) == SqlDb::Response::Success &&
                !out->empty())
                return true;
        return false;
    };

    BusyDialog busy("Opening preview...", mExplorerDock);
    busy.show();
    QCoreApplication::processEvents();
    busy.repaint();
    LoadFile("file:" + path, QFileInfo(path).fileName(), data, std::move(save), std::move(resolver));
}

void RobloxFilePreviewController::EnsureVisible() {
    mExplorerDock->show();
    mExplorerDock->raise();
    mPropertiesDock->show();
    mPropertiesDock->raise();
}

void NoobWarrior::ShowDockedPreviewForDatabaseAsset(QMainWindow* host,
                                                    std::function<EmuDb*()> dbGetter,
                                                    int64_t assetId) {
    if (host == nullptr || !dbGetter)
        return;
    EmuDb* db = dbGetter();
    if (db == nullptr)
        return;

    BusyDialog busy("Opening preview...", host);
    busy.show();
    QCoreApplication::processEvents();
    busy.repaint(); // force the first paint before parsing blocks the event loop

    std::vector<unsigned char> data;
    if (db->RetrieveDecodedAssetData(assetId, 0, &data) != SqlDb::Response::Success || data.empty()) {
        busy.close();
        QMessageBox::warning(host, "Preview", "This asset has no data to preview.");
        return;
    }

    QString name;
    Statement stmt = db->PrepareStatement("SELECT Name FROM Asset WHERE Id = ?;");
    stmt.Bind(1, assetId);
    if (stmt.Step() == SQLITE_ROW)
        name = QString::fromStdString(stmt.GetStringFromColumnIndex(0));
    if (name.isEmpty())
        name = QString::number(assetId);

    // dbGetter is re-resolved on every use so a closed project makes these inert, not dangling.
    auto save = [dbGetter, assetId](const std::vector<unsigned char>& bytes) -> bool {
        EmuDb* liveDb = dbGetter();
        if (liveDb == nullptr)
            return false;
        if (liveDb->AttachDataToAsset(assetId, 0, bytes) != SqlDb::Response::Success)
            return false;
        liveDb->MarkDirty();
        return true;
    };
    auto resolver = [dbGetter](int64_t soundAssetId, std::vector<unsigned char>* out) -> bool {
        EmuDb* liveDb = dbGetter();
        return liveDb != nullptr &&
               liveDb->RetrieveDecodedAssetData(soundAssetId, 0, out) == SqlDb::Response::Success &&
               !out->empty();
    };

    auto* controller = host->findChild<RobloxFilePreviewController*>(
        "RobloxFilePreviewController", Qt::FindDirectChildrenOnly);
    if (controller == nullptr)
        controller = new RobloxFilePreviewController(host);
    // Keyed on (database, asset) so re-previewing an open asset refocuses it.
    const QString key = QString("db:%1:%2").arg(reinterpret_cast<quintptr>(db)).arg(assetId);
    controller->LoadFile(key, name, data, std::move(save), std::move(resolver));
}

const std::vector<Roblox::AssetType>& NoobWarrior::PreviewableAssetTypes() {
    static const std::vector<Roblox::AssetType> kTypes = {
        Roblox::AssetType::Place, Roblox::AssetType::Model, Roblox::AssetType::Decal, Roblox::AssetType::Animation
    };
    return kTypes;
}

bool NoobWarrior::IsPreviewableAssetType(Roblox::AssetType type) {
    const auto& types = PreviewableAssetTypes();
    return std::find(types.begin(), types.end(), type) != types.end();
}
