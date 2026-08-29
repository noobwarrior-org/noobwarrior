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
// File: OverviewWidget.cpp
// Started by: Hattozo
// Started on: 7/4/2025
// Description: The page that you see that details all the statistics when you open a database in the editor.
#include "OverviewWidget.h"

#include "Sdk/ExecuteSqlDialog.h"
#include "Sdk/Sdk.h"
#include "Sdk/Item/Browser/ItemBrowserWidget.h"
#include "Application.h"

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

#include <QApplication>
#include <QCheckBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QToolButton>

#include <fstream>

using namespace NoobWarrior;

static constexpr qint64 kMaxIconBytes = 4 * 1024 * 1024;

static QString GetIconFileFromItemType(ItemType type) {
    switch (type) {
    case ItemType::Asset: return ":/images/silk/brick.png";
    case ItemType::Badge: return ":/images/silk/medal_gold_1.png";
    case ItemType::Bundle: return ":/images/silk/package.png";
    case ItemType::DevProduct: return ":/images/silk/key.png";
    case ItemType::Group: return ":/images/silk/group.png";
    case ItemType::Outfit: return ":/images/silk/user_female.png";
    case ItemType::Pass: return ":/images/silk/vcard.png";
    case ItemType::Set: return ":/images/silk/bricks.png";
    case ItemType::Universe: return ":/images/silk/world.png";
    case ItemType::User: return ":/images/silk/user.png";
    default: return ":/images/silk/page_white.png";
    }
}

static QString FormatBytes(int64_t bytes) {
    return QLocale::system().formattedDataSize(bytes, 2, QLocale::DataSizeTraditionalFormat);
}

static QString FormatCount(int64_t count) {
    return QLocale::system().toString(static_cast<qlonglong>(count));
}

static QLabel *MakeSectionTitle(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    label->setFont(QFont(QApplication::font().family(), 13, QFont::Bold));
    return label;
}

OverviewWidget::OverviewWidget(EmuDb *db, QWidget *parent) : QWidget(parent),
    mDatabase(db),
    ToplevelLayout(nullptr),
    ContentLayout(nullptr)
{
    setWindowTitle("Overview");
    InitWidgets();
    Refresh();
}

void OverviewWidget::InitWidgets() {
    ToplevelLayout = new QVBoxLayout(this);
    ToplevelLayout->setContentsMargins(32, 32, 32, 32);

    mOverviewLabel = new QLabel(QString::fromStdString(mDatabase->GetTitle()), this);
    mOverviewLabel->setFont(QFont(QApplication::font().family(), 24));
    ToplevelLayout->addWidget(mOverviewLabel);

    mPathLabel = new QLabel(this);
    mPathLabel->setForegroundRole(QPalette::PlaceholderText);
    mPathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ToplevelLayout->addWidget(mPathLabel);

    ToplevelLayout->addSpacing(16);

    ContentLayout = new QGridLayout();
    ToplevelLayout->addLayout(ContentLayout);

    InitMetadataBox();
    InitContentsBox();
    InitSettingsBox();
    InitStorageBox();
    InitHealthBox();

    ContentLayout->setColumnStretch(0, 1);
    ContentLayout->setColumnStretch(1, 1);

    ToplevelLayout->addStretch();
}

