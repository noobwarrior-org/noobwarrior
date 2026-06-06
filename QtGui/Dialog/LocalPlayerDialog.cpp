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

#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Roblox/DataType/BrickColor.h>

#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QInputDialog>
#include <QGroupBox>
#include <QLabel>
#include <QDialog>

#include <optional>

using namespace NoobWarrior;

// Resolves a BrickColor name to its preview hex, falling back to grey.
static QColor HexForBrickName(const QString& name) {
    for (const auto& entry : Roblox::BrickColor::Palette) {
        if (name == QLatin1String(entry.name))
            return QColor(entry.hex);
    }
    return QColor("#A3A2A5"); // Medium stone grey
}

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

    mIdInput->setText(QString::number(reg->GetKeyValue<int64_t>("user.id").value_or(1000)));
    mNameInput->setText(QString::fromStdString(reg->GetKeyValue<std::string>("user.name").value_or("Player")));
    mDisplayNameInput->setText(QString::fromStdString(reg->GetKeyValue<std::string>("user.display_name").value_or("Player")));

    mIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mIdInput));
    mFormLayout->addRow("User Id", mIdInput);
    mFormLayout->addRow("Name", mNameInput);
    mFormLayout->addRow("Display Name", mDisplayNameInput);

    // Left column: identity form sitting above the body-part swatches.
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

    // Registry defaults from Registry::Open(): yellow head/arms, blue torso,
    // yellowish-green legs
    AddBodyPart(grid, "head",      "Head",      0, 1, 48, 48, "Bright yellow");
    AddBodyPart(grid, "left_arm",  "Left Arm",  1, 0, 36, 72, "Bright yellow");
    AddBodyPart(grid, "torso",     "Torso",     1, 1, 72, 72, "Bright blue");
    AddBodyPart(grid, "right_arm", "Right Arm", 1, 2, 36, 72, "Bright yellow");

    QHBoxLayout* legs = new QHBoxLayout;
    legs->setSpacing(4);
    AddBodyPart(grid, "left_leg",  "Left Leg",  -1, -1, 36, 72, "Br. yellowish green");
    AddBodyPart(grid, "right_leg", "Right Leg", -1, -1, 36, 72, "Br. yellowish green");
    legs->addWidget(mBodyParts["left_leg"].button);
    legs->addWidget(mBodyParts["right_leg"].button);
    grid->addLayout(legs, 2, 1, Qt::AlignHCenter | Qt::AlignTop);

    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(2, 1);
    return box;
}

void LocalPlayerDialog::AddBodyPart(QGridLayout* grid, const QString& key, const QString& label,
                                    int row, int col, int w, int h, const QString& defaultColorName) {
    AvatarBodyPart part;
    part.key = key;
    part.label = label;
    part.colorName = defaultColorName;
    part.color = HexForBrickName(defaultColorName);
    part.button = new QPushButton;
    part.button->setFixedSize(w, h);
    part.button->setCursor(Qt::PointingHandCursor);
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
    part.button->setToolTip(QString("%1: %2").arg(part.label, part.colorName));
    part.button->setStyleSheet(QString(
        "QPushButton { background-color: %1; border: 1px solid #1b1b1b; border-radius: 3px; }"
        "QPushButton:hover { border: 1px solid #ffffff; }")
        .arg(part.color.name()));
}

void LocalPlayerDialog::PickBodyColor(AvatarBodyPart& part) {
    QDialog dlg(this);
    dlg.setWindowTitle(QString("Pick %1 Color").arg(part.label));
    QGridLayout* grid = new QGridLayout(&dlg);
    grid->setSpacing(2);

    QString result;
    const int columns = 8;
    int i = 0;
    for (const auto& entry : Roblox::BrickColor::Palette) {
        const QString name = QString::fromLatin1(entry.name);
        QPushButton* swatch = new QPushButton;
        swatch->setFixedSize(28, 28);
        swatch->setCursor(Qt::PointingHandCursor);
        swatch->setToolTip(name);
        const bool selected = (name == part.colorName);
        swatch->setStyleSheet(QString(
            "QPushButton { background-color: %1; border: %2; border-radius: 3px; }"
            "QPushButton:hover { border: 2px solid #ffffff; }")
            .arg(entry.hex, selected ? "2px solid #ffffff" : "1px solid #1b1b1b"));
        connect(swatch, &QPushButton::clicked, [&dlg, &result, name]() {
            result = name;
            dlg.accept();
        });
        grid->addWidget(swatch, i / columns, i % columns);
        ++i;
    }

    if (dlg.exec() != QDialog::Accepted || result.isEmpty())
        return;
    part.colorName = result;
    part.color = HexForBrickName(result);
    ApplyBodyColor(part);
}

