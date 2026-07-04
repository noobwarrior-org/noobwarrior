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
// File: PlayerDialog.cpp
// Started by: Hattozo
// Started on: 4/23/2026
// Description:
#include "PlayerDialog.h"
#include "AvatarBackend.h"
#include "../Application.h"

#include "Sdk/Item/ItemListWidget.h"
#include "Sdk/Item/ItemWidget.h"

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/Roblox/DataType/BrickColor.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <NoobWarrior/Keychain/Keychain.h>
#include <NoobWarrior/Keychain/EmuKeychain.h>
#include <NoobWarrior/Keychain/MasterKeychain.h>

#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QStyledItemDelegate>
#include <QEventLoop>
#include <QTimer>

#include <BusyDialog.h>

#include <mutex>
#include <thread>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QComboBox>
#include <QStackedWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPainter>
#include <QPalette>
#include <QGroupBox>
#include <QLabel>
#include <QImage>
#include <QCloseEvent>
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

namespace NoobWarrior {
// One background-fetched thumbnail, tagged with the fetch epoch it belongs to (so a result from a
// superseded fetch can be discarded).
struct ThumbResult {
    int Epoch;
    qint64 Id;
    std::vector<unsigned char> Bytes;
};
// The worker->UI handoff: the fetch thread appends results under the mutex; the UI-thread timer drains it.
struct PlayerThumbQueue {
    std::mutex Mutex;
    std::vector<ThumbResult> Items;
};
}

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

PlayerDialog::PlayerDialog(QWidget *parent) : QDialog(parent) {
    // Opens on the local player; the target switcher swaps to a signed-in account's avatar.
    mBackend = new LocalRegistryBackend(gApp->GetCore());
    mLocal = true;
    InitWidgets();
    ApplyTargetToIdentity();
    LoadFromBackend();
    mDirty = false;
}

PlayerDialog::~PlayerDialog() {
    StopThumbnailFetch(); // cancel the background fetch (its worker self-cleans via shared_ptrs)
    delete mBackend;
}

QString PlayerDialog::CurrentTargetName() const {
    return mLocal ? QStringLiteral("Local Player") : QString::fromStdString(mBackend->Describe());
}

// Reflects the current target in the window title, the switcher button, and the identity fields (which
// are editable for the local player but fixed/disabled for a remote account — only its avatar changes).
void PlayerDialog::ApplyTargetToIdentity() {
    Registry* reg = gApp->GetCore()->GetRegistry();
    setWindowTitle(mLocal ? "Player Settings"
                          : QString("Player - %1").arg(CurrentTargetName()));
    if (mTargetButton)
        mTargetButton->setText(CurrentTargetName());
    if (mLocal) {
        mIdInput->setText(QString::number(reg->GetKeyValue<int64_t>("user.id").value_or(1000)));
        mNameInput->setText(QString::fromStdString(reg->GetKeyValue<std::string>("user.name").value_or("Player")));
        mDisplayNameInput->setText(QString::fromStdString(reg->GetKeyValue<std::string>("user.display_name").value_or("Player")));
    } else {
        mIdInput->clear();
        mDisplayNameInput->clear();
        mNameInput->setText(QString::fromStdString(mBackend->Describe()));
    }
    mIdInput->setEnabled(mLocal);
    mNameInput->setEnabled(mLocal);
    mDisplayNameInput->setEnabled(mLocal);
}

