// === noobWarrior ===
// File: ItemBrowserWidget.cpp
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Dockable Qt widget that allows the user to explore the contents of a database in an easily-digestible format
// It's very similar to the Roblox Studio toolbox widget.
// Limitations are that this doesn't support tree view, only per-page icon view.
#include "ItemBrowserWidget.h"
#include "../DatabaseEditor.h"

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Database/Item/Asset.h>

#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#define ADD_ITEMTYPE(type, pageType) mPages.push_back(new pageType(this)); \
    ItemBrowserPage* type##_page = mPages.back(); \
    MainLayout->addWidget(type##_page); \
    QString type##_Str = QString::fromStdString(#type); \
    ItemTypeDropdown->addItem(QIcon(), type##_Str, QVariant::fromValue(type##_page));

using namespace NoobWarrior;

ItemBrowserWidget::ItemBrowserWidget(QWidget *parent) : QDockWidget(parent),
    mAssetCategory(AssetCategory::DevelopmentItem),
    mAssetType(Roblox::AssetType::Model),
    MainWidget(nullptr),
    MainLayout(nullptr),
    AssetFilterDropdownLayout(nullptr),
    ItemTypeDropdown(nullptr),
    AssetTypeDropdown(nullptr),
    AssetCategoryDropdown(nullptr),
    SearchBox(nullptr),
    NoDatabaseFoundLabel(nullptr)
{
    assert(dynamic_cast<DatabaseEditor*>(this->parent()) != nullptr && "ItemBrowserWidget should not be parented to anything other than DatabaseEditor");
    setWindowTitle("Item Browser");
    InitWidgets();
}

ItemBrowserWidget::~ItemBrowserWidget() {}

Database* ItemBrowserWidget::GetDatabase() {
    assert(dynamic_cast<DatabaseEditor*>(parent()) != nullptr && "ItemBrowserWidget should not be parented to anything other than DatabaseEditor");
    auto editor = dynamic_cast<DatabaseEditor*>(parent());
    return editor->GetCurrentlyEditingDatabase();
}

void ItemBrowserWidget::RefreshAssetCategory() {
    AssetTypeDropdown->clear();
    AssetTypeDropdown->addItem("All");
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

    Refresh();
}

void ItemBrowserWidget::Refresh() {
    RefreshEx(mCurrentPageIndex);
}

void ItemBrowserWidget::InitWidgets() {
    auto editor = dynamic_cast<DatabaseEditor*>(parent());

    MainWidget = new QWidget(this);
    setWidget(MainWidget);

    MainLayout = new QVBoxLayout(MainWidget);

    ItemTypeDropdown = new QComboBox();

    AssetFilterDropdownLayout = new QHBoxLayout(MainWidget);

    AssetCategoryDropdown = new QComboBox();
    AssetCategoryDropdown->addItem("Any");

    AssetTypeDropdown = new QComboBox();

    for (int i = 0; i <= AssetCategoryCount; i++) {
        auto assetTypeCategory = static_cast<AssetCategory>(i);
        QString assetTypeCategoryStr = AssetCategoryAsTranslatableString(assetTypeCategory);
        AssetCategoryDropdown->addItem(assetTypeCategoryStr, i);
    }

    AssetFilterDropdownLayout->addWidget(AssetCategoryDropdown);
    AssetFilterDropdownLayout->addWidget(AssetTypeDropdown);

    NoDatabaseFoundLabel = new QLabel("No database loaded", MainWidget);
    SearchBox = new QLineEdit(MainWidget);

    NoDatabaseFoundLabel->setWordWrap(true);
    SearchBox->setPlaceholderText("Search...");

    MainLayout->addWidget(ItemTypeDropdown);
    MainLayout->addLayout(AssetFilterDropdownLayout);
    MainLayout->addWidget(SearchBox);
    MainLayout->addWidget(NoDatabaseFoundLabel);

    ADD_ITEMTYPE(Asset, AssetPage)

    connect(ItemTypeDropdown, &QComboBox::currentIndexChanged, this, [this](int index) {
        mCurrentPageIndex = index;
        RefreshEx(index);
    });
    connect(AssetCategoryDropdown, &QComboBox::currentIndexChanged, this, [this](int index) {
        RefreshAssetCategory();
    });
    connect(AssetTypeDropdown, &QComboBox::currentIndexChanged, this, [this](int index) {
        Refresh();
    });

    InitPageCounter();
    RefreshAssetCategory();
    GoToPage(1);
}

void ItemBrowserWidget::InitPageCounter() {
    auto pageCountLayout = new QHBoxLayout(MainWidget);

    auto backButton = new QPushButton("Back", MainWidget);
    auto nextButton = new QPushButton("Next", MainWidget);

    auto pageLabel = new QLabel("Page", MainWidget);
    auto pageInputLabel = new QSpinBox(MainWidget);
    auto pageLabelTotal = new QLabel("of 1", MainWidget);

    pageCountLayout->addWidget(backButton);
    pageCountLayout->addWidget(pageLabel);
    pageCountLayout->addWidget(pageInputLabel);
    pageCountLayout->addWidget(pageLabelTotal);
    pageCountLayout->addWidget(nextButton);

    MainLayout->addLayout(pageCountLayout);
}

void ItemBrowserWidget::GoToPage(int num) {

}
