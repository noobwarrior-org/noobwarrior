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
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QRadioButton>
#include <QColor>
#include <QMap>
#include <QSet>

#include <cstdint>
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

// One tab of the avatar editor. A tab shows every asset of its `types` aggregated from all mounted
// databases. `worn` is the authoritative set of equipped asset ids (kept separate from the list's
// transient selection so a search that hides a worn item never drops it on save).
struct AvatarCategoryTab {
    QString label;
    bool multiSelect = false;            // accessories: many items can be worn at once
    bool isAccessory = false;            // saved to the accessories table rather than per-slot keys
    std::vector<Roblox::AssetType> types;
    QMap<int, QString> typeToRegKey;     // single-slot tabs: assetType value -> registry key
    QString accessoryKey;                // accessory tab: the registry table key

    ItemListWidget* list = nullptr;
    QLineEdit* search = nullptr;

    QSet<qint64> worn;                   // equipped asset ids (source of truth)
    QMap<qint64, int> idTypes;           // asset id -> its asset type value (persistent)
    bool guard = false;                  // re-entrancy guard while syncing selection
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
    QWidget* BuildScaleTab();
    void BuildCategoryTab(QTabWidget* tabs, const AvatarCategoryTab& def);
    void AddBodyPart(QGridLayout* grid, const QString& key, const QString& label,
                     int row, int col, int w, int h, const QString& defaultColorName);
    void PickBodyColor(AvatarBodyPart& part);
    void ApplyBodyColor(const AvatarBodyPart& part);

    QDoubleSpinBox* MakeScaleField(const QString& regKey);

    // Avatar category tabs.
    void PopulateCategory(AvatarCategoryTab& tab);
    void ApplyWornSelection(AvatarCategoryTab& tab);
    void OnCategorySelectionChanged(AvatarCategoryTab& tab);
    void ReadWornFromRegistry(AvatarCategoryTab& tab);
    void SaveCategory(AvatarCategoryTab& tab);

    // Replaces the current appearance with a user's avatar stored in a mounted database.
    void ImportAvatarFromDatabase();
    // Applies a stored user's identity, worn items, body colors, scales and avatar type to the dialog.
    void ApplyImportedAvatar(EmuDb* db, int64_t userId);

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
    QList<AvatarCategoryTab> mCategoryTabs;

    QRadioButton* mAvatarTypeR6 { nullptr };
    QRadioButton* mAvatarTypeR15 { nullptr };

    QDialogButtonBox* mButtonBox;
};
}