void PlayerDialog::InitWidgets() {
    // Async thumbnail plumbing: a shared queue the fetch worker pushes into, and a UI-thread timer that
    // drains it and fills in icons.
    mThumbQueue = std::make_shared<PlayerThumbQueue>();
    mThumbInFlight = std::make_shared<std::atomic<int>>(0);
    mThumbTimer = new QTimer(this);
    connect(mThumbTimer, &QTimer::timeout, this, &PlayerDialog::DrainThumbnails);

    mLayout = new QVBoxLayout(this);
    mMainLayout = new QHBoxLayout;
    mFormLayout = new QFormLayout;

    mIdInput = new QLineEdit;
    mNameInput = new QLineEdit;
    mDisplayNameInput = new QLineEdit;

    // Editing the local player's identity marks it dirty (a remote account's fields are disabled).
    for (QLineEdit* f : { mIdInput, mNameInput, mDisplayNameInput })
        connect(f, &QLineEdit::textEdited, this, [this](const QString&) { mDirty = true; });

    mIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mIdInput));
    mFormLayout->addRow("User Id", mIdInput);
    mFormLayout->addRow("Name", mNameInput);
    mFormLayout->addRow("Display Name", mDisplayNameInput);

    QPushButton* importButton = new QPushButton(QIcon(":/images/roblox_backup.png"), "Import Avatar from Database…");
    connect(importButton, &QPushButton::clicked, this, &PlayerDialog::ImportAvatarFromDatabase);

    QVBoxLayout* leftColumn = new QVBoxLayout;
    leftColumn->addLayout(mFormLayout);
    leftColumn->addWidget(importButton);
    leftColumn->addWidget(BuildAvatarBody());
    leftColumn->addStretch();

    mMainLayout->addLayout(leftColumn);
    mMainLayout->addWidget(BuildItemEditor(), 1);

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(mButtonBox, &QDialogButtonBox::accepted, [this]() {
        if (SaveToBackend())
            close();
    });
    connect(mButtonBox, &QDialogButtonBox::rejected, [this]() {
        close();
    });

    mLayout->addWidget(BuildTargetRow());
    mLayout->addLayout(mMainLayout);
    mLayout->addWidget(mButtonBox);
}

// The target switcher: a button whose menu lists the local player plus every signed-in server-emulator
// and master-server account, rebuilt each time it opens so it tracks the keychains.
QWidget* PlayerDialog::BuildTargetRow() {
    QWidget* row = new QWidget;
    QHBoxLayout* lay = new QHBoxLayout(row);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(new QLabel("Editing:"));

    mTargetButton = new QPushButton(QIcon(":/images/silk/user.png"), CurrentTargetName());
    mTargetMenu = new QMenu(mTargetButton);
    connect(mTargetMenu, &QMenu::aboutToShow, this, &PlayerDialog::PopulateTargetMenu);
    mTargetButton->setMenu(mTargetMenu);
    lay->addWidget(mTargetButton);
    lay->addStretch();
    return row;
}

void PlayerDialog::PopulateTargetMenu() {
    mTargetMenu->clear();
    Core* core = gApp->GetCore();

    QAction* local = mTargetMenu->addAction(QIcon(":/images/silk/user.png"), "Local Player");
    local->setCheckable(true);
    local->setChecked(mLocal);
    connect(local, &QAction::triggered, this, [this]() { SwitchTarget(nullptr); });

    // Signed-in server-emulator accounts (their own accounts; master-auth, not slaves).
    std::vector<Account>& emu = core->GetEmuKeychain()->GetAccounts();
    if (!emu.empty()) {
        mTargetMenu->addSeparator();
        mTargetMenu->addAction("Server emulator accounts")->setEnabled(false);
        for (Account& acc : emu) {
            std::string label = (acc.DisplayName.empty() ? acc.Name : acc.DisplayName) + " on " + acc.Name;
            std::string base = "https://" + acc.Name, token = acc.Token;
            QAction* a = mTargetMenu->addAction(QIcon(":/images/silk/user.png"), QString::fromStdString(label));
            a->setCheckable(true);
            a->setChecked(!mLocal && CurrentTargetName() == QString::fromStdString(label));
            connect(a, &QAction::triggered, this, [this, base, token, label]() {
                SwitchTarget(new RemoteAccountBackend(base, token, label));
            });
        }
    }

    // Master-server accounts (federated identities).
    std::vector<Account>& master = core->GetMasterKeychain()->GetAccounts();
    if (!master.empty()) {
        mTargetMenu->addSeparator();
        mTargetMenu->addAction("Master server accounts")->setEnabled(false);
        for (Account& acc : master) {
            std::string url = acc.Url, token = acc.Token, name = acc.Name;
            QAction* a = mTargetMenu->addAction(QIcon(":/images/silk/user.png"), QString::fromStdString(name));
            a->setCheckable(true);
            a->setChecked(!mLocal && CurrentTargetName() == QString::fromStdString(name));
            connect(a, &QAction::triggered, this, [this, url, token, name]() {
                SwitchTarget(new RemoteAccountBackend(url, token, name));
            });
        }
    }
}

