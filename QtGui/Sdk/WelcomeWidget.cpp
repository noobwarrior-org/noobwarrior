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
// File: WelcomeWidget.cpp
// Started by: Hattozo
// Started on: 11/27/2025
// Description: Starting page for Database Editor
#include "WelcomeWidget.h"

#include "Sdk.h"
#include "EmuDbListWidget.h"
#include "../Application.h"

#include <NoobWarrior/NoobWarrior.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPalette>
#include <QShowEvent>
#include <QToolButton>
#include <QUrl>

#include <system_error>

using namespace NoobWarrior;

static constexpr int RecentPathRole = Qt::UserRole;

static QFont SectionFont() {
    QFont font(QApplication::font().family(), 12);
    font.setBold(true);
    return font;
}

static QToolButton* MakeActionButton(QAction* action, QWidget* parent) {
    auto* button = new QToolButton(parent);
    button->setDefaultAction(action);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::StrongFocus);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setStyleSheet("QToolButton { padding: 5px; text-align: left; }");
    return button;
}

static void AddPlaceholderItem(QListWidget* list, const QString& text) {
    auto* item = new QListWidgetItem(text, list);
    item->setFlags(Qt::NoItemFlags);
    QFont font = item->font();
    font.setItalic(true);
    item->setFont(font);
    item->setForeground(list->palette().brush(QPalette::Disabled, QPalette::Text));
}

WelcomeWidget::WelcomeWidget(Sdk* sdk, QWidget *parent) : QWidget(parent),
    mSdk(sdk),
    mLayout(nullptr),
    mHeader(nullptr),
    mSubHeader(nullptr),
    mStartFrame(nullptr),
    mStartLayout(nullptr),
    mStartLabel(nullptr),
    mRecentFrame(nullptr),
    mRecentLayout(nullptr),
    mRecentLabel(nullptr),
    mRecentList(nullptr),
    mDatabasesFrame(nullptr),
    mDatabasesLayout(nullptr),
    mDatabasesLabel(nullptr),
    mDatabasesList(nullptr),
    mDatabasesHint(nullptr),
    mOpenFolderButton(nullptr)
{
    InitWidgets();
    Refresh();
}

void WelcomeWidget::InitWidgets() {
    mLayout = new QGridLayout(this);
    mLayout->setContentsMargins(24, 24, 24, 24);
    mLayout->setHorizontalSpacing(32);
    mLayout->setVerticalSpacing(16);

    auto* headerFrame = new QFrame(this);
    auto* headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(12);

    auto* headerIcon = new QLabel(headerFrame);
    headerIcon->setPixmap(QPixmap(":/images/icon64.png").scaled(48, 48, Qt::KeepAspectRatio,
                                                               Qt::SmoothTransformation));
    headerLayout->addWidget(headerIcon, 0, Qt::AlignTop);

    auto* headerTextFrame = new QFrame(headerFrame);
    auto* headerTextLayout = new QVBoxLayout(headerTextFrame);
    headerTextLayout->setContentsMargins(0, 0, 0, 0);
    headerTextLayout->setSpacing(2);

    mHeader = new QLabel("Welcome", headerTextFrame);
    mHeader->setFont(QFont(QApplication::font().family(), 20));
    headerTextLayout->addWidget(mHeader);

    mSubHeader = new QLabel("Create and edit databases and plugins for " NOOBWARRIOR_BRAND ".", headerTextFrame);
    mSubHeader->setForegroundRole(QPalette::PlaceholderText);
    headerTextLayout->addWidget(mSubHeader);

    headerLayout->addWidget(headerTextFrame, 1);
    mLayout->addWidget(headerFrame, 0, 0, 1, 2);

    InitStartSection();
    InitRecentSection();
    InitDatabasesSection();

    mLayout->addWidget(mStartFrame, 1, 0);
    mLayout->addWidget(mRecentFrame, 2, 0);
    mLayout->addWidget(mDatabasesFrame, 1, 1, 2, 1);

    mLayout->setColumnStretch(0, 1);
    mLayout->setColumnStretch(1, 1);
    mLayout->setRowStretch(2, 1);
}

