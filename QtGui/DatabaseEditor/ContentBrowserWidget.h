// === noobWarrior ===
// File: ContentBrowserWidget.h
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Dockable Qt widget that allows the user to explore the contents of an archive in an easily-digestible format
#pragma once
#include <NoobWarrior/ReflectionMetadata.h>
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

    /**
     * @brief A simple Refresh function that dynamically calls RefreshWithNewIdRecord() depending on what id type the user has
     * selected in the dropdown menu
     */
    std::function<void()> Refresh;
protected:
    void RefreshAssetCategory();

    /**
     * @brief Re-generates everything in the list, and uses the according IdRecord to search content.
     * @tparam T An IdRecord
     */
    template<typename T>
    void RefreshWithNewIdRecord() {
        static_assert(std::is_base_of_v<IdRecord, T>, "typename must inherit from IdRecord");
        auto editor = dynamic_cast<DatabaseEditor*>(parent());
        Database *db = editor->GetCurrentlyEditingDatabase();

        mAssetCategory = static_cast<AssetCategory>(AssetCategoryDropdown->currentData().toInt());
        mAssetType = static_cast<Roblox::AssetType>(AssetTypeDropdown->currentData().toInt());

        IdTypeDropdown->setCurrentText(QString::fromStdString(Reflection::GetIdTypeName<T>()));
        AssetCategoryDropdown->setVisible(std::is_same_v<T, Asset>);
        AssetTypeDropdown->setVisible(std::is_same_v<T, Asset>);

        NoDatabaseFoundLabel->setVisible(db == nullptr);
        List->setVisible(db != nullptr);
        List->clear();
        if (db == nullptr)
            return;

        SearchOptions opt {};
        opt.Offset = 0;
        opt.Limit = 100;
        opt.AssetType = mAssetType;

        std::vector<T> list = db->SearchContent<T>(opt);
        for (auto &item : list) {
            new ContentListItem<T>(item, db, List);
            // cool->setIcon(QIcon(item.Icon));
        }
    }
private:
    void InitWidgets();
    void InitPageCounter();
    void GoToPage(int num);

    std::vector<std::function<void()>> mRefreshFunctions;

    // Similarly to Roblox's Toolbox widget, we have a few dropdowns that allow you to filter out what you don't want.
    AssetCategory       mAssetCategory;
    Roblox::AssetType   mAssetType;

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
