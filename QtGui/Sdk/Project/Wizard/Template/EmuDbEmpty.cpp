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
// File: EmuDbEmpty.cpp
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#include "EmuDbEmpty.h"
#include "TemplatePage.h"
#include "Sdk/Sdk.h"
#include "Sdk/Project/EmuDb/EmuDbProject.h"
#include "Application.h"

#include <NoobWarrior/Log.h>
#include <filesystem>
#include <qmessagebox.h>

using namespace NoobWarrior;

EmuDbEmptyIntroPage::EmuDbEmptyIntroPage(QWidget* parent) : TemplatePage(parent) {
    setTitle("Setup Information");
    setSubTitle("Set your database's name, description, and other information here.");

    mMainLayout = new QVBoxLayout(this);
    mFormLayout = new QFormLayout();
    mMainLayout->addLayout(mFormLayout);

    mPathEdit = new QLineEdit();
    mPathEdit->setText(QString::fromStdString((gApp->GetCore()->GetUserDataDir() / "databases").string()));
    mFormLayout->addRow(new QLabel("File Path"), mPathEdit);

    mIconFrame = new QFrame();
    mIconFrame->setFrameShape(QFrame::Box);
    mIconFrame->setFrameShadow(QFrame::Sunken);
    mIconFrame->setAutoFillBackground(true);

    mIconFrameLayout = new QVBoxLayout(mIconFrame);
    mIconFrameLayout->setAlignment(Qt::AlignCenter);

    mIcon = new QLabel();
    mIcon->setPixmap(QPixmap(":/images/db_96x96.png"));
    mIcon->setAlignment(Qt::AlignCenter);

    mChangeIconButton = new QPushButton("Change Icon");
    
    mIconFrameLayout->addWidget(mIcon);
    mIconFrameLayout->addWidget(mChangeIconButton);

    mFormLayout->addRow(new QLabel("Icon"), mIconFrame);

    mTitleEdit = new QLineEdit();
    mTitleEdit->setPlaceholderText("My Cool Game");
    mTitleEdit->setClearButtonEnabled(true);
    mFormLayout->addRow(new QLabel("Title"), mTitleEdit);

    mDescriptionEdit = new QPlainTextEdit();
    mDescriptionEdit->setPlaceholderText("A brief description of your project.");
    mFormLayout->addRow(new QLabel("Description"), mDescriptionEdit);

    mAuthorEdit = new QLineEdit();
    QString defaultAuthor = qgetenv("USER");
    if (defaultAuthor.isEmpty()) defaultAuthor = qgetenv("USERNAME");
    mAuthorEdit->setText(defaultAuthor);
    mFormLayout->addRow(new QLabel("Author"), mAuthorEdit);

    mVersionEdit = new QLineEdit("1.0.0");
    mFormLayout->addRow(new QLabel("Version"), mVersionEdit);

    connect(mTitleEdit, &QLineEdit::textChanged, [this]() {
        std::filesystem::path path(gApp->GetCore()->GetUserDataDir() / "databases");
        QString fileName = mTitleEdit->text().toLower().replace(" ", "_") + ".nwdb";
        path /= fileName.toStdString();
        mPathEdit->setText(QString::fromStdString(path));
        completeChanged();
    });

    connect(mTitleEdit, &QLineEdit::textChanged, this, &EmuDbEmptyIntroPage::completeChanged);
}

bool EmuDbEmptyIntroPage::validatePage() {
    if (!isComplete())
        return false;
    bool res = TemplatePage::validatePage();
    if (res) {
        Sdk* sdk = dynamic_cast<Sdk*>(wizard()->parent());
        if (sdk == nullptr) {
            Out("EmuDbEmptyIntroPage", "Failed to create project: Sdk is not a parent of wizard");
            return false;
        }
        auto project = new EmuDbProject(mPathEdit->text().toStdString());
        if (project->Fail()) {
            auto error = QMessageBox::critical(this,
                "Cannot Create Project",
                QString("Failed to create the project.\nMessage received: \"%1\"")
                    .arg(project->GetFailMsg())
            );
        }
        sdk->AddProject(project);
    }
    return res;
}

bool EmuDbEmptyIntroPage::isComplete() const {
    if (std::filesystem::exists(mPathEdit->text().toStdString()) || std::filesystem::is_directory(mPathEdit->text().toStdString()))
        return false;
    return !mPathEdit->text().isEmpty() && !mTitleEdit->text().isEmpty();
}

int EmuDbEmptyIntroPage::nextId() const {
    return -1;
}

QString EmuDbEmptyIntroPage::GetName() {
    return "Empty Database";
}

QString EmuDbEmptyIntroPage::GetDescription() {
    return "A database stores Roblox items like games and models. It is intended to be either used for retrieving Roblox assets from a server emulator for gameplay consumption, or to serve as an archive for game preservation.\n\nIf you're trying to create a game or model, pick this option.";
}

QIcon EmuDbEmptyIntroPage::GetIcon() {
    return QIcon(":/images/db_96x96.png");
}