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

#include "Sdk/Item/ItemListWidget.h"
#include "Sdk/Item/ItemWidget.h"

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Roblox/DataType/BrickColor.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QStyledItemDelegate>
#include <QInputDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QStackedWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPainter>
#include <QPalette>
#include <QGroupBox>
#include <QLabel>
#include <QDialog>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>

using namespace NoobWarrior;
using AT = Roblox::AssetType;
using Kind = AvatarSubgroup::Kind;

// Catalog items shown per page in each subgroup's list.
static constexpr int kPageSize = 60;

namespace {
// Draws a selected picker item as a full-bleed inverted box (light fill) with contrasting dark text,
// so the worn item is obvious. Scoped to the avatar picker lists (not the shared SDK item list).
class WornItemDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
protected:
    void initStyleOption(QStyleOptionViewItem* opt, const QModelIndex& index) const override {
        QStyledItemDelegate::initStyleOption(opt, index);
        if (opt->state & QStyle::State_Selected) {
            opt->palette.setColor(QPalette::Highlight, QColor(0xF2, 0xF3, 0xF3));
            opt->palette.setColor(QPalette::HighlightedText, QColor(0x11, 0x11, 0x11));
        }
    }
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, QColor(0xF2, 0xF3, 0xF3));
        QStyledItemDelegate::paint(painter, option, index);
    }
};
}

// Resolves a BrickColor name to its preview hex, falling back to grey.
static QColor HexForBrickName(const QString& name) {
    for (const auto& entry : Roblox::BrickColor::Palette) {
        if (name == QLatin1String(entry.name))
            return QColor(entry.hex);
    }
    return QColor("#A3A2A5"); // Medium stone grey
}

// Closest palette color name to an RGB triple (used when importing a stored body color).
static QString BrickNameForRgb(int r, int g, int b) {
    QString best = "Medium stone grey";
    long bestDist = -1;
    for (const auto& entry : Roblox::BrickColor::Palette) {
        QColor c(entry.hex);
        long dr = r - c.red(), dg = g - c.green(), db = b - c.blue();
        long dist = dr * dr + dg * dg + db * db;
        if (bestDist < 0 || dist < bestDist) {
            bestDist = dist;
            best = QLatin1String(entry.name);
        }
    }
    return best;
}

// Maps a UserCharacterBodyPart value to this dialog's body-part color key.
static QString ColorKeyForBodyPart(int part) {
    switch (static_cast<UserCharacterBodyPart>(part)) {
    case UserCharacterBodyPart::Head:     return "head";
    case UserCharacterBodyPart::Torso:    return "torso";
    case UserCharacterBodyPart::RightArm: return "right_arm";
    case UserCharacterBodyPart::LeftArm:  return "left_arm";
    case UserCharacterBodyPart::RightLeg: return "right_leg";
    case UserCharacterBodyPart::LeftLeg:  return "left_leg";
    }
    return {};
}