void OverviewWidget::InitMetadataBox() {
    auto *metadataBox = new QGroupBox(this);
    auto *boxLayout = new QVBoxLayout(metadataBox);
    boxLayout->addWidget(MakeSectionTitle("Metadata", metadataBox));

    auto *metadataContainerLayout = new QHBoxLayout();

    auto *iconLayout = new QVBoxLayout();
    mIconLabel = new QLabel(metadataBox);
    mIconLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    mIconLabel->setFixedSize(128, 128);
    iconLayout->addWidget(mIconLabel);

    auto *changeIcon = new QPushButton("Change Icon", metadataBox);
    connect(changeIcon, &QPushButton::clicked, this, &OverviewWidget::SetIconFromFile);
    iconLayout->addWidget(changeIcon);

    auto *resetIcon = new QPushButton("Reset Icon", metadataBox);
    resetIcon->setToolTip("Drops this database's own icon and goes back to the shared default.");
    connect(resetIcon, &QPushButton::clicked, this, &OverviewWidget::ResetIcon);
    iconLayout->addWidget(resetIcon);

    iconLayout->addStretch();

    auto *nameAndDescriptionLayout = new QFormLayout();

    mTitleField = new QLineEdit(metadataBox);
    mDescriptionField = new QPlainTextEdit(metadataBox);
    mVersionField = new QLineEdit(metadataBox);
    mAuthorField = new QLineEdit(metadataBox);

    mTitleField->setMaximumWidth(256);
    mDescriptionField->setMaximumWidth(400);
    mDescriptionField->setMinimumHeight(128);
    mDescriptionField->setWordWrapMode(QTextOption::WordWrap);
    mVersionField->setMaximumWidth(64);
    mAuthorField->setMaximumWidth(192);

    connect(mTitleField, &QLineEdit::textChanged, this, [this](const QString &text) {
        mOverviewLabel->setText(text);
    });
    connect(mTitleField, &QLineEdit::editingFinished, this, &OverviewWidget::CommitMetadata);
    connect(mVersionField, &QLineEdit::editingFinished, this, &OverviewWidget::CommitMetadata);
    connect(mAuthorField, &QLineEdit::editingFinished, this, &OverviewWidget::CommitMetadata);
    mDescriptionField->installEventFilter(this);

    nameAndDescriptionLayout->addRow("Title", mTitleField);
    nameAndDescriptionLayout->addRow("Description", mDescriptionField);
    nameAndDescriptionLayout->addRow("Version", mVersionField);
    nameAndDescriptionLayout->addRow("Author", mAuthorField);

    metadataContainerLayout->addLayout(iconLayout);
    metadataContainerLayout->addSpacing(16);
    metadataContainerLayout->addLayout(nameAndDescriptionLayout);
    metadataContainerLayout->addStretch();

    boxLayout->addLayout(metadataContainerLayout);
    ContentLayout->addWidget(metadataBox, 0, 0);
}

void OverviewWidget::InitContentsBox() {
    auto *contentsBox = new QGroupBox(this);
    auto *boxLayout = new QVBoxLayout(contentsBox);

    auto *titleLayout = new QHBoxLayout();
    titleLayout->addWidget(MakeSectionTitle("Contents", contentsBox));
    titleLayout->addStretch();

    auto *refreshButton = new QPushButton(QIcon(":/images/silk/arrow_refresh.png"), "", contentsBox);
    refreshButton->setToolTip("Recount the items and re-measure the file.");
    refreshButton->setFlat(true);
    connect(refreshButton, &QPushButton::clicked, this, &OverviewWidget::RefreshStatistics);
    titleLayout->addWidget(refreshButton);
    boxLayout->addLayout(titleLayout);

    // Two columns of counts, one row per item type. Each is a button because clicking it drives the
    // Item Browser to that type, which is the thing you want next after seeing the number.
    auto *grid = new QGridLayout();
    for (int i = 0; i < ItemTypeCount; i++) {
        auto type = static_cast<ItemType>(i);

        auto *button = new QToolButton(contentsBox);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setIcon(QIcon(GetIconFileFromItemType(type)));
        button->setAutoRaise(true);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setStyleSheet("QToolButton { padding: 3px; text-align: left; }");
        button->setText(QString("- %1").arg(QString::fromStdString(GetTableNameFromItemType(type))));
        button->setToolTip("Show these in the Content Browser.");
        connect(button, &QToolButton::clicked, this, [this, type]() {
            // The browser is a dock of the SDK window this page sits in.
            if (auto *sdk = qobject_cast<Sdk*>(window()); sdk != nullptr) {
                if (ItemBrowserWidget *browser = sdk->GetItemBrowser(); browser != nullptr) {
                    browser->setVisible(true);
                    browser->raise();
                    browser->ShowItemType(type);
                }
            }
        });

        mCountButtons[i] = button;
        grid->addWidget(button, i / 2, i % 2);
    }
    boxLayout->addLayout(grid);

    mUniverseLabel = new QLabel(contentsBox);
    mUniverseLabel->setWordWrap(true);
    mUniverseLabel->setForegroundRole(QPalette::PlaceholderText);
    boxLayout->addWidget(mUniverseLabel);

    boxLayout->addStretch();
    ContentLayout->addWidget(contentsBox, 0, 1);
}