void WelcomeWidget::InitStartSection() {
    mStartFrame = new QFrame(this);
    mStartLayout = new QVBoxLayout(mStartFrame);
    mStartLayout->setContentsMargins(0, 0, 0, 0);
    mStartLayout->setSpacing(2);

    mStartLabel = new QLabel("Start", mStartFrame);
    mStartLabel->setFont(SectionFont());
    mStartLayout->addWidget(mStartLabel);

    if (mSdk == nullptr)
        return;

    mStartLayout->addWidget(MakeActionButton(mSdk->GetNewProjectAction(), mStartFrame));
    mStartLayout->addWidget(MakeActionButton(mSdk->GetOpenProjectAction(), mStartFrame));

    /* useless
    QToolButton* backupButton = MakeActionButton(mSdk->GetBackupAction(), mStartFrame);
    backupButton->setToolTip("Copies a game and everything it references out of Roblox and into the "
                             "open database. Open a database first.");
    mStartLayout->addWidget(backupButton);
    */
}

void WelcomeWidget::InitRecentSection() {
    mRecentFrame = new QFrame(this);
    mRecentLayout = new QVBoxLayout(mRecentFrame);
    mRecentLayout->setContentsMargins(0, 0, 0, 0);
    mRecentLayout->setSpacing(4);

    mRecentLabel = new QLabel("Recent", mRecentFrame);
    mRecentLabel->setFont(SectionFont());
    mRecentLayout->addWidget(mRecentLabel);

    mRecentList = new QListWidget(mRecentFrame);
    mRecentList->setIconSize(QSize(24, 24));
    mRecentList->setSelectionMode(QAbstractItemView::SingleSelection);
    mRecentList->setContextMenuPolicy(Qt::CustomContextMenu);
    mRecentList->setStyleSheet(R"(
    QListWidget::item {
        padding: 3px 3px 3px 6px;
    }
    )");
    mRecentLayout->addWidget(mRecentList, 1);

    connect(mRecentList, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        if (item != nullptr)
            OpenPath(item->data(RecentPathRole).toString(), true);
    });
    connect(mRecentList, &QListWidget::customContextMenuRequested,
            this, &WelcomeWidget::OnRecentContextMenu);
}

void WelcomeWidget::InitDatabasesSection() {
    mDatabasesFrame = new QFrame(this);
    mDatabasesLayout = new QVBoxLayout(mDatabasesFrame);
    mDatabasesLayout->setContentsMargins(0, 0, 0, 0);
    mDatabasesLayout->setSpacing(4);

    mDatabasesLabel = new QLabel("Installed Databases", mDatabasesFrame);
    mDatabasesLabel->setFont(SectionFont());
    mDatabasesLayout->addWidget(mDatabasesLabel);

    mDatabasesList = new EmuDbListWidget(EmuDbListWidget::Mode::ShowEntriesInDir, mDatabasesFrame);
    mDatabasesList->setSelectionMode(QAbstractItemView::SingleSelection);
    mDatabasesList->setAcceptDrops(true);
    mDatabasesLayout->addWidget(mDatabasesList, 1);

    connect(mDatabasesList, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        if (item != nullptr)
            OpenPath(item->data(EmuDbListWidget::FilePathRole).toString(), false);
    });

    connect(mDatabasesList, &EmuDbListWidget::filesDropped, this, [this](const QStringList& paths) {
        for (const QString& path : paths)
            OpenPath(path, false);
    });

    auto* bottomFrame = new QFrame(mDatabasesFrame);
    auto* bottomLayout = new QHBoxLayout(bottomFrame);
    bottomLayout->setContentsMargins(0, 0, 0, 0);

    mDatabasesHint = new QLabel(bottomFrame);
    mDatabasesHint->setForegroundRole(QPalette::PlaceholderText);
    mDatabasesHint->setWordWrap(true);
    bottomLayout->addWidget(mDatabasesHint, 1);

    mOpenFolderButton = new QPushButton(QIcon(":/images/silk/folder_database.png"),
                                        "Open Databases Folder", bottomFrame);
    connect(mOpenFolderButton, &QPushButton::clicked, this, []() {
        std::filesystem::path dbDir = gApp->GetCore()->GetUserDataDir() / NW_PATH_DATABASES;
        QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(dbDir.string())));
    });
    bottomLayout->addWidget(mOpenFolderButton, 0);

    mDatabasesLayout->addWidget(bottomFrame);
}

void WelcomeWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    Refresh();
}

