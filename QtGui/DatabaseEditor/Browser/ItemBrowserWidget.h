// === noobWarrior ===
// File: ItemBrowserWidget.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Dockable Qt widget that allows the user to explore the contents of a database in an easily-digestible format
#pragma once
#include <NoobWarrior/Roblox/Api/Asset.h>
#include <NoobWarrior/Database/Item/Asset.h>

#include "../DatabaseEditor.h"
#include "BrowserItem.h"
#include "AssetPage.h"

#include <QDockWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QComboBox>

namespace NoobWarrior {
class ItemBrowserWidget : public QDockWidget {
    Q_OBJECT
public:
    ItemBrowserWidget(QWidget *parent = nullptr);
    ~ItemBrowserWidget();

    Database* GetDatabase();
    void Refresh();
protected:
    void RefreshAssetCategory();

    void RefreshEx(int index) {
        auto editor = dynamic_cast<DatabaseEditor*>(parent());
        Database *db = editor->GetCurrentlyEditingDatabase();

        ItemBrowserPage *page = mPages.at(index);
        bool isAssetPage = dynamic_cast<AssetPage*>(page) != nullptr;

        for (ItemBrowserPage* page : mPages) {
            page->setVisible(false);
        }

        mAssetCategory = static_cast<AssetCategory>(AssetCategoryDropdown->currentData().toInt());
        mAssetType = static_cast<Roblox::AssetType>(AssetTypeDropdown->currentData().toInt());

        ItemTypeDropdown->setCurrentIndex(index);
        AssetCategoryDropdown->setVisible(isAssetPage);
        AssetTypeDropdown->setVisible(isAssetPage);

        NoDatabaseFoundLabel->setVisible(db == nullptr);
        if (db != nullptr) {
            page->setVisible(true);
            page->clear();
            page->Refresh();
        }
    }

    void RefreshEx(ItemBrowserPage *page) {
        auto pageIndex = std::find(mPages.begin(), mPages.end(), page);
        RefreshEx(std::distance(mPages.begin(), pageIndex));
    }
private:
    void InitWidgets();
    void InitPageCounter();
    void GoToPage(int num);

    // Similarly to Roblox's Toolbox widget, we have a few dropdowns that allow you to filter out what you don't want.
    AssetCategory       mAssetCategory;
    Roblox::AssetType   mAssetType;

    //////////// QWidget stuff ////////////
    std::vector<ItemBrowserPage*> mPages;
    int mCurrentPageIndex { 0 };

    QWidget*        MainWidget;
    QVBoxLayout*    MainLayout;

    QComboBox*      ItemTypeDropdown;
    QHBoxLayout*    AssetFilterDropdownLayout;
    QComboBox*      AssetTypeDropdown;
    QComboBox*      AssetCategoryDropdown;

    QLineEdit*      SearchBox;
    QLabel*         NoDatabaseFoundLabel;
};
}
