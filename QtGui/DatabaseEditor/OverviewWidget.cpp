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

#include <fstream>
#include <QMessageBox>

using namespace NoobWarrior;

OverviewWidget::OverviewWidget(Database *db, QWidget *parent) : QWidget(parent),
    mDatabase(db),
    MainLayout(nullptr)
{
    setWindowTitle(QString::fromStdString(mDatabase->GetTitle()));
    InitWidgets();
}

void OverviewWidget::InitWidgets() {
    MainLayout = new QVBoxLayout(this);

    MainLayout->setContentsMargins(32, 32, 32, 32);

    auto *overviewLabel = new QLabel(QString::fromStdString(mDatabase->GetTitle()));
    overviewLabel->setFont(QFont("Source Sans Pro", 24, QFont::Bold));

    auto *spacer1 = new QSpacerItem(16, 16);
    MainLayout->addWidget(overviewLabel);
    MainLayout->addItem(spacer1);

    auto *metadataAndSettingsLayout = new QHBoxLayout();

    ////////// Metadata //////////
    auto *metadataLayout = new QVBoxLayout();

    auto *metadataLabel = new QLabel("Metadata");
    metadataLabel->setAlignment(Qt::AlignLeft);
    metadataLabel->setFont(QFont(QApplication::font().family(), 18));
    metadataLayout->addWidget(metadataLabel);

    auto *metadataContainerLayout = new QHBoxLayout();

    auto *iconLayout = new QVBoxLayout();
    QImage image;
    image.loadFromData(mDatabase->GetIcon());

    QPixmap pixmap = QPixmap::fromImage(image);

    auto *icon = new QLabel();
    icon->setAlignment(Qt::AlignLeft);
    icon->setPixmap(pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLayout->addWidget(icon);

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
            std::ifstream file(filePath.toStdString());

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

    auto *titleField = new QLineEdit(QString::fromStdString(mDatabase->GetTitle()));
    auto *descriptionField = new QPlainTextEdit(QString::fromStdString(mDatabase->GetDescription()));
    auto *versionField = new QLineEdit(QString::fromStdString(mDatabase->GetVersion()));
    auto *authorField = new QLineEdit(QString::fromStdString(mDatabase->GetAuthor()));

    titleField->setMaximumWidth(256);
    descriptionField->setMaximumWidth(400);
    descriptionField->setMinimumHeight(128);
    descriptionField->setWordWrapMode(QTextOption::WordWrap);
    versionField->setMaximumWidth(64);
    authorField->setMaximumWidth(192);

    connect(titleField, &QLineEdit::textChanged, [&](const QString &text) {
        mDatabase->SetTitle(text.toStdString());
    });

    nameAndDescriptionLayout->addRow("Title", titleField);
    nameAndDescriptionLayout->addRow("Description", descriptionField);
    nameAndDescriptionLayout->addRow("Version", versionField);
    nameAndDescriptionLayout->addRow("Author", authorField);

    metadataContainerLayout->addLayout(iconLayout);
    metadataContainerLayout->addItem(metadataSpacer);
    metadataContainerLayout->addLayout(nameAndDescriptionLayout);

    metadataLayout->addLayout(metadataContainerLayout);
    metadataAndSettingsLayout->addLayout(metadataLayout);

    ////////// Settings //////////
    auto *settingsLayout = new QVBoxLayout();

    auto *settingsLabel = new QLabel("Settings");
    settingsLabel->setAlignment(Qt::AlignLeft);
    settingsLabel->setFont(QFont(QApplication::font().family(), 18));
    settingsLayout->addWidget(settingsLabel);

    auto *settingsContainerLayout = new QFormLayout();
    settingsContainerLayout->addRow(new QCheckBox(), new QLabel("Mutable"));

    settingsLayout->addLayout(settingsContainerLayout);

    auto *metadataAndSettingsSpacer = new QSpacerItem(32, 32);
    metadataAndSettingsLayout->addItem(metadataAndSettingsSpacer);
    metadataAndSettingsLayout->addLayout(settingsLayout);

    auto *spacer2 = new QSpacerItem(64, 64);

    MainLayout->addLayout(metadataAndSettingsLayout);
    MainLayout->addStretch();
    MainLayout->addItem(spacer2);
}
