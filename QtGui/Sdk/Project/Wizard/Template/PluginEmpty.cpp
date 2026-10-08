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
// File: PluginEmpty.cpp
// Started by: Hattozo
// Started on: 2/2/2026
// Description:
#include "PluginEmpty.h"
#include "../ProjectWizard.h"
#include "Application.h"
#include "Sdk/Project/Plugin/PluginProject.h"

#include <QMessageBox>
#include <QFileDialog>

using namespace NoobWarrior;

PluginEmptyIntroPage::PluginEmptyIntroPage(QWidget *parent) : TemplatePage(parent) {
    setTitle("Setup Information");
    setSubTitle("Set your plugin's name, identifier, and other information here.");

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
    mIcon->setPixmap(QPixmap(":/images/empty_database_96x96.png"));
    mIcon->setProperty("path", "");
    mIcon->setAlignment(Qt::AlignCenter);

    mChangeIconButton = new QPushButton("Change Icon");

    connect(mChangeIconButton, &QPushButton::clicked, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this, "Select Icon", QDir::currentPath(), "Image File (*.png *.jpg *.jpeg *.bmp *.gif)");
        mIcon->setProperty("path", filePath);
        
        std::ifstream file(filePath.toStdString(), std::ios::binary);

        if (!file.is_open()) {
            QMessageBox::critical(this, "Error", "Unable to open file");
            return;
        }

        std::vector<unsigned char> buffer(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        QImage image;
        image.loadFromData(buffer);

        QPixmap pixmap = QPixmap::fromImage(image);

        mIcon->setPixmap(pixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    });
    
    mIconFrameLayout->addWidget(mIcon);
    mIconFrameLayout->addWidget(mChangeIconButton);

    mFormLayout->addRow(new QLabel("Icon"), mIconFrame);

    mIdentifierEdit = new QLineEdit();
    mIdentifierEdit->setPlaceholderText("plugin@example.com");
    mFormLayout->addRow(new QLabel("Identifier"), mIdentifierEdit);

    mTitleEdit = new QLineEdit();
    mTitleEdit->setPlaceholderText("Really Cool Plugin");
    mFormLayout->addRow(new QLabel("Title"), mTitleEdit);
};

bool PluginEmptyIntroPage::validatePage() {
    if (!isComplete())
        return false;
    bool res = TemplatePage::validatePage();
    if (res) {
        Sdk* sdk = dynamic_cast<Sdk*>(wizard()->parent());
        if (sdk == nullptr) {
            gApp->GetCore()->Out("EmuDbEmptyIntroPage", "Failed to create project: Sdk is not a parent of wizard");
            return false;
        }
        auto project = new PluginProject(mPathEdit->text().toStdString());
        if (project->Fail()) {
            auto error = QMessageBox::critical(this,
                "Cannot Create Project",
                QString("Failed to create the project.\nMessage received: \"%1\"")
                    .arg(project->GetFailMsg())
            );
        }

        QString iconPath = mIcon->property("path").toString();
        
        if (!iconPath.isEmpty()) {
            std::ifstream file(iconPath.toStdString(), std::ios::binary);
            if (file.is_open()) {
                std::vector<unsigned char> buffer(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>()
                );

                // project->GetDb()->SetIcon(buffer);
            }
        }

        // project->GetDb()->SetTitle(mTitleEdit->text().toStdString());
        // project->GetDb()->SetDescription(mDescriptionEdit->toPlainText().toStdString());
        // project->GetDb()->SetAuthor(mAuthorEdit->text().toStdString());
        // project->GetDb()->SetVersion(mVersionEdit->text().toStdString());
        sdk->AddProject(project);
    }
    return res;
}

bool PluginEmptyIntroPage::isComplete() const {
    return !mIdentifierEdit->text().isEmpty() && !mTitleEdit->text().isEmpty();
}

int PluginEmptyIntroPage::nextId() const {
    return static_cast<int>(ProjectWizard::PageId::Intro);
}

QString PluginEmptyIntroPage::GetName() {
    return "Empty Plugin";
}

QString PluginEmptyIntroPage::GetDescription() {
    return "A plugin extends the functionality of noobWarrior by being able to access a powerful Lua API and modify the DataModel of any hosted server.\n\nIf you're trying to modify an existing game to add new functionality, or if you want to modify the functionality of noobWarrior itself, pick this option.";
}

QIcon PluginEmptyIntroPage::GetIcon() {
    return QIcon(":/images/plugin_96x96.png");
}