// Escapes a string for use inside a SQL LIKE pattern (matching ItemListWidget's escaping).
static std::string EscapeLike(const std::string& input) {
    std::string result;
    for (char c : input) {
        if (c == '%' || c == '_' || c == '\\')
            result += '\\';
        result += c;
    }
    return result;
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

    QPushButton* importButton = new QPushButton(QIcon(":/images/roblox_backup.png"), "Import Avatar from Database…");
    connect(importButton, &QPushButton::clicked, this, &LocalPlayerDialog::ImportAvatarFromDatabase);

    QVBoxLayout* leftColumn = new QVBoxLayout;
    leftColumn->addLayout(mFormLayout);
    leftColumn->addWidget(importButton);
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

QDoubleSpinBox* LocalPlayerDialog::MakeScaleField(const QString& regKey) {
    QDoubleSpinBox* field = new QDoubleSpinBox;
    field->setRange(0.0, 2.0);
    field->setSingleStep(0.05);
    field->setDecimals(2);
    mScaleFields.insert(regKey, field);
    return field;
}

QWidget* LocalPlayerDialog::BuildScaleWidget() {
    QWidget* page = new QWidget;
    QFormLayout* form = new QFormLayout(page);

    // Rig type: R6 (6 limbs) vs R15 (15-part rig). Drives resolvedAvatarType in the served avatar.
    mAvatarTypeR6 = new QRadioButton("R6");
    mAvatarTypeR15 = new QRadioButton("R15");
    mAvatarTypeR6->setChecked(true);
    QButtonGroup* rigGroup = new QButtonGroup(page);
    rigGroup->addButton(mAvatarTypeR6);
    rigGroup->addButton(mAvatarTypeR15);
    QHBoxLayout* rigRow = new QHBoxLayout;
    rigRow->addWidget(mAvatarTypeR6);
    rigRow->addWidget(mAvatarTypeR15);
    rigRow->addStretch();
    form->addRow("Rig", rigRow);

    form->addRow("Height",     MakeScaleField("user.appearance.scale.height"));
    form->addRow("Width",      MakeScaleField("user.appearance.scale.width"));
    form->addRow("Head",       MakeScaleField("user.appearance.scale.head"));
    form->addRow("Depth",      MakeScaleField("user.appearance.scale.depth"));
    form->addRow("Proportion", MakeScaleField("user.appearance.scale.proportion"));
    form->addRow("Body Type",  MakeScaleField("user.appearance.scale.body_type"));
    return page;
}

QWidget* LocalPlayerDialog::BuildItemEditor() {
    QTabWidget* tabs = new QTabWidget;
    tabs->setMinimumWidth(400);

    auto slot = [](const char* n, AT t, const char* key) {
        return AvatarSubgroup{ n, t, Kind::Slot, key };
    };
    auto acc = [](const char* n, AT t) {
        return AvatarSubgroup{ n, t, Kind::Accessory, QString() };
    };

    {
        AvatarTab def;
        def.name = "Clothing";
        def.subgroups = {
            slot("Shirts",   AT::Shirt,  "user.appearance.shirt"),
            slot("Pants",    AT::Pants,  "user.appearance.pants"),
            slot("T-Shirts", AT::TShirt, "user.appearance.tshirt"),
        };
        BuildTab(tabs, def);
    }
    {
        AvatarTab def;
        def.name = "Accessories";
        def.subgroups = {
            acc("Head",      AT::Hat),
            acc("Face",      AT::FaceAccessory),
            acc("Neck",      AT::NeckAccessory),
            acc("Shoulders", AT::ShoulderAccessory),
            acc("Front",     AT::FrontAccessory),
            acc("Back",      AT::BackAccessory),
            acc("Waist",     AT::WaistAccessory),
            acc("Gear",      AT::Gear),
        };
        BuildTab(tabs, def);
    }
    {
        AvatarTab def;
        def.name = "Body";
        def.subgroups = {
            acc("Hair",       AT::HairAccessory),
            slot("Faces",     AT::Face,     "user.appearance.face"),
            slot("Torso",     AT::Torso,    "user.appearance.body.torso"),
            slot("Left Arms", AT::LeftArm,  "user.appearance.body.left_arm"),
            slot("Right Arms",AT::RightArm, "user.appearance.body.right_arm"),
            slot("Left Legs", AT::LeftLeg,  "user.appearance.body.left_leg"),
            slot("Right Legs",AT::RightLeg, "user.appearance.body.right_leg"),
            AvatarSubgroup{ "Scale", AT::None, Kind::Scale, QString() },
        };
        BuildTab(tabs, def);
    }
    {
        AvatarTab def;
        def.name = "Animations";
        def.subgroups = {
            acc("Emotes", AT::EmoteAnimation),
            slot("Walk",  AT::WalkAnimation,  "user.appearance.animation.walk"),
            slot("Run",   AT::RunAnimation,   "user.appearance.animation.run"),
            slot("Fall",  AT::FallAnimation,  "user.appearance.animation.fall"),
            slot("Jump",  AT::JumpAnimation,  "user.appearance.animation.jump"),
            slot("Swim",  AT::SwimAnimation,  "user.appearance.animation.swim"),
            slot("Climb", AT::ClimbAnimation, "user.appearance.animation.climb"),
            slot("Idle",  AT::IdleAnimation,  "user.appearance.animation.idle"),
        };
        BuildTab(tabs, def);
    }

    tabs->addTab(BuildOutfitsTab(), "Outfits");
    return tabs;
}

void LocalPlayerDialog::BuildTab(QTabWidget* tabs, AvatarTab def) {
    mTabs.append(def);
    const int idx = mTabs.size() - 1;
    AvatarTab& tab = mTabs[idx];

    // Register each subgroup's equip target so RouteWornAsset/import/outfits know where ids go.
    for (const AvatarSubgroup& sg : tab.subgroups) {
        if (sg.kind == Kind::Slot)
            mTypeToSlotKey[(int)sg.type] = sg.regKey;
        else if (sg.kind == Kind::Accessory)
            mAccessoryTypes.insert((int)sg.type);
    }

    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    tab.subgroupCombo = new QComboBox;
    for (const AvatarSubgroup& sg : tab.subgroups)
        tab.subgroupCombo->addItem(sg.name);
    QHBoxLayout* catRow = new QHBoxLayout;
    catRow->addWidget(new QLabel("Category:"));
    catRow->addWidget(tab.subgroupCombo, 1);
    layout->addLayout(catRow);

    tab.search = new QLineEdit;
    tab.search->setPlaceholderText("Search…");
    tab.search->setClearButtonEnabled(true);
    layout->addWidget(tab.search);

    tab.stack = new QStackedWidget;

    // Page 0: the catalog list + pagination controls.
    QWidget* listPage = new QWidget;
    QVBoxLayout* lpl = new QVBoxLayout(listPage);
    lpl->setContentsMargins(0, 0, 0, 0);

    // Catalog: click an item to wear it (no persistent selection, worn items show in the worn list).
    tab.list = new ItemListWidget(listPage);
    tab.list->setSelectionMode(QAbstractItemView::NoSelection);
    tab.list->setContextMenuPolicy(Qt::NoContextMenu);
    tab.list->SetOnDoubleClick([](ItemWidget*) {});
    lpl->addWidget(tab.list, 1);

    QHBoxLayout* pager = new QHBoxLayout;
    tab.prevBtn = new QPushButton("◄ Prev");
    tab.nextBtn = new QPushButton("Next ►");
    tab.pageLabel = new QLabel("Page 1 / 1");
    pager->addWidget(tab.prevBtn);
    pager->addWidget(tab.pageLabel, 1, Qt::AlignHCenter);
    pager->addWidget(tab.nextBtn);
    lpl->addLayout(pager);

    // Currently worn: a separate strip below the catalog. Click an item here to remove it.
    lpl->addWidget(new QLabel("Currently worn (click to remove):"));
    tab.wornList = new ItemListWidget(listPage);
    tab.wornList->setSelectionMode(QAbstractItemView::NoSelection);
    tab.wornList->setContextMenuPolicy(Qt::NoContextMenu);
    tab.wornList->SetOnDoubleClick([](ItemWidget*) {});
    tab.wornList->setMaximumHeight(140);
    lpl->addWidget(tab.wornList);

    tab.stack->addWidget(listPage); // index 0

    bool hasScale = false;
    for (const AvatarSubgroup& sg : tab.subgroups)
        if (sg.kind == Kind::Scale) hasScale = true;
    if (hasScale)
        tab.stack->addWidget(BuildScaleWidget()); // index 1

    layout->addWidget(tab.stack, 1);

    connect(tab.subgroupCombo, &QComboBox::currentIndexChanged, this, [this, idx](int) {
        OnSubgroupChanged(mTabs[idx]);
    });
    connect(tab.search, &QLineEdit::textChanged, this, [this, idx](const QString&) {
        CollectIds(mTabs[idx]);
        mTabs[idx].page = 0;
        RenderPage(mTabs[idx]);
    });
    connect(tab.prevBtn, &QPushButton::clicked, this, [this, idx]() { StepPage(mTabs[idx], -1); });
    connect(tab.nextBtn, &QPushButton::clicked, this, [this, idx]() { StepPage(mTabs[idx], +1); });
    connect(tab.list, &QListWidget::itemClicked, this, [this, idx](QListWidgetItem* it) {
        if (auto* iw = dynamic_cast<ItemWidget*>(it))
            WearItem(mTabs[idx], (qint64)iw->GetId());
    });
    connect(tab.wornList, &QListWidget::itemClicked, this, [this, idx](QListWidgetItem* it) {
        if (auto* iw = dynamic_cast<ItemWidget*>(it))
            UnwearItem(mTabs[idx], (qint64)iw->GetId());
    });

    tabs->addTab(page, tab.name);
}

const AvatarSubgroup& LocalPlayerDialog::ActiveSubgroup(const AvatarTab& tab) const {
    int i = tab.subgroupCombo ? tab.subgroupCombo->currentIndex() : 0;
    if (i < 0 || i >= (int)tab.subgroups.size())
        i = 0;
    return tab.subgroups[i];
}

void LocalPlayerDialog::OnSubgroupChanged(AvatarTab& tab) {
    const AvatarSubgroup& sg = ActiveSubgroup(tab);
    if (sg.kind == Kind::Scale) {
        tab.search->setEnabled(false);
        tab.stack->setCurrentIndex(1);
        return;
    }
    tab.search->setEnabled(true);
    tab.stack->setCurrentIndex(0);
    CollectIds(tab);
    tab.page = 0;
    RenderPage(tab);
    RenderWorn(tab);
}

void LocalPlayerDialog::CollectIds(AvatarTab& tab) {
    tab.pageIds.clear();
    const AvatarSubgroup& sg = ActiveSubgroup(tab);
    if (sg.kind == Kind::Scale)
        return;

    EmuDbManager* mgr = gApp->GetCore()->GetEmuDbManager();
    const QString query = tab.search->text().trimmed();
    QSet<qint64> seen;

    for (EmuDb* db : mgr->GetMountedDatabases()) {
        std::string s = "SELECT Id FROM Asset WHERE Type = ?";
        if (!query.isEmpty())
            s += " AND Name LIKE ? ESCAPE '\\'";
        s += " ORDER BY Name;";
        Statement stmt = db->PrepareStatement(s);
        stmt.Bind(1, (int)sg.type);
        if (!query.isEmpty())
            stmt.Bind(2, "%" + EscapeLike(query.toStdString()) + "%");
        while (stmt.Step() == SQLITE_ROW) {
            qint64 id = stmt.GetInt64FromColumnIndex(0);
            if (!seen.contains(id)) {
                seen.insert(id);
                tab.pageIds.push_back({ id, db });
            }
        }
    }

    // Keep a worn slot's item visible even if no mounted database lists it anymore.
    if (query.isEmpty() && sg.kind == Kind::Slot) {
        qint64 worn = mWornSlots.value(sg.regKey, 0);
        if (worn > 0 && !seen.contains(worn)) {
            EmuDb* owner = mgr->GetFirstDbWhereItemExists(ItemType::Asset, worn);
            if (owner == nullptr) {
                std::vector<EmuDb*> dbs = mgr->GetMountedDatabases();
                owner = dbs.empty() ? nullptr : dbs.front();
            }
            if (owner != nullptr) {
                seen.insert(worn);
                tab.pageIds.push_back({ worn, owner });
            }
        }
    }

    tab.pageCount = std::max<int>(1, ((int)tab.pageIds.size() + kPageSize - 1) / kPageSize);
}

void LocalPlayerDialog::RenderPage(AvatarTab& tab) {
    const AvatarSubgroup& sg = ActiveSubgroup(tab);
    if (sg.kind == Kind::Scale)
        return;
    if (tab.page < 0) tab.page = 0;
    if (tab.page >= tab.pageCount) tab.page = tab.pageCount - 1;

    tab.list->Clear();
    const int start = tab.page * kPageSize;
    const int end = std::min<int>((int)tab.pageIds.size(), start + kPageSize);
    for (int i = start; i < end; ++i)
        tab.list->AddFromDatabase(tab.pageIds[i].second, ItemType::Asset, tab.pageIds[i].first);

    tab.pageLabel->setText(QString("Page %1 / %2").arg(tab.page + 1).arg(tab.pageCount));
    tab.prevBtn->setEnabled(tab.page > 0);
    tab.nextBtn->setEnabled(tab.page < tab.pageCount - 1);
}

void LocalPlayerDialog::StepPage(AvatarTab& tab, int delta) {
    const int np = tab.page + delta;
    if (np < 0 || np >= tab.pageCount)
        return;
    tab.page = np;
    RenderPage(tab);
}

void LocalPlayerDialog::RenderWorn(AvatarTab& tab) {
    if (tab.wornList == nullptr)
        return;
    tab.wornList->Clear();
    const AvatarSubgroup& sg = ActiveSubgroup(tab);
    if (sg.kind == Kind::Scale)
        return;

    EmuDbManager* mgr = gApp->GetCore()->GetEmuDbManager();
    auto add = [&](qint64 id) {
        if (id <= 0)
            return;
        EmuDb* db = mgr->GetFirstDbWhereItemExists(ItemType::Asset, id);
        if (db == nullptr) {
            std::vector<EmuDb*> dbs = mgr->GetMountedDatabases();
            db = dbs.empty() ? nullptr : dbs.front();
        }
        tab.wornList->AddFromDatabase(db, ItemType::Asset, id);
    };

    if (sg.kind == Kind::Slot) {
        add(mWornSlots.value(sg.regKey, 0));
    } else {
        for (qint64 id : mWornAccessories)
            if (mWornAccType.value(id, -1) == (int)sg.type)
                add(id);
    }
}

void LocalPlayerDialog::WearItem(AvatarTab& tab, qint64 id) {
    if (id <= 0)
        return;
    const AvatarSubgroup& sg = ActiveSubgroup(tab);
    if (sg.kind == Kind::Slot) {
        mWornSlots[sg.regKey] = id;
    } else if (sg.kind == Kind::Accessory) {
        mWornAccessories.insert(id);
        mWornAccType[id] = (int)sg.type;
    } else {
        return;
    }
    RenderWorn(tab);
}

void LocalPlayerDialog::UnwearItem(AvatarTab& tab, qint64 id) {
    const AvatarSubgroup& sg = ActiveSubgroup(tab);
    if (sg.kind == Kind::Slot) {
        if (mWornSlots.value(sg.regKey, 0) == id)
            mWornSlots[sg.regKey] = 0;
    } else if (sg.kind == Kind::Accessory) {
        mWornAccessories.remove(id);
        mWornAccType.remove(id);
    }
    RenderWorn(tab);
}

void LocalPlayerDialog::RefreshAllTabs() {
    for (AvatarTab& tab : mTabs) {
        if (ActiveSubgroup(tab).kind == Kind::Scale)
            continue;
        CollectIds(tab);
        RenderPage(tab);
        RenderWorn(tab);
    }
}

void LocalPlayerDialog::RouteWornAsset(qint64 id) {
    if (id <= 0)
        return;
    int type = 0;
    if (auto summary = gApp->GetCore()->GetEmuDbManager()->GetAssetSummary(id); summary.has_value())
        type = summary->Type;
    if (mTypeToSlotKey.contains(type)) {
        mWornSlots[mTypeToSlotKey.value(type)] = id;
    } else {
        mWornAccessories.insert(id); // accessory type or unknown -> the flat list
        mWornAccType[id] = type;
    }
}

void LocalPlayerDialog::ImportAvatarFromDatabase() {
    EmuDbManager* mgr = gApp->GetCore()->GetEmuDbManager();
    if (mgr->GetMountedDatabases().empty()) {
        QMessageBox::information(this, "Import Avatar", "No databases are mounted to import from.");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Import Avatar from Database");
    dlg.resize(480, 540);
    QVBoxLayout* layout = new QVBoxLayout(&dlg);
    layout->addWidget(new QLabel("Pick a user to copy their avatar from.\nUsers from every mounted database are shown together."));

    QLineEdit* search = new QLineEdit(&dlg);
    search->setPlaceholderText("Search users…");
    search->setClearButtonEnabled(true);
    layout->addWidget(search);

    ItemListWidget* userList = new ItemListWidget(&dlg);
    userList->setSelectionMode(QAbstractItemView::SingleSelection);
    userList->setItemDelegate(new WornItemDelegate(userList));
    userList->setContextMenuPolicy(Qt::NoContextMenu);
    userList->SetOnDoubleClick([](ItemWidget*) {});
    layout->addWidget(userList, 1);

    auto userDbs = std::make_shared<QMap<qint64, EmuDb*>>();
    auto populate = [mgr, userList, search, userDbs]() {
        userList->Clear();
        userDbs->clear();
        const QString query = search->text().trimmed();
        for (EmuDb* db : mgr->GetMountedDatabases()) {
            std::string sql = "SELECT Id FROM User";
            if (!query.isEmpty())
                sql += " WHERE Name LIKE ? ESCAPE '\\'";
            sql += " ORDER BY Name;";
            Statement stmt = db->PrepareStatement(sql);
            if (!query.isEmpty())
                stmt.Bind(1, "%" + EscapeLike(query.toStdString()) + "%");
            while (stmt.Step() == SQLITE_ROW) {
                int64_t id = stmt.GetInt64FromColumnIndex(0);
                if (userList->AddFromDatabase(db, ItemType::User, id))
                    (*userDbs)[(qint64)id] = db;
            }
        }
    };
    populate();
    connect(search, &QLineEdit::textChanged, &dlg, [populate](const QString&) { populate(); });

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(userList, &QListWidget::itemDoubleClicked, &dlg, &QDialog::accept);

    if (dlg.exec() != QDialog::Accepted)
        return;

    auto* sel = userList->selectedItems().isEmpty()
        ? nullptr : dynamic_cast<ItemWidget*>(userList->selectedItems().first());
    if (sel == nullptr) {
        QMessageBox::information(this, "Import Avatar", "No user was selected.");
        return;
    }
    const qint64 userId = (qint64)sel->GetId();
    EmuDb* db = userDbs->value(userId, nullptr);
    if (db != nullptr)
        ApplyImportedAvatar(db, userId);
}

void LocalPlayerDialog::ApplyImportedAvatar(EmuDb* db, int64_t userId) {
    // Identity.
    {
        Statement stmt = db->PrepareStatement("SELECT Name, DisplayName FROM User WHERE Id = ?;");
        stmt.Bind(1, userId);
        if (stmt.Step() == SQLITE_ROW) {
            QString name = QString::fromStdString(stmt.GetStringFromColumnIndex(0));
            QString display = QString::fromStdString(stmt.GetStringFromColumnIndex(1));
            mIdInput->setText(QString::number(userId));
            if (!name.isEmpty())
                mNameInput->setText(name);
            mDisplayNameInput->setText(display.isEmpty() ? name : display);
        }
    }

    // Worn items: route each by its asset type.
    mWornSlots.clear();
    mWornAccessories.clear();
    mWornAccType.clear();
    {
        Statement stmt = db->PrepareStatement("SELECT AssetId FROM UserCharacterItem WHERE Id = ?;");
        stmt.Bind(1, userId);
        while (stmt.Step() == SQLITE_ROW)
            RouteWornAsset(stmt.GetInt64FromColumnIndex(0));
    }

    // Body colors.
    {
        Statement stmt = db->PrepareStatement("SELECT BodyPart, Color3 FROM UserCharacterBodyColor WHERE Id = ?;");
        stmt.Bind(1, userId);
        while (stmt.Step() == SQLITE_ROW) {
            int part = stmt.GetIntFromColumnIndex(0);
            int color = stmt.GetIntFromColumnIndex(1);
            QString key = ColorKeyForBodyPart(part);
            if (key.isEmpty() || !mBodyParts.contains(key))
                continue;
            QString name = BrickNameForRgb((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
            AvatarBodyPart& bp = mBodyParts[key];
            bp.colorName = name;
            bp.color = HexForBrickName(name);
            ApplyBodyColor(bp);
        }
    }

    // Scales.
    {
        Statement stmt = db->PrepareStatement(
            "SELECT CharacterWidth, CharacterHeight, CharacterHead, CharacterProportions FROM User WHERE Id = ?;");
        stmt.Bind(1, userId);
        if (stmt.Step() == SQLITE_ROW) {
            auto set = [&](const QString& key, double v) {
                if (v > 0.0 && mScaleFields.contains(key))
                    mScaleFields[key]->setValue(v);
            };
            set("user.appearance.scale.width",      stmt.GetDoubleFromColumnIndex(0));
            set("user.appearance.scale.height",     stmt.GetDoubleFromColumnIndex(1));
            set("user.appearance.scale.head",       stmt.GetDoubleFromColumnIndex(2));
            set("user.appearance.scale.proportion", stmt.GetDoubleFromColumnIndex(3));
        }
    }

    // Rig type (CharacterBodyType: 0 = R6, 1 = R15).
    {
        Statement stmt = db->PrepareStatement("SELECT CharacterBodyType FROM User WHERE Id = ?;");
        stmt.Bind(1, userId);
        if (stmt.Step() == SQLITE_ROW)
            (stmt.GetIntFromColumnIndex(0) == 1 ? mAvatarTypeR15 : mAvatarTypeR6)->setChecked(true);
    }

    RefreshAllTabs();
}

QWidget* LocalPlayerDialog::BuildOutfitsTab() {
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("Saved outfits. Wear one to load it onto your avatar, or save the\ncurrent avatar as a new outfit."));

    mOutfitList = new ItemListWidget(page);
    mOutfitList->setSelectionMode(QAbstractItemView::SingleSelection);
    mOutfitList->setItemDelegate(new WornItemDelegate(mOutfitList));
    mOutfitList->setContextMenuPolicy(Qt::NoContextMenu);
    mOutfitList->SetOnDoubleClick([](ItemWidget*) {});
    layout->addWidget(mOutfitList, 1);

    QHBoxLayout* btns = new QHBoxLayout;
    QPushButton* saveBtn = new QPushButton("Save Current as Outfit…");
    QPushButton* wearBtn = new QPushButton("Wear Selected");
    QPushButton* delBtn  = new QPushButton("Delete Selected");
    btns->addWidget(saveBtn);
    btns->addWidget(wearBtn);
    btns->addWidget(delBtn);
    layout->addLayout(btns);

    connect(saveBtn, &QPushButton::clicked, this, &LocalPlayerDialog::SaveCurrentOutfit);
    connect(wearBtn, &QPushButton::clicked, this, &LocalPlayerDialog::WearSelectedOutfit);
    connect(delBtn,  &QPushButton::clicked, this, &LocalPlayerDialog::DeleteSelectedOutfit);
    connect(mOutfitList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) { WearSelectedOutfit(); });

    return page;
}

void LocalPlayerDialog::RefreshOutfits() {
    if (mOutfitList == nullptr)
        return;
    mOutfitList->Clear();
    mOutfitDbs.clear();
    EmuDbManager* mgr = gApp->GetCore()->GetEmuDbManager();
    // Outfit ids are per-database (not global), so the same id can appear in several databases; the
    // shared list keys by id, so the first database that has a given id wins (its db is remembered for
    // wear/delete). The thumbnail comes from RetrieveImageData, which has no Outfit image and so falls
    // back to the "content deleted" placeholder.
    for (EmuDb* db : mgr->GetMountedDatabases()) {
        Statement stmt = db->PrepareStatement("SELECT Id FROM Outfit ORDER BY Name;");
        while (stmt.Step() == SQLITE_ROW) {
            qint64 id = stmt.GetInt64FromColumnIndex(0);
            if (mOutfitList->AddFromDatabase(db, ItemType::Outfit, id))
                mOutfitDbs[id] = db;
        }
    }
}

// The (db, outfit id) of the selected outfit list item, or {nullptr, 0}.
static std::pair<EmuDb*, qint64> SelectedOutfit(ItemListWidget* list, const QMap<qint64, EmuDb*>& dbs) {
    if (list == nullptr || list->selectedItems().isEmpty())
        return { nullptr, 0 };
    auto* iw = dynamic_cast<ItemWidget*>(list->selectedItems().first());
    if (iw == nullptr)
        return { nullptr, 0 };
    qint64 id = (qint64)iw->GetId();
    return { dbs.value(id, nullptr), id };
}

void LocalPlayerDialog::SaveCurrentOutfit() {
    EmuDb* db = gApp->GetCore()->GetEmuDbManager()->GetMasterDatabase();
    if (db == nullptr) {
        QMessageBox::warning(this, "Save Outfit", "There's no master database to save the outfit to.");
        return;
    }

    bool ok = false;
    QString name = QInputDialog::getText(this, "Save Outfit", "Outfit name:", QLineEdit::Normal, "My Outfit", &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    name = name.trimmed();

    int64_t newId = 1;
    {
        Statement stmt = db->PrepareStatement("SELECT COALESCE(MAX(Id), 0) + 1 FROM Outfit;");
        if (stmt.Step() == SQLITE_ROW)
            newId = stmt.GetInt64FromColumnIndex(0);
    }

    auto scaleVal = [&](const QString& key) -> double {
        auto* f = mScaleFields.value(key, nullptr);
        return f ? f->value() : 0.0;
    };

    SqlRow row;
    row.push_back({ "Id", newId });
    row.push_back({ "Name", name.toStdString() });
    row.push_back({ "BodyType", (int)(mAvatarTypeR15 && mAvatarTypeR15->isChecked() ? 1 : 0) });
    row.push_back({ "Width", scaleVal("user.appearance.scale.width") });
    row.push_back({ "Height", scaleVal("user.appearance.scale.height") });
    row.push_back({ "Head", scaleVal("user.appearance.scale.head") });
    row.push_back({ "Proportions", scaleVal("user.appearance.scale.proportion") });
    if (db->AddItem(ItemType::Outfit, row) != SqlDb::Response::Success) {
        QMessageBox::warning(this, "Save Outfit", "Failed to create the outfit row.");
        return;
    }

    for (auto it = mWornSlots.begin(); it != mWornSlots.end(); ++it)
        if (it.value() > 0)
            db->AddAssetToOutfit(newId, it.value());
    for (qint64 id : mWornAccessories)
        db->AddAssetToOutfit(newId, id);

    struct ColorPart { const char* key; int part; };
    const ColorPart parts[] = {
        { "head", (int)UserCharacterBodyPart::Head },   { "torso", (int)UserCharacterBodyPart::Torso },
        { "right_arm", (int)UserCharacterBodyPart::RightArm }, { "left_arm", (int)UserCharacterBodyPart::LeftArm },
        { "right_leg", (int)UserCharacterBodyPart::RightLeg }, { "left_leg", (int)UserCharacterBodyPart::LeftLeg },
    };
    for (const ColorPart& cp : parts) {
        if (!mBodyParts.contains(cp.key))
            continue;
        const QColor c = mBodyParts[cp.key].color;
        int packed = ((c.red() & 0xFF) << 16) | ((c.green() & 0xFF) << 8) | (c.blue() & 0xFF);
        Statement stmt = db->PrepareStatement("INSERT OR REPLACE INTO OutfitBodyColor (Id, BodyPart, Color3) VALUES (?, ?, ?);");
        stmt.Bind(1, newId);
        stmt.Bind(2, cp.part);
        stmt.Bind(3, packed);
        stmt.Step();
    }

    db->MarkDirty();
    RefreshOutfits();
    QMessageBox::information(this, "Save Outfit", QString("Saved outfit \"%1\".").arg(name));
}

void LocalPlayerDialog::WearSelectedOutfit() {
    auto [db, outfitId] = SelectedOutfit(mOutfitList, mOutfitDbs);
    if (db == nullptr) {
        QMessageBox::information(this, "Wear Outfit", "Select an outfit to wear first.");
        return;
    }

    mWornSlots.clear();
    mWornAccessories.clear();
    mWornAccType.clear();
    {
        Statement stmt = db->PrepareStatement("SELECT AssetId FROM OutfitItem WHERE Id = ?;");
        stmt.Bind(1, outfitId);
        while (stmt.Step() == SQLITE_ROW)
            RouteWornAsset(stmt.GetInt64FromColumnIndex(0));
    }
    {
        Statement stmt = db->PrepareStatement("SELECT BodyPart, Color3 FROM OutfitBodyColor WHERE Id = ?;");
        stmt.Bind(1, outfitId);
        while (stmt.Step() == SQLITE_ROW) {
            int part = stmt.GetIntFromColumnIndex(0);
            int color = stmt.GetIntFromColumnIndex(1);
            QString key = ColorKeyForBodyPart(part);
            if (key.isEmpty() || !mBodyParts.contains(key))
                continue;
            QString name = BrickNameForRgb((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
            AvatarBodyPart& bp = mBodyParts[key];
            bp.colorName = name;
            bp.color = HexForBrickName(name);
            ApplyBodyColor(bp);
        }
    }
    {
        Statement stmt = db->PrepareStatement("SELECT BodyType, Width, Height, Head, Proportions FROM Outfit WHERE Id = ?;");
        stmt.Bind(1, outfitId);
        if (stmt.Step() == SQLITE_ROW) {
            int bodyType = stmt.GetIntFromColumnIndex(0);
            auto set = [&](const QString& key, double v) {
                if (v > 0.0 && mScaleFields.contains(key))
                    mScaleFields[key]->setValue(v);
            };
            set("user.appearance.scale.width",      stmt.GetDoubleFromColumnIndex(1));
            set("user.appearance.scale.height",     stmt.GetDoubleFromColumnIndex(2));
            set("user.appearance.scale.head",       stmt.GetDoubleFromColumnIndex(3));
            set("user.appearance.scale.proportion", stmt.GetDoubleFromColumnIndex(4));
            if (mAvatarTypeR6 && mAvatarTypeR15)
                (bodyType == 1 ? mAvatarTypeR15 : mAvatarTypeR6)->setChecked(true);
        }
    }

    RefreshAllTabs();
}

void LocalPlayerDialog::DeleteSelectedOutfit() {
    auto [db, outfitId] = SelectedOutfit(mOutfitList, mOutfitDbs);
    if (db == nullptr)
        return;
    if (QMessageBox::question(this, "Delete Outfit", "Delete the selected outfit?") != QMessageBox::Yes)
        return;

    { Statement s = db->PrepareStatement("DELETE FROM OutfitItem WHERE Id = ?;");      s.Bind(1, outfitId); s.Step(); }
    { Statement s = db->PrepareStatement("DELETE FROM OutfitBodyColor WHERE Id = ?;"); s.Bind(1, outfitId); s.Step(); }
    db->DeleteItem(ItemType::Outfit, outfitId);
    db->MarkDirty();
    RefreshOutfits();
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

    for (auto it = mScaleFields.begin(); it != mScaleFields.end(); ++it)
        it.value()->setValue(reg->GetKeyValue<double>(it.key().toStdString()).value_or(0.0));

    if (mAvatarTypeR6 && mAvatarTypeR15) {
        if (reg->GetKeyValue<std::string>("user.appearance.avatar_type").value_or("R6") == "R15")
            mAvatarTypeR15->setChecked(true);
        else
            mAvatarTypeR6->setChecked(true);
    }

    // Worn state.
    mWornSlots.clear();
    for (auto it = mTypeToSlotKey.begin(); it != mTypeToSlotKey.end(); ++it)
        mWornSlots[it.value()] = (qint64)reg->GetKeyValue<int64_t>(it.value().toStdString()).value_or(0);

    mWornAccessories.clear();
    if (auto table = reg->GetKeyValue<sol::table>("user.appearance.accessories"); table.has_value()) {
        const std::size_t count = table->size();
        for (std::size_t i = 1; i <= count; ++i) {
            sol::object obj = (*table)[i];
            if (obj.get_type() != sol::type::number)
                continue;
            qint64 id = (qint64)obj.as<int64_t>();
            if (id > 0)
                mWornAccessories.insert(id);
        }
    }

    // Resolve each worn accessory's type so the per-subgroup worn lists can filter on it.
    mWornAccType.clear();
    {
        EmuDbManager* mgr = gApp->GetCore()->GetEmuDbManager();
        for (qint64 id : mWornAccessories) {
            int type = 0;
            if (auto summary = mgr->GetAssetSummary(id); summary.has_value())
                type = summary->Type;
            mWornAccType[id] = type;
        }
    }

    // Initialise each tab on its first subgroup (the combo is already at index 0, so trigger manually).
    for (AvatarTab& tab : mTabs)
        OnSubgroupChanged(tab);

    RefreshOutfits();
}

void LocalPlayerDialog::SaveToRegistry() {
    Core* core = gApp->GetCore();
    Registry* reg = core->GetRegistry();

    reg->SetKeyValue("user.id", mIdInput->text().toLongLong());
    reg->SetKeyValue("user.name", mNameInput->text().toStdString());
    reg->SetKeyValue("user.display_name", mDisplayNameInput->text().toStdString());

    for (auto it = mBodyParts.begin(); it != mBodyParts.end(); ++it)
        reg->SetKeyValue("user.appearance.color." + it.key().toStdString(), it->colorName.toStdString());

    for (auto it = mScaleFields.begin(); it != mScaleFields.end(); ++it)
        reg->SetKeyValue<double>(it.key().toStdString(), it.value()->value());

    if (mAvatarTypeR15)
        reg->SetKeyValue("user.appearance.avatar_type", std::string(mAvatarTypeR15->isChecked() ? "R15" : "R6"));

    // Worn single-slot items.
    for (auto it = mTypeToSlotKey.begin(); it != mTypeToSlotKey.end(); ++it)
        reg->SetKeyValue<int64_t>(it.value().toStdString(), (int64_t)mWornSlots.value(it.value(), 0));

    // Accessories list.
    sol::table table = core->GetLuaState()->create_table();
    int n = 0;
    for (qint64 id : mWornAccessories)
        table[++n] = (int64_t)id;
    reg->SetKeyValue("user.appearance.accessories", table);
}
