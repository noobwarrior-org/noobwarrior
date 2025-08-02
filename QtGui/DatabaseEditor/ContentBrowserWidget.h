// === noobWarrior ===
// File: ContentBrowserWidget.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Dockable Qt widget that allows the user to explore the contents of an archive in an easily-digestible format
#pragma once
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

    template<typename T>
    void Refresh() {
        auto editor = dynamic_cast<DatabaseEditor*>(parent());
        Database *db = editor->GetCurrentlyEditingDatabase();

        mAssetCategory = static_cast<AssetCategory>(AssetCategoryDropdown->currentData().toInt());

        IdTypeDropdown->setCurrentText(QString::fromStdString(T::TableName));
        AssetTypeDropdown->setVisible(std::is_same_v<T, Asset>);
        AssetCategoryDropdown->setVisible(std::is_same_v<T, Asset>);

        AssetTypeDropdown->clear();
        for (int i = 0; i <= Roblox::AssetTypeCount; i++) {
            auto assetType = static_cast<Roblox::AssetType>(i);
            QString assetTypeStr = Roblox::AssetTypeAsTranslatableString(assetType);
            if (assetTypeStr.compare("None") != 0) {
                // Does this match our category type?
                if (AssetCategoryDropdown->currentIndex() == 0 || // If it's 0, then that should mean it's set to "Any" so let it through
                    MapAssetTypeToCategory(assetType) == mAssetCategory)
                    AssetTypeDropdown->addItem(assetTypeStr, i);
            }
        }

        if (!AssetCategoryDropdown->currentData().isNull())
            AssetTypeDropdown->setCurrentText(mAssetCategory == AssetCategory::AvatarItem ? "Hat" : "Model"); // set sane default.
        else
            AssetTypeDropdown->setCurrentText("All");

        NoDatabaseFoundLabel->setVisible(db == nullptr);
        List->setVisible(db != nullptr);
        List->clear();
        if (db == nullptr)
            return;

        SearchOptions opt {};
        opt.Offset = 0;
        opt.Limit = 100;

        std::vector<T> list = db->SearchContent<T>(opt);
        for (auto item : list) {
            new ContentListItem(db, &item, List);
            // cool->setIcon(QIcon(item.Icon));
        }
    }
private:
    void InitWidgets();
    void InitPageCounter();
    void GoToPage(int num);

    // Similarly to Roblox's Toolbox widget, we have a few dropdowns that allow you to filter out what you don't want.
    Roblox::AssetType   mAssetType;
    AssetCategory       mAssetCategory;

    //////////// QWidget stuff ////////////
    QWidget*        MainWidget;
    QVBoxLayout*    MainLayout;

    QComboBox*      IdTypeDropdown;
    QHBoxLayout*    AssetFilterDropdownLayout;
    QComboBox*      AssetTypeDropdown;
    QComboBox*      AssetCategoryDropdown;

    QLineEdit*      SearchBox;
    QListWidget*    List;
    QLabel*         NoDatabaseFoundLabel;
};
}