QLineEdit* LocalPlayerDialog::MakeAssetField(const QString& regKey) {
    QLineEdit* field = new QLineEdit;
    field->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), field));
    field->setPlaceholderText("0 (none)");
    mAssetFields.insert(regKey, field);
    return field;
}

QDoubleSpinBox* LocalPlayerDialog::MakeScaleField(const QString& regKey) {
    QDoubleSpinBox* field = new QDoubleSpinBox;
    field->setRange(0.0, 2.0);
    field->setSingleStep(0.05);
    field->setDecimals(2);
    mScaleFields.insert(regKey, field);
    return field;
}

QWidget* LocalPlayerDialog::BuildItemEditor() {
    QTabWidget* tabs = new QTabWidget;
    tabs->setMinimumWidth(280);

    // Clothing: single-asset fields
    {
        QWidget* page = new QWidget;
        QFormLayout* form = new QFormLayout(page);
        form->addRow("Shirt",   MakeAssetField("user.appearance.shirt"));
        form->addRow("Pants",   MakeAssetField("user.appearance.pants"));
        form->addRow("T-Shirt", MakeAssetField("user.appearance.tshirt"));
        form->addRow("Face",    MakeAssetField("user.appearance.face"));
        tabs->addTab(page, "Clothing");
    }

    // Accessories: list backed by the user.appearance.accessories table
    {
        QWidget* page = new QWidget;
        QVBoxLayout* pageLayout = new QVBoxLayout(page);

        QListWidget* list = new QListWidget;
        list->setSelectionMode(QAbstractItemView::ExtendedSelection);
        pageLayout->addWidget(new QLabel("Equipped accessories (asset ids)"));
        pageLayout->addWidget(list);

        QHBoxLayout* buttons = new QHBoxLayout;
        QPushButton* addButton = new QPushButton("Add");
        QPushButton* removeButton = new QPushButton("Remove");
        buttons->addWidget(addButton);
        buttons->addWidget(removeButton);
        buttons->addStretch();
        pageLayout->addLayout(buttons);

        AvatarItemCategory category{"user.appearance.accessories", "Accessories", list};
        mItemCategories.append(category);
        int index = mItemCategories.size() - 1;
        connect(addButton, &QPushButton::clicked, [this, index]() {
            AddItemToCategory(mItemCategories[index]);
        });
        connect(removeButton, &QPushButton::clicked, [this, index]() {
            RemoveSelectedItem(mItemCategories[index]);
        });

        tabs->addTab(page, "Accessories");
    }

    // Body: per-part mesh/package asset overrides
    {
        QWidget* page = new QWidget;
        QFormLayout* form = new QFormLayout(page);
        form->addRow("Head",      MakeAssetField("user.appearance.body.head"));
        form->addRow("Torso",     MakeAssetField("user.appearance.body.torso"));
        form->addRow("Left Arm",  MakeAssetField("user.appearance.body.left_arm"));
        form->addRow("Right Arm", MakeAssetField("user.appearance.body.right_arm"));
        form->addRow("Left Leg",  MakeAssetField("user.appearance.body.left_leg"));
        form->addRow("Right Leg", MakeAssetField("user.appearance.body.right_leg"));
        tabs->addTab(page, "Body");
    }

    // Animation: per-state animation asset ids
    {
        QWidget* page = new QWidget;
        QFormLayout* form = new QFormLayout(page);
        form->addRow("Climb", MakeAssetField("user.appearance.animation.climb"));
        form->addRow("Fall",  MakeAssetField("user.appearance.animation.fall"));
        form->addRow("Idle",  MakeAssetField("user.appearance.animation.idle"));
        form->addRow("Jump",  MakeAssetField("user.appearance.animation.jump"));
        form->addRow("Run",   MakeAssetField("user.appearance.animation.run"));
        form->addRow("Swim",  MakeAssetField("user.appearance.animation.swim"));
        form->addRow("Walk",  MakeAssetField("user.appearance.animation.walk"));
        tabs->addTab(page, "Animation");
    }

    // Scale: R15 body proportion sliders
    {
        QWidget* page = new QWidget;
        QFormLayout* form = new QFormLayout(page);
        form->addRow("Height",     MakeScaleField("user.appearance.scale.height"));
        form->addRow("Width",      MakeScaleField("user.appearance.scale.width"));
        form->addRow("Head",       MakeScaleField("user.appearance.scale.head"));
        form->addRow("Depth",      MakeScaleField("user.appearance.scale.depth"));
        form->addRow("Proportion", MakeScaleField("user.appearance.scale.proportion"));
        form->addRow("Body Type",  MakeScaleField("user.appearance.scale.body_type"));
        tabs->addTab(page, "Scale");
    }

    return tabs;
}

