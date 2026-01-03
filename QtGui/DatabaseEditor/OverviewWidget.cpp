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
#include <QGroupBox>
#include <QMessageBox>
#include <QMediaPlayer>

using namespace NoobWarrior;

OverviewWidget::OverviewWidget(Database *db, QWidget *parent) : QWidget(parent),
    mDatabase(db),
    ToplevelLayout(nullptr),
    ContentLayout(nullptr)
{
    setWindowTitle(QString::fromStdString(mDatabase->GetTitle()));
    InitWidgets();
}

void OverviewWidget::InitWidgets() {
    ToplevelLayout = new QVBoxLayout(this);
    ToplevelLayout->setContentsMargins(32, 32, 32, 32);

    auto *overviewLabel = new QLabel(QString::fromStdString(mDatabase->GetTitle()));
    overviewLabel->setFont(QFont(QApplication::font().family(), 24));

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

    connect(titleField, &QLineEdit::textChanged, [&, overviewLabel](const QString &text) {
        mDatabase->SetTitle(text.toStdString());
        overviewLabel->setText(QString::fromStdString(mDatabase->GetTitle()));
    });

    nameAndDescriptionLayout->addRow("Title", titleField);
    nameAndDescriptionLayout->addRow("Description", descriptionField);
    nameAndDescriptionLayout->addRow("Version", versionField);
    nameAndDescriptionLayout->addRow("Author", authorField);

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
