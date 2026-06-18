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
#include <QSignalBlocker>
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPainter>
#include <QPalette>
#include <QGroupBox>
#include <QLabel>
#include <QDialog>

#include <memory>

#include <optional>
#include <string>

using namespace NoobWarrior;
using AT = Roblox::AssetType;

namespace {
// Draws a selected picker item as a full-bleed inverted box (light fill) with contrasting dark text,
// so the worn item is obvious. Scoped to the avatar picker lists (not the shared SDK item list).
// QSS can't fill the whole cell in IconMode, hence a delegate. Setting Highlight/HighlightedText in
// initStyleOption (the last thing before drawing) survives the base delegate's re-init, so the text
// stays dark even though ItemWidget sets a foreground brush.
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

    // Left column: identity form, the import shortcut, then the body-part swatches.
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
    tabs->setMinimumWidth(360);

    // Clothing — one item per slot (shirt/pants/t-shirt/face).
    {
        AvatarCategoryTab def;
        def.label = "Clothing";
        def.types = { AT::Shirt, AT::Pants, AT::TShirt };
        def.typeToRegKey[(int)AT::Shirt]  = "user.appearance.shirt";
        def.typeToRegKey[(int)AT::Pants]  = "user.appearance.pants";
        def.typeToRegKey[(int)AT::TShirt] = "user.appearance.tshirt";
        BuildCategoryTab(tabs, def);
    }
    // Accessories — many can be worn at once.
    {
        AvatarCategoryTab def;
        def.label = "Accessories";
        def.multiSelect = true;
        def.isAccessory = true;
        def.accessoryKey = "user.appearance.accessories";
        def.types = { AT::Hat, AT::HairAccessory, AT::FaceAccessory, AT::NeckAccessory,
                      AT::ShoulderAccessory, AT::FrontAccessory, AT::BackAccessory,
                      AT::WaistAccessory, AT::EarAccessory, AT::EyeAccessory };
        BuildCategoryTab(tabs, def);
    }
    // Body — per-part mesh/package override (one per part).
    {
        AvatarCategoryTab def;
        def.label = "Body";
        def.types = { AT::Package, AT::Face, AT::Head, AT::Torso, AT::LeftArm, AT::RightArm, AT::LeftLeg, AT::RightLeg };
        def.typeToRegKey[(int)AT::Package]  = "user.appearance.body.package";
        def.typeToRegKey[(int)AT::Face]     = "user.appearance.face";
        def.typeToRegKey[(int)AT::Head]     = "user.appearance.body.head";
        def.typeToRegKey[(int)AT::Torso]    = "user.appearance.body.torso";
        def.typeToRegKey[(int)AT::LeftArm]  = "user.appearance.body.left_arm";
        def.typeToRegKey[(int)AT::RightArm] = "user.appearance.body.right_arm";
        def.typeToRegKey[(int)AT::LeftLeg]  = "user.appearance.body.left_leg";
        def.typeToRegKey[(int)AT::RightLeg] = "user.appearance.body.right_leg";
        BuildCategoryTab(tabs, def);
    }
    // Animation — one per state.
    {
        AvatarCategoryTab def;
        def.label = "Animation";
        def.types = { AT::ClimbAnimation, AT::FallAnimation, AT::IdleAnimation, AT::JumpAnimation,
                      AT::RunAnimation, AT::SwimAnimation, AT::WalkAnimation };
        def.typeToRegKey[(int)AT::ClimbAnimation] = "user.appearance.animation.climb";
        def.typeToRegKey[(int)AT::FallAnimation]  = "user.appearance.animation.fall";
        def.typeToRegKey[(int)AT::IdleAnimation]  = "user.appearance.animation.idle";
        def.typeToRegKey[(int)AT::JumpAnimation]  = "user.appearance.animation.jump";
        def.typeToRegKey[(int)AT::RunAnimation]   = "user.appearance.animation.run";
        def.typeToRegKey[(int)AT::SwimAnimation]  = "user.appearance.animation.swim";
        def.typeToRegKey[(int)AT::WalkAnimation]  = "user.appearance.animation.walk";
        BuildCategoryTab(tabs, def);
    }

    tabs->addTab(BuildScaleTab(), "Scale");
    return tabs;
}

