// === noobWarrior ===
// File: ContentBrowserWidget.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Dockable Qt widget that allows the user to explore the contents of an archive in an easily-digestible format
#pragma once
#include <NoobWarrior/Reflection.h>
#include <NoobWarrior/Roblox/Api/Asset.h>
#include <NoobWarrior/Database/AssetCategory.h>

#include "DatabaseEditor.h"
#include "ContentListItem.h"

#include <QDockWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QComboBox>

namespace NoobWarrior {
class ContentBrowserWidget : public QDockWidget {
    Q_OBJECT
public:
    ContentBrowserWidget(QWidget *parent = nullptr);
    ~ContentBrowserWidget();

    void Refresh();
protected:
    void RefreshAssetCategory();
    void RefreshEx(const Reflection::ItemType &itemType);
private:
    void InitWidgets();
    void InitPageCounter();
    void GoToPage(int num);

    Reflection::ItemType &mItemType;

    // Similarly to Roblox's Toolbox widget, we have a few dropdowns that allow you to filter out what you don't want.
    AssetCategory       mAssetCategory;
    Roblox::AssetType   mAssetType;

    //////////// QWidget stuff ////////////
    QWidget*        MainWidget;
    QVBoxLayout*    MainLayout;

    QComboBox*      ItemTypeDropdown;
    QHBoxLayout*    AssetFilterDropdownLayout;
    QComboBox*      AssetTypeDropdown;
    QComboBox*      AssetCategoryDropdown;

    QLineEdit*      SearchBox;
    QListWidget*    List;
    QLabel*         NoDatabaseFoundLabel;
};
}