void OverviewWidget::InitSettingsBox() {
    auto *settingsBox = new QGroupBox(this);
    auto *settingsMainLayout = new QVBoxLayout(settingsBox);
    settingsMainLayout->addWidget(MakeSectionTitle("Settings", settingsBox));

    mMutableCheckBox = new QCheckBox("Mutable", settingsBox);
    mMutableCheckBox->setToolTip(
        "Allow this database to be modified at runtime.\n\n"
        "When off, the emulator mounts this database read-only: places published over it and any "
        "DataStore writes go to the master database instead, which takes priority over this one. "
        "The file itself is never written to.\n\n"
        "This only affects the emulator. You can still edit the database here.");
    connect(mMutableCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        mDatabase->SetMutable(checked);
    });
    settingsMainLayout->addWidget(mMutableCheckBox);

    auto *sqlButton = new QPushButton(QIcon(":/images/silk/database_gear.png"), "Execute SQL", settingsBox);
    sqlButton->setToolTip("Run SQL directly against this database.");
    connect(sqlButton, &QPushButton::clicked, this, &OverviewWidget::OpenSqlConsole);
    settingsMainLayout->addWidget(sqlButton, 0, Qt::AlignLeft);

    settingsMainLayout->addStretch();
    ContentLayout->addWidget(settingsBox, 1, 0);
}

void OverviewWidget::InitStorageBox() {
    auto *storageBox = new QGroupBox(this);
    auto *boxLayout = new QVBoxLayout(storageBox);
    boxLayout->addWidget(MakeSectionTitle("Storage", storageBox));

    auto *form = new QFormLayout();
    mTotalSizeLabel = new QLabel(storageBox);
    mBlobSizeLabel = new QLabel(storageBox);
    mFreeSpaceLabel = new QLabel(storageBox);
    mAutoVacuumLabel = new QLabel(storageBox);

    form->addRow("Total", mTotalSizeLabel);
    form->addRow("Content", mBlobSizeLabel);
    form->addRow("Reclaimable", mFreeSpaceLabel);
    form->addRow("Auto-vacuum", mAutoVacuumLabel);
    boxLayout->addLayout(form);

    auto *buttonLayout = new QHBoxLayout();

    mTrimButton = new QPushButton("Trim", storageBox);
    mTrimButton->setToolTip(
        "Hands the free pages at the end of the file back to the operating system.\n\n"
        "Cheap, because it does not rewrite the database, but it only works when auto-vacuum is "
        "set to incremental.");
    connect(mTrimButton, &QPushButton::clicked, this, &OverviewWidget::TrimFreePages);
    buttonLayout->addWidget(mTrimButton);

    mCompactButton = new QPushButton("Compact", storageBox);
    mCompactButton->setToolTip(
        "Rebuilds the database from scratch, reclaiming every free page and defragmenting the rest.\n\n"
        "Reclaims the most space, but rewrites the whole file, so it is slow on a large database.");
    connect(mCompactButton, &QPushButton::clicked, this, &OverviewWidget::Compact);
    buttonLayout->addWidget(mCompactButton);

    mIncrementalButton = new QPushButton("Enable Trimming", storageBox);
    mIncrementalButton->setToolTip(
        "Switches this database to incremental auto-vacuum so that Trim works on it.\n\n"
        "SQLite fixes the vacuum mode when a database is created, so changing it means rewriting "
        "the file once, which is the same work Compact does. After that, trimming is cheap.");
    connect(mIncrementalButton, &QPushButton::clicked, this, &OverviewWidget::EnableIncrementalVacuum);
    buttonLayout->addWidget(mIncrementalButton);

    buttonLayout->addStretch();
    boxLayout->addLayout(buttonLayout);
    boxLayout->addStretch();

    ContentLayout->addWidget(storageBox, 1, 1);
}

void OverviewWidget::InitHealthBox() {
    auto *healthBox = new QGroupBox(this);
    auto *boxLayout = new QVBoxLayout(healthBox);
    boxLayout->addWidget(MakeSectionTitle("Health", healthBox));

    auto *form = new QFormLayout();
    mSchemaLabel = new QLabel(healthBox);
    mAccessLabel = new QLabel(healthBox);
    mMountLabel = new QLabel(healthBox);
    mMountLabel->setWordWrap(true);

    form->addRow("Schema", mSchemaLabel);
    form->addRow("Access", mAccessLabel);
    form->addRow("Emulator", mMountLabel);
    boxLayout->addLayout(form);

    auto *buttonLayout = new QHBoxLayout();

    auto *integrityButton = new QPushButton("Check Integrity", healthBox);
    integrityButton->setToolTip("Runs SQLite's own consistency check over the whole database.");
    connect(integrityButton, &QPushButton::clicked, this, &OverviewWidget::CheckIntegrity);
    buttonLayout->addWidget(integrityButton);

    auto *orphanButton = new QPushButton("Collect Orphaned Blobs", healthBox);
    orphanButton->setToolTip(
        "Deletes stored blobs that no item references any more.\n\n"
        "The space they were using becomes reclaimable, so this pairs with Trim or Compact.");
    connect(orphanButton, &QPushButton::clicked, this, &OverviewWidget::CollectOrphanedBlobs);
    buttonLayout->addWidget(orphanButton);

    buttonLayout->addStretch();
    boxLayout->addLayout(buttonLayout);
    boxLayout->addStretch();

    ContentLayout->addWidget(healthBox, 2, 0, 1, 2);
}

