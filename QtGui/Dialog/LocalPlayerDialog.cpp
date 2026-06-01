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
// File: LocalPlayerDialog.cpp
// Started by: Hattozo
// Started on: 4/23/2026
// Description:
#include "LocalPlayerDialog.h"
#include "../Application.h"

#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QColorDialog>
#include <QInputDialog>
#include <QGroupBox>
#include <QLabel>
#include <QIntValidator>

using namespace NoobWarrior;

static constexpr const char* kAvatarType = "user.avatar.type";
static constexpr const char* kColorPrefix = "user.avatar.color.";  // + body part key
static constexpr const char* kItemsPrefix = "user.avatar.items.";  // + category key

LocalPlayerDialog::LocalPlayerDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Local Player Settings");
    InitWidgets();
    LoadFromRegistry();
}

LocalPlayerDialog::~LocalPlayerDialog() {}

void LocalPlayerDialog::InitWidgets() {
    Registry* reg = gApp->GetCore()->GetRegistry();

    mLayout = new QVBoxLayout(this);
    mMainLayout = new QHBoxLayout;
    mFormLayout = new QFormLayout;

    mIdInput = new QLineEdit;
    mNameInput = new QLineEdit;
    mDisplayNameInput = new QLineEdit;
    mAvatarTypeInput = new QComboBox;
    mAvatarTypeInput->addItems({"R6", "R15"});

    mIdInput->setText(QString::number(reg->GetKeyValue<int64_t>("user.id").value_or(5)));
    mNameInput->setText(QString::fromStdString(reg->GetKeyValue<std::string>("user.name").value_or("Player")));
    mDisplayNameInput->setText(QString::fromStdString(reg->GetKeyValue<std::string>("user.display_name").value_or("Player")));

    mIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mIdInput));
    mFormLayout->addRow("User Id", mIdInput);
    mFormLayout->addRow("Name", mNameInput);
    mFormLayout->addRow("Display Name", mDisplayNameInput);
    mFormLayout->addRow("Avatar Type", mAvatarTypeInput);
    
    QVBoxLayout* leftColumn = new QVBoxLayout;
    leftColumn->addLayout(mFormLayout);
    leftColumn->addWidget(BuildAvatarBody());
    leftColumn->addStretch();

    mMainLayout->addLayout(leftColumn);
    mMainLayout->addWidget(BuildItemEditor(), 1);

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(mButtonBox, &QDialogButtonBox::accepted, [this]() {
        SaveToRegistry();
        close();
    });
    connect(mButtonBox, &QDialogButtonBox::rejected, [this]() {
        close();
    });

    mLayout->addLayout(mMainLayout);
    mLayout->addWidget(mButtonBox);
}

QWidget* LocalPlayerDialog::BuildAvatarBody() {
    QGroupBox* box = new QGroupBox("Body Colors");
    QGridLayout* grid = new QGridLayout(box);
    grid->setHorizontalSpacing(4);
    grid->setVerticalSpacing(2);

    const QColor yellow("#F5CD30");
    const QColor blue("#0D69AC");
    const QColor green("#4B974B");

    AddBodyPart(grid, "head",     "Head",      0, 1, 48, 48, yellow);
    AddBodyPart(grid, "leftArm",  "Left Arm",  1, 0, 36, 72, yellow);
    AddBodyPart(grid, "torso",    "Torso",     1, 1, 72, 72, blue);
    AddBodyPart(grid, "rightArm", "Right Arm", 1, 2, 36, 72, yellow);

    QHBoxLayout* legs = new QHBoxLayout;
    legs->setSpacing(4);
    AddBodyPart(grid, "leftLeg",  "Left Leg",  -1, -1, 36, 72, green);
    AddBodyPart(grid, "rightLeg", "Right Leg", -1, -1, 36, 72, green);
    legs->addWidget(mBodyParts["leftLeg"].button);
    legs->addWidget(mBodyParts["rightLeg"].button);
    grid->addLayout(legs, 2, 1, Qt::AlignHCenter | Qt::AlignTop);

    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(2, 1);
    return box;
}

void LocalPlayerDialog::AddBodyPart(QGridLayout* grid, const QString& key, const QString& label,
                                    int row, int col, int w, int h, const QColor& defaultColor) {
    AvatarBodyPart part;
    part.key = key;
    part.label = label;
    part.color = defaultColor;
    part.button = new QPushButton;
    part.button->setFixedSize(w, h);
    part.button->setCursor(Qt::PointingHandCursor);
    part.button->setToolTip(QString("%1 - click to recolor").arg(label));
    mBodyParts.insert(key, part);

    AvatarBodyPart& stored = mBodyParts[key];
    ApplyBodyColor(stored);
    connect(stored.button, &QPushButton::clicked, [this, key]() {
        PickBodyColor(mBodyParts[key]);
    });

    if (row >= 0 && col >= 0)
        grid->addWidget(stored.button, row, col, Qt::AlignHCenter | Qt::AlignTop);
}

