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
// File: LocalPlayerDialog.h
// Started by: Hattozo
// Started on: 4/23/2026
// Description:
#pragma once
#include <NoobWarrior/Roblox/Api/Asset.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTabWidget>
#include <QStackedWidget>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QRadioButton>
#include <QColor>
#include <QMap>
#include <QSet>

#include <cstdint>
#include <utility>
#include <vector>

namespace NoobWarrior {
class ItemListWidget;
class EmuDb;

struct AvatarBodyPart {
    QString key;
    QString label;
    QPushButton* button;
    QString colorName;
    QColor color;
};

// One filter within a tab (e.g. Clothing's "Shirts", Body's "Scale"). A subgroup shows one asset
// type and equips into either a single registry slot, the shared accessories list, or — for the
// special Scale subgroup; swaps the item list for the scale/rig controls.
struct AvatarSubgroup {
    enum class Kind { Slot, Accessory, Scale };
    QString name;
    Roblox::AssetType type { Roblox::AssetType::None };
    Kind kind { Kind::Slot };
    QString regKey; // Slot only: the registry key this subgroup writes
};

// A top-level editor tab holding several subgroups. The catalog for the active subgroup is paginated
// in-memory (ids cached in pageIds; only the current page's items become widgets).
struct AvatarTab {
    QString name;
    std::vector<AvatarSubgroup> subgroups;

    QComboBox* subgroupCombo { nullptr };
    QLineEdit* search { nullptr };
    QStackedWidget* stack { nullptr }; // 0 = list+pagination, 1 = scale controls (if present)
    ItemListWidget* list { nullptr };
    ItemListWidget* wornList { nullptr }; // the currently-worn items for the active subgroup
    QLabel* pageLabel { nullptr };
    QPushButton* prevBtn { nullptr };
    QPushButton* nextBtn { nullptr };

    int page { 0 };
    int pageCount { 1 };
    std::vector<std::pair<qint64, EmuDb*>> pageIds; // cached ids for the active subgroup + search
};

class LocalPlayerDialog : public QDialog {
    Q_OBJECT
public:
    LocalPlayerDialog(QWidget *parent = nullptr);
    ~LocalPlayerDialog();
protected:
    void InitWidgets();

    QWidget* BuildAvatarBody();
    QWidget* BuildItemEditor();
    QWidget* BuildScaleWidget();
    QWidget* BuildOutfitsTab();
    void BuildTab(QTabWidget* tabs, AvatarTab def);
    void AddBodyPart(QGridLayout* grid, const QString& key, const QString& label,
                     int row, int col, int w, int h, const QString& defaultColorName);
    void PickBodyColor(AvatarBodyPart& part);
    void ApplyBodyColor(const AvatarBodyPart& part);

    QDoubleSpinBox* MakeScaleField(const QString& regKey);

    // Subgroup / pagination plumbing.
    const AvatarSubgroup& ActiveSubgroup(const AvatarTab& tab) const;
    void OnSubgroupChanged(AvatarTab& tab);
    void CollectIds(AvatarTab& tab);
    void RenderPage(AvatarTab& tab);
    void StepPage(AvatarTab& tab, int delta);
    void RenderWorn(AvatarTab& tab);
    void WearItem(AvatarTab& tab, qint64 id);
    void UnwearItem(AvatarTab& tab, qint64 id);
    void RefreshAllTabs();

    // Routes a worn asset id into the right slot / the accessories set based on its asset type.
    void RouteWornAsset(qint64 id);

    // Replaces the current appearance with a user's avatar stored in a mounted database.
    void ImportAvatarFromDatabase();
    void ApplyImportedAvatar(EmuDb* db, int64_t userId);

    // Outfits (stored in EmuDb's Outfit/OutfitItem/OutfitBodyColor tables).
    void RefreshOutfits();
    void SaveCurrentOutfit();
    void WearSelectedOutfit();
    void DeleteSelectedOutfit();

    void LoadFromRegistry();
    void SaveToRegistry();
private:
    QVBoxLayout* mLayout;
    QHBoxLayout* mMainLayout;
    QFormLayout* mFormLayout;

    QLineEdit* mIdInput;
    QLineEdit* mNameInput;
    QLineEdit* mDisplayNameInput;

    QMap<QString, AvatarBodyPart> mBodyParts;
    QMap<QString, QDoubleSpinBox*> mScaleFields;

    QRadioButton* mAvatarTypeR6 { nullptr };
    QRadioButton* mAvatarTypeR15 { nullptr };

    QList<AvatarTab> mTabs;
    
    QMap<QString, qint64> mWornSlots;   // registry slot key: worn asset id (0 = none)
    QSet<qint64> mWornAccessories;      // the flat accessories list
    QMap<qint64, int> mWornAccType;     // worn accessory id: its asset type (for per-subgroup worn lists)
    QMap<int, QString> mTypeToSlotKey;  // asset type: slot key (built from the Slot subgroups)
    QSet<int> mAccessoryTypes;          // asset types that equip into the accessories list

    ItemListWidget* mOutfitList { nullptr };
    QMap<qint64, EmuDb*> mOutfitDbs; // outfit id: the database it lives in (ids are per-database)

    QDialogButtonBox* mButtonBox;
};
}