QWidget* LocalPlayerDialog::BuildScaleTab() {
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

void LocalPlayerDialog::BuildCategoryTab(QTabWidget* tabs, const AvatarCategoryTab& def) {
    mCategoryTabs.append(def);
    const int idx = mCategoryTabs.size() - 1;
    AvatarCategoryTab& tab = mCategoryTabs[idx];

    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    tab.search = new QLineEdit;
    tab.search->setPlaceholderText("Search " + tab.label.toLower() + "…");
    tab.search->setClearButtonEnabled(true);
    layout->addWidget(tab.search);

    tab.list = new ItemListWidget(page);
    tab.list->setSelectionMode(QAbstractItemView::MultiSelection);
    // Worn (selected) items get a full-bleed inverted box with contrasting text.
    tab.list->setItemDelegate(new WornItemDelegate(tab.list));
    // This is a picker, not an editor: a stray double-click or context-menu action would otherwise
    // open/delete real database items, and the list has no Populate database for those actions.
    tab.list->setContextMenuPolicy(Qt::NoContextMenu);
    tab.list->SetOnDoubleClick([](ItemWidget*) {});
    layout->addWidget(tab.list, 1);

    layout->addWidget(new QLabel(tab.isAccessory
        ? "Click to wear or remove. Multiple accessories can be worn at once."
        : "Click to wear an item. One item is worn per slot."));

    connect(tab.search, &QLineEdit::textChanged, this, [this, idx]() {
        PopulateCategory(mCategoryTabs[idx]);
    });
    connect(tab.list, &QListWidget::itemSelectionChanged, this, [this, idx]() {
        OnCategorySelectionChanged(mCategoryTabs[idx]);
    });

    tabs->addTab(page, tab.label);
}

void LocalPlayerDialog::PopulateCategory(AvatarCategoryTab& tab) {
    EmuDbManager* mgr = gApp->GetCore()->GetEmuDbManager();
    const QString query = tab.search ? tab.search->text().trimmed() : QString();

    tab.guard = true;
    tab.list->Clear();

    // Merge every matching asset from all mounted databases (priority order; the first database that
    // holds an id wins, since AddFromDatabase skips duplicates).
    for (EmuDb* db : mgr->GetMountedDatabases()) {
        for (AT type : tab.types) {
            std::string sql = "SELECT Id FROM Asset WHERE Type = ?";
            if (!query.isEmpty())
                sql += " AND Name LIKE ? ESCAPE '\\'";
            sql += " ORDER BY Name;";
            Statement stmt = db->PrepareStatement(sql);
            stmt.Bind(1, (int)type);
            if (!query.isEmpty())
                stmt.Bind(2, "%" + EscapeLike(query.toStdString()) + "%");
            while (stmt.Step() == SQLITE_ROW) {
                int64_t id = stmt.GetInt64FromColumnIndex(0);
                if (tab.list->AddFromDatabase(db, ItemType::Asset, id))
                    tab.idTypes[(qint64)id] = (int)type;
            }
        }
    }

    // When not filtering, keep worn items present even if no mounted database still has them, so they
    // stay visible and aren't silently dropped on save.
    if (query.isEmpty()) {
        std::vector<EmuDb*> dbs = mgr->GetMountedDatabases();
        EmuDb* fallback = dbs.empty() ? nullptr : dbs.front();
        for (qint64 id : tab.worn) {
            if (tab.list->IsItemInList(ItemType::Asset, id))
                continue;
            EmuDb* owner = mgr->GetFirstDbWhereItemExists(ItemType::Asset, id);
            tab.list->AddFromDatabase(owner ? owner : fallback, ItemType::Asset, id);
        }
    }

    tab.guard = false;
    ApplyWornSelection(tab);
}

void LocalPlayerDialog::ApplyWornSelection(AvatarCategoryTab& tab) {
    // Reflect the worn set onto the visible items without letting the selection handler run (it would
    // just re-derive the same worn set).
    QSignalBlocker block(tab.list);
    tab.list->clearSelection();
    for (qint64 id : tab.worn) {
        if (auto* iw = tab.list->GetItemWidget(ItemType::Asset, id))
            iw->setSelected(true);
    }
}

void LocalPlayerDialog::OnCategorySelectionChanged(AvatarCategoryTab& tab) {
    if (tab.guard)
        return;
    tab.guard = true;

    // Single-slot tabs allow at most one worn item per asset type: when a second item of a type is
    // selected, drop the previously-worn one of that type.
    if (!tab.isAccessory) {
        auto* current = dynamic_cast<ItemWidget*>(tab.list->currentItem());
        if (current && current->isSelected()) {
            const int curType = tab.idTypes.value((qint64)current->GetId(), -1);
            for (int i = 0; i < tab.list->count(); ++i) {
                auto* iw = dynamic_cast<ItemWidget*>(tab.list->item(i));
                if (!iw || iw == current || !iw->isSelected())
                    continue;
                if (tab.idTypes.value((qint64)iw->GetId(), -1) == curType)
                    iw->setSelected(false);
            }
        }
    }

    // Sync the visible items' selection into the worn set (worn items hidden by a search stay put).
    for (int i = 0; i < tab.list->count(); ++i) {
        auto* iw = dynamic_cast<ItemWidget*>(tab.list->item(i));
        if (!iw)
            continue;
        const qint64 id = (qint64)iw->GetId();
        if (iw->isSelected())
            tab.worn.insert(id);
        else
            tab.worn.remove(id);
    }

    tab.guard = false;
}

void LocalPlayerDialog::ReadWornFromRegistry(AvatarCategoryTab& tab) {
    Registry* reg = gApp->GetCore()->GetRegistry();
    tab.worn.clear();

    if (tab.isAccessory) {
        if (auto table = reg->GetKeyValue<sol::table>(tab.accessoryKey.toStdString()); table.has_value()) {
            const std::size_t count = table->size();
            for (std::size_t i = 1; i <= count; ++i) {
                sol::object obj = (*table)[i];
                if (obj.get_type() != sol::type::number)
                    continue;
                qint64 id = (qint64)obj.as<int64_t>();
                if (id > 0)
                    tab.worn.insert(id);
            }
        }
        return;
    }

    for (auto it = tab.typeToRegKey.begin(); it != tab.typeToRegKey.end(); ++it) {
        int64_t id = reg->GetKeyValue<int64_t>(it.value().toStdString()).value_or(0);
        if (id > 0) {
            tab.worn.insert((qint64)id);
            tab.idTypes[(qint64)id] = it.key();
        }
    }
}

void LocalPlayerDialog::SaveCategory(AvatarCategoryTab& tab) {
    Core* core = gApp->GetCore();
    Registry* reg = core->GetRegistry();

    if (tab.isAccessory) {
        sol::table table = core->GetLuaState()->create_table();
        int n = 0;
        for (qint64 id : tab.worn)
            table[++n] = (int64_t)id;
        reg->SetKeyValue(tab.accessoryKey.toStdString(), table);
        return;
    }

    // Reset every slot, then fill from the worn set keyed by each item's asset type.
    QMap<int, int64_t> slotById;
    for (qint64 id : tab.worn) {
        const int t = tab.idTypes.value(id, -1);
        if (t >= 0)
            slotById[t] = (int64_t)id;
    }
    for (auto it = tab.typeToRegKey.begin(); it != tab.typeToRegKey.end(); ++it)
        reg->SetKeyValue<int64_t>(it.value().toStdString(), slotById.value(it.key(), 0));
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

    // Remembers which database each shown user came from (the first mounted database that has a given
    // user id wins, matching AddFromDatabase's de-duplication).
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

    // Worn assets: route each into the category tab that owns its asset type.
    for (AvatarCategoryTab& tab : mCategoryTabs)
        tab.worn.clear();
    {
        Statement stmt = db->PrepareStatement("SELECT AssetId FROM UserCharacterItem WHERE Id = ?;");
        stmt.Bind(1, userId);
        while (stmt.Step() == SQLITE_ROW) {
            int64_t assetId = stmt.GetInt64FromColumnIndex(0);
            if (assetId <= 0)
                continue;
            int type = 0;
            if (auto summary = db->GetAssetSummary(assetId); summary.has_value())
                type = summary->Type;
            for (AvatarCategoryTab& tab : mCategoryTabs) {
                bool matches = false;
                for (AT t : tab.types)
                    if ((int)t == type) { matches = true; break; }
                if (matches) {
                    tab.worn.insert((qint64)assetId);
                    tab.idTypes[(qint64)assetId] = type;
                    break;
                }
            }
        }
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

    // Scales (only the four morphs stored on the User row round-trip).
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
        if (stmt.Step() == SQLITE_ROW) {
            if (stmt.GetIntFromColumnIndex(0) == 1)
                mAvatarTypeR15->setChecked(true);
            else
                mAvatarTypeR6->setChecked(true);
        }
    }

    for (AvatarCategoryTab& tab : mCategoryTabs)
        PopulateCategory(tab);
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

    if (reg->GetKeyValue<std::string>("user.appearance.avatar_type").value_or("R6") == "R15")
        mAvatarTypeR15->setChecked(true);
    else
        mAvatarTypeR6->setChecked(true);

    for (AvatarCategoryTab& tab : mCategoryTabs) {
        ReadWornFromRegistry(tab);
        PopulateCategory(tab);
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

    // Scales.
    for (auto it = mScaleFields.begin(); it != mScaleFields.end(); ++it)
        reg->SetKeyValue<double>(it.key().toStdString(), it.value()->value());

    reg->SetKeyValue("user.appearance.avatar_type", std::string(mAvatarTypeR15->isChecked() ? "R15" : "R6"));

    // Worn items per category.
    for (AvatarCategoryTab& tab : mCategoryTabs)
        SaveCategory(tab);
}