bool OverviewWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == mDescriptionField && event->type() == QEvent::FocusOut)
        CommitMetadata();
    return QWidget::eventFilter(watched, event);
}

void OverviewWidget::CommitMetadata() {
    if (mDatabase == nullptr || mDatabase->Fail())
        return;

    const std::string title = mTitleField->text().toStdString();
    if (title != mDatabase->GetTitle())
        mDatabase->SetTitle(title);

    const std::string description = mDescriptionField->toPlainText().toStdString();
    if (description != mDatabase->GetDescription())
        mDatabase->SetDescription(description);

    const std::string version = mVersionField->text().toStdString();
    if (version != mDatabase->GetVersion())
        mDatabase->SetVersion(version);

    const std::string author = mAuthorField->text().toStdString();
    if (author != mDatabase->GetAuthor())
        mDatabase->SetAuthor(author);
}

void OverviewWidget::ShowIcon() {
    QImage image;
    image.loadFromData(mDatabase->GetIcon());
    if (image.isNull())
        image.loadFromData(EmuDb::GetDefaultIconData());

    mIconLabel->setPixmap(QPixmap::fromImage(image).scaled(128, 128, Qt::KeepAspectRatio,
                                                           Qt::SmoothTransformation));
}

void OverviewWidget::SetIconFromFile() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Change Icon",
        QString(),
        "Image File (*.png *.jpg *.jpeg *.bmp *.gif)"
    );
    if (filePath.isEmpty())
        return;

    QImageReader reader(filePath);
    if (!reader.canRead()) {
        QMessageBox::critical(this, "Cannot Use Icon",
            QString("\"%1\" could not be read as an image.").arg(QFileInfo(filePath).fileName()));
        return;
    }

    QFileInfo info(filePath);
    if (info.size() > kMaxIconBytes) {
        QMessageBox::critical(this, "Icon Too Large",
            QString("That image is %1. An icon is stored inside the database and re-read every time "
                    "a database list is shown, so it has to stay under %2.")
                .arg(FormatBytes(info.size()))
                .arg(FormatBytes(kMaxIconBytes)));
        return;
    }

    std::ifstream file(filePath.toStdString(), std::ios::binary);
    if (!file.is_open()) {
        QMessageBox::critical(this, "Error", "Unable to open file");
        return;
    }

    std::vector<unsigned char> buffer(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    mDatabase->SetIcon(buffer);
    ShowIcon();
}

void OverviewWidget::ResetIcon() {
    mDatabase->SetIcon({});
    ShowIcon();
}

void OverviewWidget::Refresh() {
    RefreshMetadata();
    if (!mStatisticsLoaded)
        RefreshStatistics();
}

void OverviewWidget::RefreshAll() {
    RefreshMetadata();
    RefreshStatistics();
}

void OverviewWidget::RefreshMetadata() {
    if (mOverviewLabel != nullptr)
        mOverviewLabel->setText(QString::fromStdString(mDatabase->GetTitle()));

    if (mPathLabel != nullptr) {
        mPathLabel->setText(mDatabase->IsMemory()
            ? "Not saved to disk yet"
            : QString::fromStdString(mDatabase->GetFilePath().string()));
    }

    if (mIconLabel != nullptr)
        ShowIcon();

    if (mTitleField != nullptr) {
        QSignalBlocker blocker(mTitleField);
        mTitleField->setText(QString::fromStdString(mDatabase->GetTitle()));
    }
    if (mDescriptionField != nullptr) {
        QSignalBlocker blocker(mDescriptionField);
        mDescriptionField->setPlainText(QString::fromStdString(mDatabase->GetDescription()));
    }
    if (mVersionField != nullptr) {
        QSignalBlocker blocker(mVersionField);
        mVersionField->setText(QString::fromStdString(mDatabase->GetVersion()));
    }
    if (mAuthorField != nullptr) {
        QSignalBlocker blocker(mAuthorField);
        mAuthorField->setText(QString::fromStdString(mDatabase->GetAuthor()));
    }
    if (mMutableCheckBox != nullptr) {
        QSignalBlocker blocker(mMutableCheckBox);
        mMutableCheckBox->setChecked(mDatabase->IsMutable());
    }
}

