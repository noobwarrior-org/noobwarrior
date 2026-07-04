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
// File: PlayerDialog.h
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
#include <QPixmap>
#include <QMap>
#include <QSet>

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>

class QMenu;

namespace NoobWarrior {
class ItemListWidget;
class EmuDb;
class AvatarBackend;
struct AvatarData;
struct AvatarCatalogPage;
struct PlayerThumbQueue; // thread-safe queue of fetched thumbnails (defined in the .cpp)

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
    bool single { false }; // Accessory only: at most one of this type worn (e.g. dynamic heads)
};

// One catalog entry on the active page. `db` is the local database that holds the asset (rendered from
// there); when null the asset is remote-only (the server has it but the client doesn't), so it's rendered
// from `name` + a `thumb` fetched from the server instead of being dropped.
struct AvatarPageItem {
    qint64 id { 0 };
    QString name;
    EmuDb* db { nullptr };
    QPixmap thumb;
};

// A top-level editor tab holding several subgroups. The catalog for the active subgroup is paginated
// in-memory (the page's items are cached in pageItems; only the current page becomes widgets).
struct AvatarTab {
    QString name;
    std::vector<AvatarSubgroup> subgroups;

    QComboBox* subgroupCombo { nullptr };
    QLineEdit* search { nullptr };
    QTimer* searchTimer { nullptr }; // debounces search typing so it fetches once you pause, not per keystroke
    QStackedWidget* stack { nullptr }; // 0 = list+pagination, 1 = scale controls (if present)
    ItemListWidget* list { nullptr };
    ItemListWidget* wornList { nullptr }; // the currently-worn items for the active subgroup
    QLabel* pageLabel { nullptr };
    QPushButton* prevBtn { nullptr };
    QPushButton* nextBtn { nullptr };

    int page { 0 };
    int pageCount { 1 };
    std::vector<AvatarPageItem> pageItems; // the active subgroup + search's current page
};

class PlayerDialog : public QDialog {
    Q_OBJECT
public:
    // Opens on the local player's registry appearance (auth-off play). The in-dialog target switcher
    // then lets you edit any signed-in server-emulator or master-server account's avatar instead.
    PlayerDialog(QWidget *parent = nullptr);
    ~PlayerDialog();
protected:
    // Defers closing the dialog while a blocking server call is running on a nested event loop (deleting
    // it mid-pump would unwind through a freed `this`); the close is honored once the pump finishes.
    void closeEvent(QCloseEvent* event) override;

    void InitWidgets();

    // Target switching: the current backend is the local registry player or a signed-in remote account.
    // Rebuilds the target-picker menu from the keychains, and swaps the active backend (nullptr = local,
    // else the dialog takes ownership), reloading the whole editor for the new target.
    QWidget* BuildTargetRow();
    void PopulateTargetMenu();
    void SwitchTarget(AvatarBackend* newBackend);
    void ApplyTargetToIdentity();
    QString CurrentTargetName() const;

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
    // True if tab is the one currently shown in the item-editor tab widget. Remote thumbnails are only
    // fetched for the visible tab, so switching to a remote account doesn't block on all four tabs at once.
    bool IsActiveTab(const AvatarTab& tab) const;
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

    bool LoadFromBackend(); // false if a remote target couldn't be loaded (widgets left untouched)
    bool SaveToBackend();

    // Backend calls: a local backend runs on the UI thread (Lua isn't thread-safe); a remote backend
    // runs on a worker thread while the UI keeps pumping its event loop — so contacting our OWN
    // in-process emulator (whose loop this same thread services) can't deadlock — with a busy dialog.
    bool BackendLoad(AvatarData& out);
    bool BackendSave(const AvatarData& data);
    AvatarCatalogPage BackendCatalog(int assetType, const std::string& search, int page);

    // Async thumbnails: a remote target's items render instantly (placeholder), and their thumbnails are
    // fetched on a background thread that pushes results into a mutex-guarded queue. A UI-thread QTimer
    // drains the queue and fills icons in, so the editor never blocks on thumbnail I/O.
    void StartThumbnailFetch(AvatarTab& tab); // fetch the visible tab's missing remote thumbnails
    void StopThumbnailFetch();                // cancel the in-flight fetch and invalidate its results
    void DrainThumbnails();                   // timer slot: apply whatever the worker has produced
    void ApplyThumbnail(int epoch, qint64 id, const std::vector<unsigned char>& bytes);

    // Pump depth guard: a RunPumped call brackets itself with BeginPump/EndPump so closeEvent knows a
    // nested event loop is on the stack and defers deletion until it unwinds.
    void BeginPump();
    void EndPump();
    void HonorDeferredClose();
private:
    AvatarBackend* mBackend { nullptr };
    bool mLocal { true }; // true = local registry player; false = a remote DB-backed account
    bool mDirty { false }; // unsaved edits to the current target (prompts a save before switching)
    int mPumpDepth { 0 };        // >0 while a blocking server call runs on a nested event loop
    std::atomic<bool> mCloseRequested { false }; // close requested during a pump; honored once idle
    QTabWidget* mItemTabs { nullptr }; // the item-editor tab widget (for the visible-tab check)

    // Async thumbnail loading state. The worker holds copies of the queue/cancel/inflight shared_ptrs, so it
    // never touches this dialog and can safely outlive it; results are matched to the live view by mFetchEpoch.
    std::shared_ptr<PlayerThumbQueue> mThumbQueue;          // worker -> UI results
    std::shared_ptr<std::atomic<bool>> mThumbCancel;        // cancels the current fetch's worker
    std::shared_ptr<std::atomic<int>> mThumbInFlight;       // running worker count (stops the timer when 0)
    int mFetchEpoch { 0 };                                  // bumped whenever the shown page/tab/target changes
    QTimer* mThumbTimer { nullptr };                        // drains mThumbQueue on the UI thread
    QPushButton* mTargetButton { nullptr };
    QMenu* mTargetMenu { nullptr };
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

    // Remote-only catalog items seen while browsing (id -> name + thumbnail), so the worn strip can render
    // an item the client has no local copy of once it's been shown in the picker.
    QMap<qint64, AvatarPageItem> mRemoteItemCache;

    QDialogButtonBox* mButtonBox;
};
}