void LocalPlayerDialog::ApplyBodyColor(const AvatarBodyPart& part) {
    part.button->setStyleSheet(QString(
        "QPushButton { background-color: %1; border: 1px solid #1b1b1b; border-radius: 3px; }"
        "QPushButton:hover { border: 1px solid #ffffff; }")
        .arg(part.color.name()));
}

void LocalPlayerDialog::PickBodyColor(AvatarBodyPart& part) {
    QColor chosen = QColorDialog::getColor(part.color, this,
        QString("Pick %1 Color").arg(part.label));
    if (!chosen.isValid())
        return;
    part.color = chosen;
    ApplyBodyColor(part);
}

QWidget* LocalPlayerDialog::BuildItemEditor() {
    QTabWidget* tabs = new QTabWidget;

    const QList<QPair<QString, QString>> categories = {
        {"hats",        "Hats"},
        {"faces",       "Faces"},
        {"accessories", "Accessories"},
        {"gear",        "Gear"},
        {"clothing",    "Clothing"},
    };

    for (const auto& [key, label] : categories) {
        QWidget* page = new QWidget;
        QVBoxLayout* pageLayout = new QVBoxLayout(page);

        QListWidget* list = new QListWidget;
        list->setSelectionMode(QAbstractItemView::ExtendedSelection);
        pageLayout->addWidget(new QLabel(QString("Equipped %1 (asset ids)").arg(label)));
        pageLayout->addWidget(list);

        QHBoxLayout* buttons = new QHBoxLayout;
        QPushButton* addButton = new QPushButton("Add");
        QPushButton* removeButton = new QPushButton("Remove");
        buttons->addWidget(addButton);
        buttons->addWidget(removeButton);
        buttons->addStretch();
        pageLayout->addLayout(buttons);

        AvatarItemCategory category{key, label, list};
        mItemCategories.append(category);
        int index = mItemCategories.size() - 1;

        connect(addButton, &QPushButton::clicked, [this, index]() {
            AddItemToCategory(mItemCategories[index]);
        });
        connect(removeButton, &QPushButton::clicked, [this, index]() {
            RemoveSelectedItem(mItemCategories[index]);
        });

        tabs->addTab(page, label);
    }

    tabs->setMinimumWidth(260);
    return tabs;
}

void LocalPlayerDialog::AddItemToCategory(AvatarItemCategory& category) {
    bool ok = false;
    int64_t assetId = QInputDialog::getInt(this,
        QString("Add %1").arg(category.label),
        "Asset Id:", 0, 0, 2147483647, 1, &ok);
    if (!ok || assetId <= 0)
        return;
    category.list->addItem(QString::number(assetId));
}

void LocalPlayerDialog::RemoveSelectedItem(AvatarItemCategory& category) {
    qDeleteAll(category.list->selectedItems());
}

void LocalPlayerDialog::LoadFromRegistry() {
    Registry* reg = gApp->GetCore()->GetRegistry();

    QString type = QString::fromStdString(
        reg->GetKeyValue<std::string>(kAvatarType).value_or("R6"));
    int typeIndex = mAvatarTypeInput->findText(type);
    mAvatarTypeInput->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);

    for (auto it = mBodyParts.begin(); it != mBodyParts.end(); ++it) {
        auto stored = reg->GetKeyValue<std::string>(kColorPrefix + it.key().toStdString());
        if (stored.has_value()) {
            QColor color(QString::fromStdString(*stored));
            if (color.isValid()) {
                it->color = color;
                ApplyBodyColor(*it);
            }
        }
    }

    for (AvatarItemCategory& category : mItemCategories) {
        auto stored = reg->GetKeyValue<std::string>(kItemsPrefix + category.key.toStdString());
        if (!stored.has_value())
            continue;
        const QString csv = QString::fromStdString(*stored);
        if (csv.isEmpty())
            continue;
        for (const QString& id : csv.split(',', Qt::SkipEmptyParts))
            category.list->addItem(id.trimmed());
    }
}

void LocalPlayerDialog::SaveToRegistry() {
    Registry* reg = gApp->GetCore()->GetRegistry();

    reg->SetKeyValue("user.id", mIdInput->text().toLongLong());
    reg->SetKeyValue("user.name", mNameInput->text().toStdString());
    reg->SetKeyValue("user.display_name", mDisplayNameInput->text().toStdString());
    reg->SetKeyValue(kAvatarType, mAvatarTypeInput->currentText().toStdString());

    for (auto it = mBodyParts.begin(); it != mBodyParts.end(); ++it)
        reg->SetKeyValue(kColorPrefix + it.key().toStdString(), it->color.name().toStdString());

    for (const AvatarItemCategory& category : mItemCategories) {
        QStringList ids;
        for (int i = 0; i < category.list->count(); ++i)
            ids << category.list->item(i)->text();
        reg->SetKeyValue(kItemsPrefix + category.key.toStdString(), ids.join(',').toStdString());
    }
}