void LocalPlayerDialog::AddItemToCategory(AvatarItemCategory& category) {
    bool ok = false;
    // Asset ids can exceed 32 bits, so prompt for text and validate as int64.
    QString text = QInputDialog::getText(this,
        QString("Add %1").arg(category.label),
        "Asset Id:", QLineEdit::Normal, QString(), &ok);
    if (!ok)
        return;
    bool valid = false;
    qlonglong assetId = text.trimmed().toLongLong(&valid);
    if (!valid || assetId <= 0)
        return;
    category.list->addItem(QString::number(assetId));
}

void LocalPlayerDialog::RemoveSelectedItem(AvatarItemCategory& category) {
    qDeleteAll(category.list->selectedItems());
}

void LocalPlayerDialog::LoadFromRegistry() {
    Registry* reg = gApp->GetCore()->GetRegistry();
    
    for (auto it = mBodyParts.begin(); it != mBodyParts.end(); ++it) {
        auto name = reg->GetKeyValue<std::string>("user.appearance.color." + it.key().toStdString());
        if (!name.has_value())
            continue;
        it->colorName = QString::fromStdString(*name);
        it->color = HexForBrickName(it->colorName);
        ApplyBodyColor(*it);
    }

    for (auto it = mAssetFields.begin(); it != mAssetFields.end(); ++it) {
        int64_t id = reg->GetKeyValue<int64_t>(it.key().toStdString()).value_or(0);
        it.value()->setText(id != 0 ? QString::number(id) : QString());
    }

    // Scales.
    for (auto it = mScaleFields.begin(); it != mScaleFields.end(); ++it)
        it.value()->setValue(reg->GetKeyValue<double>(it.key().toStdString()).value_or(0.0));

    // Accessories table.
    for (AvatarItemCategory& category : mItemCategories) {
        auto table = reg->GetKeyValue<sol::table>(category.key.toStdString());
        if (!table.has_value())
            continue;
        // Walk the array part in order; skip any non-numeric entries defensively.
        const std::size_t count = table->size();
        for (std::size_t i = 1; i <= count; ++i) {
            sol::object obj = (*table)[i];
            if (obj.get_type() != sol::type::number)
                continue;
            category.list->addItem(QString::number(obj.as<int64_t>()));
        }
    }
}

void LocalPlayerDialog::SaveToRegistry() {
    Core* core = gApp->GetCore();
    Registry* reg = core->GetRegistry();

    reg->SetKeyValue("user.id", mIdInput->text().toLongLong());
    reg->SetKeyValue("user.name", mNameInput->text().toStdString());
    reg->SetKeyValue("user.display_name", mDisplayNameInput->text().toStdString());

    // Body colors (BrickColor names)
    for (auto it = mBodyParts.begin(); it != mBodyParts.end(); ++it)
        reg->SetKeyValue("user.appearance.color." + it.key().toStdString(), it->colorName.toStdString());

    // Single-asset fields
    for (auto it = mAssetFields.begin(); it != mAssetFields.end(); ++it)
        reg->SetKeyValue<int64_t>(it.key().toStdString(), it.value()->text().toLongLong());

    // Scales.
    for (auto it = mScaleFields.begin(); it != mScaleFields.end(); ++it)
        reg->SetKeyValue<double>(it.key().toStdString(), it.value()->value());

    // Accessories table.
    for (const AvatarItemCategory& category : mItemCategories) {
        sol::table table = core->GetLuaState()->create_table();
        for (int i = 0; i < category.list->count(); ++i)
            table[i + 1] = category.list->item(i)->text().toLongLong();
        reg->SetKeyValue(category.key.toStdString(), table);
    }
}