void OverviewWidget::RefreshStatistics() {
    mStatisticsLoaded = true;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    ////////// Contents //////////
    for (int i = 0; i < ItemTypeCount; i++) {
        auto type = static_cast<ItemType>(i);
        int64_t count = mDatabase->CountItems(type);
        mCountButtons[i]->setText(QString("%1 %2")
            .arg(FormatCount(count))
            .arg(QString::fromStdString(GetTableNameFromItemType(type))));
        mCountButtons[i]->setEnabled(count > 0);
    }

    std::vector<int64_t> universes = mDatabase->ListUniverseIds(false, 8, 0);
    for (int64_t id : mDatabase->ListUniverseIds(true, 8, 0))
        universes.push_back(id);

    if (universes.empty()) {
        mUniverseLabel->clear();
    } else {
        QStringList described;
        int64_t placeTotal = 0;
        for (int64_t id : universes) {
            std::optional<EmuDb::UniverseSummary> summary = mDatabase->GetUniverseSummary(id);
            placeTotal += static_cast<int64_t>(mDatabase->ListUniversePlaceIds(id).size());
            if (described.size() < 4 && summary.has_value() && !summary->Name.empty())
                described << QString::fromStdString(summary->Name);
        }
        QString text = described.join(", ");
        if (static_cast<int>(universes.size()) > described.size())
            text += QString(" and %1 more").arg(universes.size() - described.size());
        mUniverseLabel->setText(QString("%1 · %2 place%3")
            .arg(text).arg(placeTotal).arg(placeTotal == 1 ? "" : "s"));
    }

    ////////// Storage //////////
    EmuDb::StorageStats stats = mDatabase->GetStorageStats();
    mTotalSizeLabel->setText(QString("%1 across %2 pages of %3")
        .arg(FormatBytes(stats.TotalBytes()))
        .arg(FormatCount(stats.PageCount))
        .arg(FormatBytes(stats.PageSize)));
    mBlobSizeLabel->setText(QString("%1 in %2 blobs")
        .arg(FormatBytes(stats.BlobBytes))
        .arg(FormatCount(stats.BlobCount)));
    mFreeSpaceLabel->setText(QString("%1 across %2 free pages")
        .arg(FormatBytes(stats.FreeBytes()))
        .arg(FormatCount(stats.FreePages)));

    EmuDb::AutoVacuumMode mode = mDatabase->GetAutoVacuumMode();
    switch (mode) {
    case EmuDb::AutoVacuumMode::Incremental: mAutoVacuumLabel->setText("Incremental"); break;
    case EmuDb::AutoVacuumMode::Full:        mAutoVacuumLabel->setText("Full"); break;
    default:                                 mAutoVacuumLabel->setText("None"); break;
    }

    const bool writable = !mDatabase->IsReadOnly();
    mTrimButton->setEnabled(writable && mode == EmuDb::AutoVacuumMode::Incremental);
    mCompactButton->setEnabled(writable);
    mIncrementalButton->setEnabled(writable && mode != EmuDb::AutoVacuumMode::Incremental);

    ////////// Health //////////
    mSchemaLabel->setText(mDatabase->IsSchemaUpToDate()
        ? "Up to date"
        : "Older than this build - it will be migrated when saved");
    mAccessLabel->setText(writable ? "Read/write" : "Read-only");

    EmuDbManager *manager = gApp->GetCore()->GetEmuDbManager();
    EmuDb *mounted = mDatabase->IsMemory()
        ? nullptr
        : manager->GetDbFromFilePath(mDatabase->GetFilePath());
    if (mounted == nullptr) {
        mMountLabel->setText("Not mounted");
    } else {
        mMountLabel->setText("Mounted - the emulator has this file open too, so saving may fail "
                             "while it is running.");
    }

    QApplication::restoreOverrideCursor();
}