void WelcomeWidget::Refresh() {
    RefreshRecent();

    if (mDatabasesList != nullptr) {
        mDatabasesList->Refresh();

        if (mDatabasesHint != nullptr) {
            mDatabasesHint->setText(mDatabasesList->count() > 0
                ? "Double-click a database to open it."
                : "No databases installed yet. Create one with New Project, or drop a .nwdb file here.");
        }
    }
}

void WelcomeWidget::RefreshRecent() {
    if (mRecentList == nullptr)
        return;

    mRecentList->clear();

    QStringList paths = Sdk::GetRecentProjects();
    if (paths.isEmpty()) {
        AddPlaceholderItem(mRecentList, "No recent projects yet.");
        return;
    }

    for (const QString& path : paths) {
        QFileInfo info(path);
        std::filesystem::path fsPath(path.toStdString());
        
        bool exists = info.exists() && info.isFile();

        QString title = exists ? QString::fromStdString(EmuDb::ProbeTitle(fsPath)) : QString();
        QIcon icon = exists ? EmuDbListWidget::IconFromData(EmuDb::ProbeIcon(fsPath))
                            : QIcon(":/images/silk/database_error.png");
        
        auto* item = new QListWidgetItem(icon, info.fileName(), mRecentList);
        item->setData(RecentPathRole, path);

        if (exists) {
            item->setToolTip(title.isEmpty() ? path : QString("%1\n\n%2").arg(title).arg(path));
        } else {
            item->setToolTip(QString("%1\n\nThis file no longer exists.").arg(path));
            QFont font = item->font();
            font.setItalic(true);
            item->setFont(font);
            item->setForeground(mRecentList->palette().brush(QPalette::Disabled, QPalette::Text));
        }
    }
}

void WelcomeWidget::OpenPath(const QString &path, bool isRecentEntry) {
    if (mSdk == nullptr || path.isEmpty())
        return;

    QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        if (!isRecentEntry) {
            QMessageBox::warning(this, "Cannot Open Project",
                QString("\"%1\" could not be found.").arg(path));
            Refresh();
            return;
        }

        QMessageBox::StandardButton res = QMessageBox::question(this, "Cannot Open Project",
            QString("\"%1\" could not be found. It may have been moved, renamed or deleted."
                    "\n\nRemove it from the recent list?").arg(path),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (res == QMessageBox::Yes)
            RemoveRecentEntry(path);
        else
            Refresh();
        return;
    }

    mSdk->AddProjectFromPath(std::filesystem::path(path.toStdString()));
    Refresh();
}

void WelcomeWidget::OnRecentContextMenu(const QPoint &pos) {
    if (mRecentList == nullptr)
        return;

    QListWidgetItem* item = mRecentList->itemAt(pos);
    QString path = item != nullptr ? item->data(RecentPathRole).toString() : QString();

    QMenu menu(this);

    if (!path.isEmpty()) {
        QAction* openAction = menu.addAction(QIcon(":/images/silk/database_go.png"), "Open");
        connect(openAction, &QAction::triggered, this, [this, path]() { OpenPath(path, true); });
        openAction->setEnabled(mSdk != nullptr);

        QAction* folderAction = menu.addAction(QIcon(":/images/silk/folder_database.png"),
                                               "Open Containing Folder");
        connect(folderAction, &QAction::triggered, this, [path]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
        });
        folderAction->setEnabled(QFileInfo(path).exists());

        menu.addSeparator();

        QAction* removeAction = menu.addAction(QIcon(":/images/silk/database_delete.png"),
                                               "Remove from Recent List");
        connect(removeAction, &QAction::triggered, this, [this, path]() { RemoveRecentEntry(path); });
    }

    QAction* clearAction = menu.addAction("Clear Recent List");
    clearAction->setEnabled(!Sdk::GetRecentProjects().isEmpty());
    connect(clearAction, &QAction::triggered, this, [this]() {
        Sdk::SetRecentProjects({});
        RefreshRecent();
    });

    menu.exec(mRecentList->viewport()->mapToGlobal(pos));
}

void WelcomeWidget::RemoveRecentEntry(const QString &path) {
    QStringList paths = Sdk::GetRecentProjects();
#if defined(_WIN32)
    const Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#endif
    paths.removeIf([&](const QString& other) { return other.compare(path, sensitivity) == 0; });

    Sdk::SetRecentProjects(paths);
    RefreshRecent();
}
