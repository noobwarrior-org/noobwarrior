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

#include <QCheckBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QApplication>
#include <QSignalBlocker>

#include <fstream>
#include <QGroupBox>
#include <QMessageBox>
#include <QMediaPlayer>

using namespace NoobWarrior;

OverviewWidget::OverviewWidget(EmuDb *db, QWidget *parent) : QWidget(parent),
    mDatabase(db),
    ToplevelLayout(nullptr),
    ContentLayout(nullptr)
{
    setWindowTitle("Overview");
    InitWidgets();
}

void OverviewWidget::InitWidgets() {
    ToplevelLayout = new QVBoxLayout(this);
    ToplevelLayout->setContentsMargins(32, 32, 32, 32);

    auto *overviewLabel = new QLabel(QString::fromStdString(mDatabase->GetTitle()));
    overviewLabel->setFont(QFont(QApplication::font().family(), 24));
    mOverviewLabel = overviewLabel;

    auto *spacer1 = new QSpacerItem(16, 16);
    ToplevelLayout->addWidget(overviewLabel);
    ToplevelLayout->addItem(spacer1);

    ContentLayout = new QGridLayout(this);
    ToplevelLayout->addLayout(ContentLayout);

    auto *metadataAndThumbnailsLayout = new QHBoxLayout();
    auto *settingsAndChangelogLayout = new QHBoxLayout();

    ////////// Metadata //////////
    auto *metadataBox = new QGroupBox();
    metadataBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    auto *metadataContainerLayout = new QHBoxLayout();

    auto *iconLayout = new QVBoxLayout();
    QImage image;
    image.loadFromData(mDatabase->GetIcon());

    QPixmap pixmap = QPixmap::fromImage(image);

    auto *icon = new QLabel();
    icon->setAlignment(Qt::AlignLeft);
    icon->setPixmap(pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLayout->addWidget(icon);
    mIconLabel = icon;

    auto *changeIcon = new QPushButton("Change Icon");
    iconLayout->addWidget(changeIcon);
    connect(changeIcon, &QPushButton::clicked, [&, icon]() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Change Icon",
            QString(),
            "Image File (*.png *.jpg *.jpeg *.bmp *.gif)"
        );
        if (!filePath.isEmpty()) {
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

            QImage image;
            image.loadFromData(mDatabase->GetIcon());

            QPixmap pixmap = QPixmap::fromImage(image);

            icon->setPixmap(pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    });

    iconLayout->addStretch();

    auto *metadataSpacer = new QSpacerItem(16, 16);

    auto *nameAndDescriptionLayout = new QFormLayout();

    mTitleField = new QLineEdit(QString::fromStdString(mDatabase->GetTitle()));
    mDescriptionField = new QPlainTextEdit(QString::fromStdString(mDatabase->GetDescription()));
    mVersionField = new QLineEdit(QString::fromStdString(mDatabase->GetVersion()));
    mAuthorField = new QLineEdit(QString::fromStdString(mDatabase->GetAuthor()));

    mTitleField->setMaximumWidth(256);
    mDescriptionField->setMaximumWidth(400);
    mDescriptionField->setMinimumHeight(128);
    mDescriptionField->setWordWrapMode(QTextOption::WordWrap);
    mVersionField->setMaximumWidth(64);
    mAuthorField->setMaximumWidth(192);

    connect(mTitleField, &QLineEdit::textChanged, [&, overviewLabel](const QString &text) {
        mDatabase->SetTitle(text.toStdString());
        overviewLabel->setText(QString::fromStdString(mDatabase->GetTitle()));
    });

    nameAndDescriptionLayout->addRow("Title", mTitleField);
    nameAndDescriptionLayout->addRow("Description", mDescriptionField);
    nameAndDescriptionLayout->addRow("Version", mVersionField);
    nameAndDescriptionLayout->addRow("Author", mAuthorField);

    metadataContainerLayout->addLayout(iconLayout);
    metadataContainerLayout->addItem(metadataSpacer);
    metadataContainerLayout->addLayout(nameAndDescriptionLayout);

    metadataBox->setLayout(metadataContainerLayout);
    ContentLayout->addWidget(metadataBox, 0, 0);

    /*
    ////////// Thumbnails //////////
    auto *thumbnailsBox = new QGroupBox();
    auto *thumbnailsLayout = new QVBoxLayout();
    thumbnailsBox->setLayout(thumbnailsLayout);
    thumbnailsBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    auto *thumbnailViewer = new QMediaPlayer();

    ContentLayout->addWidget(thumbnailsBox, 0, 1);
    */

    ////////// Settings //////////
    auto *settingsBox = new QGroupBox();
    auto *settingsMainLayout = new QVBoxLayout();

    auto *settingsLabel = new QLabel("Settings");
    settingsLabel->setFont(QFont(QApplication::font().family(), 18));
    settingsMainLayout->addWidget(settingsLabel);

    auto *settingsContainerLayout = new QGridLayout();
    settingsContainerLayout->addWidget(new QCheckBox("Mutable"), 0, 0);
    settingsMainLayout->addLayout(settingsContainerLayout);

    settingsBox->setLayout(settingsMainLayout);
    ContentLayout->addWidget(settingsBox, 1, 0);

    /*
    ////////// Changelog //////////
    auto *changelogBox = new QGroupBox();
    auto *changelogLayout = new QVBoxLayout();
    changelogBox->setLayout(changelogLayout);
    changelogBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    auto *changelogLabel = new QLabel("Changelog");
    changelogLabel->setFont(QFont(QApplication::font().family(), 18));
    changelogLayout->addWidget(changelogLabel);    

    ContentLayout->addWidget(changelogBox, 1, 1);
    */

    auto *spacer2 = new QSpacerItem(64, 64);

    ToplevelLayout->addLayout(ContentLayout);
    // MainLayout->addStretch();
    ToplevelLayout->addItem(spacer2);
}

void OverviewWidget::Refresh() {
    if (mOverviewLabel != nullptr)
        mOverviewLabel->setText(QString::fromStdString(mDatabase->GetTitle()));

    if (mIconLabel != nullptr) {
        QImage image;
        image.loadFromData(mDatabase->GetIcon());
        QPixmap pixmap = QPixmap::fromImage(image);
        mIconLabel->setPixmap(pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

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
}