bool OverviewWidget::ConfirmCommitBefore(const QString &actionDescription) {
    if (!mDatabase->IsDirty())
        return true;

    QMessageBox::StandardButton res = QMessageBox::question(this, "Save First?",
        QString("%1 has to save this project's unsaved changes before it can run, and that cannot "
                "be undone.\n\nSave and continue?").arg(actionDescription),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    return res == QMessageBox::Yes;
}

void OverviewWidget::Compact() {
    if (!ConfirmCommitBefore("Compacting"))
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    int64_t before = mDatabase->GetStorageStats().TotalBytes();
    SqlDb::Response res = mDatabase->Vacuum();
    int64_t after = mDatabase->GetStorageStats().TotalBytes();
    QApplication::restoreOverrideCursor();

    if (res != SqlDb::Response::Success) {
        QMessageBox::critical(this, "Compact Failed",
            QString("The database could not be compacted.\n%1")
                .arg(QString::fromStdString(mDatabase->GetLastErrorMsg())));
    } else {
        QMessageBox::information(this, "Compacted",
            QString("Compacted from %1 to %2.").arg(FormatBytes(before)).arg(FormatBytes(after)));
    }
    RefreshStatistics();
}

void OverviewWidget::TrimFreePages() {
    if (!ConfirmCommitBefore("Trimming"))
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    int64_t before = mDatabase->GetStorageStats().TotalBytes();
    SqlDb::Response res = mDatabase->TrimFreePages();
    int64_t after = mDatabase->GetStorageStats().TotalBytes();
    QApplication::restoreOverrideCursor();

    if (res == SqlDb::Response::DidNothing) {
        QMessageBox::information(this, "Nothing to Trim",
            "This database is not set to incremental auto-vacuum, so there is nothing for trimming "
            "to hand back. Use Enable Trimming to switch it over, or Compact for a one-off.");
    } else if (res != SqlDb::Response::Success) {
        QMessageBox::critical(this, "Trim Failed",
            QString("The free pages could not be released.\n%1")
                .arg(QString::fromStdString(mDatabase->GetLastErrorMsg())));
    } else {
        QMessageBox::information(this, "Trimmed",
            QString("Released %1, taking the database from %2 to %3.")
                .arg(FormatBytes(before - after)).arg(FormatBytes(before)).arg(FormatBytes(after)));
    }
    RefreshStatistics();
}

void OverviewWidget::EnableIncrementalVacuum() {
    if (!ConfirmCommitBefore("Enabling trimming"))
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    SqlDb::Response res = mDatabase->SetAutoVacuumMode(EmuDb::AutoVacuumMode::Incremental);
    QApplication::restoreOverrideCursor();

    if (res != SqlDb::Response::Success && res != SqlDb::Response::DidNothing) {
        QMessageBox::critical(this, "Could Not Enable Trimming",
            QString("The auto-vacuum mode could not be changed.\n%1")
                .arg(QString::fromStdString(mDatabase->GetLastErrorMsg())));
    } else {
        QMessageBox::information(this, "Trimming Enabled",
            "This database now releases free pages incrementally. Trim will reclaim space from here "
            "on without rewriting the whole file.");
    }
    RefreshStatistics();
}

void OverviewWidget::CheckIntegrity() {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    std::string message;
    bool ok = mDatabase->CheckIntegrity(&message);
    QApplication::restoreOverrideCursor();

    if (ok) {
        QMessageBox::information(this, "Integrity Check", "SQLite reported no problems.");
    } else {
        QMessageBox::warning(this, "Integrity Check",
            QString("SQLite reported problems:\n\n%1").arg(QString::fromStdString(message)));
    }
}

void OverviewWidget::CollectOrphanedBlobs() {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    int64_t removed = mDatabase->GarbageCollectOrphanedBlobs();
    QApplication::restoreOverrideCursor();

    if (removed == 0) {
        QMessageBox::information(this, "Nothing to Collect", "Every stored blob is still referenced.");
    } else {
        QMessageBox::information(this, "Collected",
            QString("Deleted %1 orphaned blob%2. The space they held is now reclaimable - use Trim "
                    "or Compact to hand it back.")
                .arg(FormatCount(removed)).arg(removed == 1 ? "" : "s"));
    }
    RefreshStatistics();
}

void OverviewWidget::OpenSqlConsole() {
    ExecuteSqlDialog dialog(mDatabase, this);
    dialog.exec();
    // Whatever was run may have changed the counts, the sizes, or the metadata.
    RefreshAll();
}