// Swaps the active backend (nullptr = local player, else a remote account the dialog takes ownership of),
// prompting to save first if the current target has unsaved edits, then reloads the editor for the new one.
void PlayerDialog::SwitchTarget(AvatarBackend* newBackend) {
    const bool toLocal = (newBackend == nullptr);
    // Picking the target we're already on is a no-op.
    if (toLocal == mLocal &&
        (toLocal || CurrentTargetName() == QString::fromStdString(newBackend->Describe()))) {
        delete newBackend;
        return;
    }

    if (mDirty) {
        QMessageBox::StandardButton r = QMessageBox::question(this, "Unsaved changes",
            QString("Save changes to %1 before switching?").arg(CurrentTargetName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        if (r == QMessageBox::Cancel) { delete newBackend; return; }
        if (r == QMessageBox::Save && !SaveToBackend()) { delete newBackend; return; } // save failed, stay put
    }

    StopThumbnailFetch(); // stop fetching the outgoing target's thumbnails

    // Swap in the new target but KEEP the old backend, so an unreachable server can be undone: on failure
    // LoadFromBackend leaves the displayed avatar untouched, so we just restore the previous backend.
    AvatarBackend* prevBackend = mBackend;
    const bool prevLocal = mLocal;

    mBackend = toLocal ? static_cast<AvatarBackend*>(new LocalRegistryBackend(gApp->GetCore())) : newBackend;
    mLocal = toLocal;
    ApplyTargetToIdentity();
    if (!LoadFromBackend()) {
        delete mBackend;          // couldn't load the new target
        mBackend = prevBackend;   // fall back to the previous one (its avatar is still on screen)
        mLocal = prevLocal;
        ApplyTargetToIdentity();
        return;
    }
    delete prevBackend;
    mDirty = false;
}

QWidget* PlayerDialog::BuildAvatarBody() {
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

void PlayerDialog::AddBodyPart(QGridLayout* grid, const QString& key, const QString& label,
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

void PlayerDialog::ApplyBodyColor(const AvatarBodyPart& part) {
    part.button->setToolTip(QString("%1: %2").arg(part.label, part.colorName));
    part.button->setStyleSheet(QString(
        "QPushButton { background-color: %1; border: 1px solid #1b1b1b; border-radius: 3px; }"
        "QPushButton:hover { border: 1px solid #ffffff; }")
        .arg(part.color.name()));
}

void PlayerDialog::PickBodyColor(AvatarBodyPart& part) {
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
    mDirty = true;
    ApplyBodyColor(part);
}

QDoubleSpinBox* PlayerDialog::MakeScaleField(const QString& regKey) {
    QDoubleSpinBox* field = new QDoubleSpinBox;
    field->setRange(0.0, 2.0);
    field->setSingleStep(0.05);
    field->setDecimals(2);
    connect(field, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) { mDirty = true; });
    mScaleFields.insert(regKey, field);
    return field;
}

QWidget* PlayerDialog::BuildScaleWidget() {
    QWidget* page = new QWidget;
    QFormLayout* form = new QFormLayout(page);

    // Rig type: R6 (6 limbs) vs R15 (15-part rig). Drives resolvedAvatarType in the served avatar.
    mAvatarTypeR6 = new QRadioButton("R6");
    mAvatarTypeR15 = new QRadioButton("R15");
    mAvatarTypeR6->setChecked(true);
    connect(mAvatarTypeR15, &QRadioButton::toggled, this, [this](bool) { mDirty = true; });
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

QWidget* PlayerDialog::BuildItemEditor() {
    QTabWidget* tabs = new QTabWidget;
    tabs->setMinimumWidth(400);

    auto slot = [](const char* n, AT t, const char* key) {
        return AvatarSubgroup{ n, t, Kind::Slot, key };
    };
    auto acc = [](const char* n, AT t, bool single = false) {
        return AvatarSubgroup{ n, t, Kind::Accessory, QString(), single };
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
            acc("Hair",          AT::HairAccessory),
            acc("Dynamic Heads", AT::DynamicHead, /*single*/ true),
            slot("Faces",        AT::Face,     "user.appearance.face"),
            slot("Torso",        AT::Torso,    "user.appearance.body.torso"),
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

    mItemTabs = tabs;
    // Thumbnails for a remote account are only fetched for the visible tab; when a tab is first shown,
    // (re)collect it so its remote-only items pull their thumbnails then. Local targets need nothing here.
    connect(tabs, &QTabWidget::currentChanged, this, [this](int i) {
        if (mLocal || i < 0 || i >= mTabs.size())
            return;
        AvatarTab& tab = mTabs[i];
        if (ActiveSubgroup(tab).kind == Kind::Scale)
            return;
        CollectIds(tab);
        RenderPage(tab);
        RenderWorn(tab);
    });
    return tabs;
}

void PlayerDialog::BuildTab(QTabWidget* tabs, AvatarTab def) {
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
    // Debounced search: each keystroke just restarts the timer; the fetch (which may hit a remote server)
    // only fires once the user pauses, so typing stays smooth and doesn't repeatedly refetch.
    tab.searchTimer = new QTimer(this);
    tab.searchTimer->setSingleShot(true);
    connect(tab.searchTimer, &QTimer::timeout, this, [this, idx]() {
        CollectIds(mTabs[idx]);
        mTabs[idx].page = 0;
        RenderPage(mTabs[idx]);
    });
    connect(tab.search, &QLineEdit::textChanged, this, [this, idx](const QString&) {
        mTabs[idx].searchTimer->start(300);
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

const AvatarSubgroup& PlayerDialog::ActiveSubgroup(const AvatarTab& tab) const {
    int i = tab.subgroupCombo ? tab.subgroupCombo->currentIndex() : 0;
    if (i < 0 || i >= (int)tab.subgroups.size())
        i = 0;
    return tab.subgroups[i];
}

bool PlayerDialog::IsActiveTab(const AvatarTab& tab) const {
    if (mItemTabs == nullptr)
        return true; // before the tab widget exists, treat every tab as active (initial build)
    int i = mItemTabs->currentIndex();
    return i >= 0 && i < mTabs.size() && &mTabs[i] == &tab;
}

void PlayerDialog::OnSubgroupChanged(AvatarTab& tab) {
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

void PlayerDialog::CollectIds(AvatarTab& tab) {
    // Fetches the CURRENT page from the backend's catalog (the local DBs for the local player, or the
    // remote server's catalog for an account). Each id is bound to whichever local database has it for
    // rendering; a remote server may list assets the client has no local copy of — those render from a
    // thumbnail fetched asynchronously (see StartThumbnailFetch), so this never blocks on thumbnail I/O.
    tab.pageItems.clear();
    const AvatarSubgroup& sg = ActiveSubgroup(tab);
    if (sg.kind == Kind::Scale) {
        tab.pageCount = 1;
        return;
    }

    AvatarCatalogPage cat = BackendCatalog(static_cast<int>(sg.type), tab.search->text().trimmed().toStdString(), tab.page);
    tab.pageCount = std::max(1, cat.PageCount);
    tab.page = std::clamp(cat.Page, 0, tab.pageCount - 1);

    EmuDbManager* mgr = gApp->GetCore()->GetEmuDbManager();
    QSet<qint64> seen;
    for (const AvatarCatalogItem& item : cat.Items) {
        qint64 id = static_cast<qint64>(item.Id);
        if (id <= 0 || seen.contains(id))
            continue;
        seen.insert(id);
        AvatarPageItem pi;
        pi.id = id;
        pi.name = QString::fromStdString(item.Name);
        pi.db = mgr->GetFirstDbWhereItemExists(ItemType::Asset, id);
        if (pi.db == nullptr && !mLocal) {
            // Remote-only item. Reuse an already-fetched thumbnail if we have one; otherwise leave it null
            // (RenderPage shows a placeholder) and remember the item so an async result can fill its icon.
            if (auto c = mRemoteItemCache.find(id); c != mRemoteItemCache.end() && !c->thumb.isNull())
                pi.thumb = c->thumb;
            else if (!mRemoteItemCache.contains(id))
                mRemoteItemCache.insert(id, pi); // records the name; thumb filled in by ApplyThumbnail
        }
        tab.pageItems.push_back(std::move(pi));
    }
}

void PlayerDialog::RenderPage(AvatarTab& tab) {
    const AvatarSubgroup& sg = ActiveSubgroup(tab);
    if (sg.kind == Kind::Scale)
        return;

    // pageItems already holds just the current page (CollectIds fetched it), so render all of it. Items
    // the client has locally render from their database; a remote-only item renders immediately with its
    // cached thumbnail (or a placeholder), and StartThumbnailFetch fills any missing ones in the background.
    tab.list->Clear();
    for (const AvatarPageItem& pi : tab.pageItems) {
        if (pi.db != nullptr)
            tab.list->AddFromDatabase(pi.db, ItemType::Asset, pi.id);
        else
            tab.list->AddRemote(pi.id, pi.name, pi.thumb);
    }

    tab.pageLabel->setText(QString("Page %1 / %2").arg(tab.page + 1).arg(tab.pageCount));
    tab.prevBtn->setEnabled(tab.page > 0);
    tab.nextBtn->setEnabled(tab.page < tab.pageCount - 1);

    if (!mLocal && IsActiveTab(tab))
        StartThumbnailFetch(tab); // kick off background thumbnail loading for the visible tab
}

void PlayerDialog::StepPage(AvatarTab& tab, int delta) {
    const int np = tab.page + delta;
    if (np < 0 || np >= tab.pageCount)
        return;
    tab.page = np;
    CollectIds(tab); // re-fetch the new page from the backend
    RenderPage(tab);
}

void PlayerDialog::RenderWorn(AvatarTab& tab) {
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
        if (EmuDb* db = mgr->GetFirstDbWhereItemExists(ItemType::Asset, id)) {
            tab.wornList->AddFromDatabase(db, ItemType::Asset, id);
            return;
        }
        // Remote-only worn item: render from the browsing cache if we've seen it, else a bare id fallback.
        if (auto it = mRemoteItemCache.find(id); it != mRemoteItemCache.end()) {
            tab.wornList->AddRemote(id, it->name, it->thumb);
            return;
        }
        std::vector<EmuDb*> dbs = mgr->GetMountedDatabases();
        tab.wornList->AddFromDatabase(dbs.empty() ? nullptr : dbs.front(), ItemType::Asset, id);
    };

    if (sg.kind == Kind::Slot) {
        add(mWornSlots.value(sg.regKey, 0));
    } else {
        for (qint64 id : mWornAccessories)
            if (mWornAccType.value(id, -1) == (int)sg.type)
                add(id);
    }
}

void PlayerDialog::WearItem(AvatarTab& tab, qint64 id) {
    if (id <= 0)
        return;
    const AvatarSubgroup& sg = ActiveSubgroup(tab);
    if (sg.kind == Kind::Slot) {
        mWornSlots[sg.regKey] = id;
    } else if (sg.kind == Kind::Accessory) {
        if (sg.single) {
            QList<qint64> replaced;
            for (qint64 wid : mWornAccessories)
                if (wid != id && mWornAccType.value(wid, -1) == (int)sg.type)
                    replaced.append(wid);
            for (qint64 wid : replaced) {
                mWornAccessories.remove(wid);
                mWornAccType.remove(wid);
            }
        }
        mWornAccessories.insert(id);
        mWornAccType[id] = (int)sg.type;
    } else {
        return;
    }
    mDirty = true;
    RenderWorn(tab);
}

void PlayerDialog::UnwearItem(AvatarTab& tab, qint64 id) {
    const AvatarSubgroup& sg = ActiveSubgroup(tab);
    if (sg.kind == Kind::Slot) {
        if (mWornSlots.value(sg.regKey, 0) == id)
            mWornSlots[sg.regKey] = 0;
    } else if (sg.kind == Kind::Accessory) {
        mWornAccessories.remove(id);
        mWornAccType.remove(id);
    }
    mDirty = true;
    RenderWorn(tab);
}

void PlayerDialog::RefreshAllTabs() {
    for (AvatarTab& tab : mTabs) {
        if (ActiveSubgroup(tab).kind == Kind::Scale)
            continue;
        CollectIds(tab);
        RenderPage(tab);
        RenderWorn(tab);
    }
}

void PlayerDialog::RouteWornAsset(qint64 id) {
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

void PlayerDialog::ImportAvatarFromDatabase() {
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

void PlayerDialog::ApplyImportedAvatar(EmuDb* db, int64_t userId) {
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

    mDirty = true;
    RefreshAllTabs();
}

QWidget* PlayerDialog::BuildOutfitsTab() {
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

    connect(saveBtn, &QPushButton::clicked, this, &PlayerDialog::SaveCurrentOutfit);
    connect(wearBtn, &QPushButton::clicked, this, &PlayerDialog::WearSelectedOutfit);
    connect(delBtn,  &QPushButton::clicked, this, &PlayerDialog::DeleteSelectedOutfit);
    connect(mOutfitList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) { WearSelectedOutfit(); });

    return page;
}

void PlayerDialog::RefreshOutfits() {
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

void PlayerDialog::SaveCurrentOutfit() {
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

void PlayerDialog::WearSelectedOutfit() {
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

    mDirty = true;
    RefreshAllTabs();
}

void PlayerDialog::DeleteSelectedOutfit() {
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

template <typename R, typename F>
static R RunPumped(F fn) {
    QEventLoop loop;
    R result {};
    std::thread worker([&]() {
        result = fn();
        QMetaObject::invokeMethod(&loop, [&loop]() { loop.quit(); }, Qt::QueuedConnection);
    });
    // Exclude user input while pumping: timers (the Core event pump + thumbnail drain) still fire, but
    // clicks/keystrokes are deferred until the call returns — so the caller needn't disable the dialog to
    // block re-entrant fetches, and the search field keeps focus while a fetch runs.
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    worker.join();
    return result;
}

// A blocking server call runs on a nested event loop (RunPumped). That loop delivers the window-close
// event, and QDialog's WA_DeleteOnClose would delete the dialog while these member functions are still on
// the stack -> use-after-free on unwind. BeginPump/EndPump track that a pump is active; closeEvent defers
// the close until the pump unwinds and control is back in the main event loop.
void PlayerDialog::BeginPump() { mPumpDepth++; }
void PlayerDialog::EndPump() {
    if (--mPumpDepth == 0 && mCloseRequested)
        QTimer::singleShot(0, this, [this]() { HonorDeferredClose(); });
}
void PlayerDialog::HonorDeferredClose() {
    if (mPumpDepth == 0 && mCloseRequested) {
        mCloseRequested = false;
        close();
    }
}

void PlayerDialog::closeEvent(QCloseEvent* event) {
    if (mPumpDepth > 0) {
        mCloseRequested = true;
        event->ignore();
        return;
    }
    QDialog::closeEvent(event);
}

bool PlayerDialog::BackendLoad(AvatarData& out) {
    if (mLocal)
        return mBackend->Load(out);
    BusyDialog busy("Contacting the server...", this);
    busy.show();
    bool ok = false;
    BeginPump();
    out = RunPumped<AvatarData>([this, &ok]() { AvatarData d; ok = mBackend->Load(d); return d; });
    EndPump();
    busy.close();
    return ok;
}

bool PlayerDialog::BackendSave(const AvatarData& data) {
    if (mLocal)
        return mBackend->Save(data);
    BusyDialog busy("Saving to the server...", this);
    busy.show();
    BeginPump();
    bool ok = RunPumped<bool>([this, &data]() { return mBackend->Save(data); });
    EndPump();
    busy.close();
    return ok;
}

AvatarCatalogPage PlayerDialog::BackendCatalog(int assetType, const std::string& search, int page) {
    if (mLocal)
        return mBackend->Catalog(assetType, search, page);
    // No busy dialog and no setEnabled: RunPumped excludes user input while it runs, which blocks re-entrant
    // fetches without disabling (and stealing focus from) the search field.
    BeginPump();
    AvatarCatalogPage cat = RunPumped<AvatarCatalogPage>(
        [this, assetType, search, page]() { return mBackend->Catalog(assetType, search, page); });
    EndPump();
    return cat;
}

void PlayerDialog::StartThumbnailFetch(AvatarTab& tab) {
    if (mLocal)
        return;
    std::vector<int64_t> ids;
    for (const AvatarPageItem& pi : tab.pageItems)
        if (pi.db == nullptr && pi.thumb.isNull())
            ids.push_back(pi.id);
    if (ids.empty())
        return;

    ThumbnailFetcher fetch = mBackend->MakeThumbnailFetcher();
    if (!fetch)
        return;

    StopThumbnailFetch(); // supersede any previous fetch and invalidate its results
    const int epoch = mFetchEpoch;
    auto cancel = std::make_shared<std::atomic<bool>>(false);
    mThumbCancel = cancel;
    auto queue = mThumbQueue;
    auto inflight = mThumbInFlight;
    inflight->fetch_add(1);

    // The worker holds only copies (fetch/queue/cancel/inflight) — never `this` or the backend — so it is
    // safe even if the dialog is closed while it runs. Results flow through the queue, drained by mThumbTimer.
    std::thread([fetch = std::move(fetch), ids = std::move(ids), epoch, cancel, queue, inflight]() {
        for (int64_t id : ids) {
            if (cancel->load())
                break;
            std::vector<unsigned char> bytes = fetch(id);
            if (cancel->load())
                break;
            if (!bytes.empty()) {
                std::lock_guard<std::mutex> lk(queue->Mutex);
                queue->Items.push_back({ epoch, static_cast<qint64>(id), std::move(bytes) });
            }
        }
        inflight->fetch_sub(1);
    }).detach();

    if (!mThumbTimer->isActive())
        mThumbTimer->start(120);
}

void PlayerDialog::StopThumbnailFetch() {
    if (mThumbCancel) {
        mThumbCancel->store(true);
        mThumbCancel.reset();
    }
    mFetchEpoch++; // any results still in flight now carry a stale epoch and are ignored
}

void PlayerDialog::DrainThumbnails() {
    std::vector<ThumbResult> batch;
    {
        std::lock_guard<std::mutex> lk(mThumbQueue->Mutex);
        batch.swap(mThumbQueue->Items);
    }
    for (const ThumbResult& r : batch)
        ApplyThumbnail(r.Epoch, r.Id, r.Bytes);
    if (batch.empty() && mThumbInFlight->load() == 0)
        mThumbTimer->stop();
}

void PlayerDialog::ApplyThumbnail(int epoch, qint64 id, const std::vector<unsigned char>& bytes) {
    QImage img;
    img.loadFromData(bytes.data(), static_cast<int>(bytes.size()));
    if (img.isNull())
        return;
    QPixmap px = QPixmap::fromImage(img);

    // Cache it (keeping the item's name) so revisits and the worn strip can reuse it.
    if (auto c = mRemoteItemCache.find(id); c != mRemoteItemCache.end())
        c->thumb = px;
    else {
        AvatarPageItem pi;
        pi.id = id;
        pi.thumb = px;
        mRemoteItemCache.insert(id, pi);
    }

    if (epoch != mFetchEpoch)
        return; // a newer fetch (tab / page / target change) owns the display now

    int idx = mItemTabs ? mItemTabs->currentIndex() : -1;
    if (idx < 0 || idx >= mTabs.size())
        return;
    AvatarTab& tab = mTabs[idx];
    for (AvatarPageItem& pi : tab.pageItems)
        if (pi.id == id && pi.db == nullptr) {
            pi.thumb = px;
            break;
        }
    if (tab.list)
        if (ItemWidget* w = tab.list->GetItemWidget(ItemType::Asset, id))
            w->SetRemoteIcon(px);
    if (tab.wornList)
        if (ItemWidget* w = tab.wornList->GetItemWidget(ItemType::Asset, id))
            w->SetRemoteIcon(px);
}

bool PlayerDialog::LoadFromBackend() {
    AvatarData data;
    if (!BackendLoad(data) && !mLocal) {
        // Remote target unreachable. Leave the currently-displayed avatar untouched and report failure so
        // the caller (SwitchTarget) can fall back to the previous target instead of showing a blank noob.
        QMessageBox::warning(this, "Avatar", "Couldn't load your avatar from the server. It may be unreachable.");
        return false;
    }

    for (auto it = mBodyParts.begin(); it != mBodyParts.end(); ++it) {
        auto found = data.Colors.find(it.key().toStdString());
        if (found == data.Colors.end())
            continue;
        it->colorName = QString::fromStdString(found->second);
        it->color = HexForBrickName(it->colorName);
        ApplyBodyColor(*it);
    }

    for (auto it = mScaleFields.begin(); it != mScaleFields.end(); ++it) {
        auto found = data.Scales.find(it.key().toStdString());
        it.value()->setValue(found != data.Scales.end() ? found->second : 0.0);
    }

    if (mAvatarTypeR6 && mAvatarTypeR15) {
        if (data.AvatarType == "R15")
            mAvatarTypeR15->setChecked(true);
        else
            mAvatarTypeR6->setChecked(true);
    }

    // Worn state.
    mWornSlots.clear();
    for (auto it = mTypeToSlotKey.begin(); it != mTypeToSlotKey.end(); ++it) {
        auto found = data.Slots.find(it.value().toStdString());
        mWornSlots[it.value()] = found != data.Slots.end() ? (qint64)found->second : 0;
    }

    mWornAccessories.clear();
    for (int64_t id : data.Accessories)
        if (id > 0)
            mWornAccessories.insert((qint64)id);

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
    // Bail if the user asked to close mid-load (a slow remote target) so the close feels responsive.
    for (AvatarTab& tab : mTabs) {
        if (mCloseRequested)
            break;
        OnSubgroupChanged(tab);
    }

    RefreshOutfits();
    mDirty = false; // loading a target's stored avatar isn't an edit (neutralises signals fired above)
    return true;
}

bool PlayerDialog::SaveToBackend() {
    // Identity (user.id/name/display_name) is only editable for the local player; a remote account's
    // identity is fixed, so only its avatar is written.
    if (mLocal) {
        Registry* reg = gApp->GetCore()->GetRegistry();
        reg->SetKeyValue("user.id", mIdInput->text().toLongLong());
        reg->SetKeyValue("user.name", mNameInput->text().toStdString());
        reg->SetKeyValue("user.display_name", mDisplayNameInput->text().toStdString());
    }

    AvatarData data;
    for (auto it = mBodyParts.begin(); it != mBodyParts.end(); ++it)
        data.Colors[it.key().toStdString()] = it->colorName.toStdString();
    for (auto it = mScaleFields.begin(); it != mScaleFields.end(); ++it)
        data.Scales[it.key().toStdString()] = it.value()->value();
    data.AvatarType = (mAvatarTypeR15 && mAvatarTypeR15->isChecked()) ? "R15" : "R6";
    for (auto it = mTypeToSlotKey.begin(); it != mTypeToSlotKey.end(); ++it)
        data.Slots[it.value().toStdString()] = (int64_t)mWornSlots.value(it.value(), 0);
    for (qint64 id : mWornAccessories)
        data.Accessories.push_back((int64_t)id);

    if (!BackendSave(data)) {
        if (!mLocal)
            QMessageBox::critical(this, "Avatar",
                "Couldn't save your avatar to the server. It may be unreachable. Your changes were not saved.");
        return false;
    }
    mDirty = false;
    return true;
}
