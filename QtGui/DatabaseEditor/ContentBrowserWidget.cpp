// === noobWarrior ===
// File: ContentBrowserWidget.cpp
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Dockable Qt widget that allows the user to explore the contents of a database in an easily-digestible format
// It's very similar to the Roblox Studio toolbox widget.
// Limitations are that this doesn't support tree view, only per-page icon view.
#include "ContentBrowserWidget.h"
#include "DatabaseEditor.h"

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Reflection.h>
#include <NoobWarrior/Database/AssetCategory.h>

#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

using namespace NoobWarrior;

ContentBrowserWidget::ContentBrowserWidget(QWidget *parent) : QDockWidget(parent),
    mItemType(const_cast<Reflection::ItemType&>(Reflection::GetItemType<Asset>())),
    mAssetCategory(AssetCategory::DevelopmentItem),
    mAssetType(Roblox::AssetType::Model),
    MainWidget(nullptr),
    MainLayout(nullptr),
    AssetFilterDropdownLayout(nullptr),
    ItemTypeDropdown(nullptr),
    AssetTypeDropdown(nullptr),
    AssetCategoryDropdown(nullptr),
    SearchBox(nullptr),
    List(nullptr),
    NoDatabaseFoundLabel(nullptr)
{
    assert(dynamic_cast<DatabaseEditor*>(this->parent()) != nullptr && "ContentBrowserWidget should not be parented to anything other than DatabaseEditor");
    setWindowTitle("Content Browser");
    InitWidgets();
}

ContentBrowserWidget::~ContentBrowserWidget() {}

void ContentBrowserWidget::RefreshAssetCategory() {
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

void ContentBrowserWidget::RefreshEx(const Reflection::ItemType &itemType) {
    auto editor = dynamic_cast<DatabaseEditor*>(parent());
    Database *db = editor->GetCurrentlyEditingDatabase();

    mItemType = itemType;
    mAssetCategory = static_cast<AssetCategory>(AssetCategoryDropdown->currentData().toInt());
    mAssetType = static_cast<Roblox::AssetType>(AssetTypeDropdown->currentData().toInt());

    ItemTypeDropdown->setCurrentText(QString::fromStdString(itemType.Name));
    AssetCategoryDropdown->setVisible(itemType.Name.compare("Asset") == 0);
    AssetTypeDropdown->setVisible(itemType.Name.compare("Asset") == 0);

    NoDatabaseFoundLabel->setVisible(db == nullptr);
    List->setVisible(db != nullptr);
    List->clear();
    if (db == nullptr)
        return;

    SearchOptions opt {};
    opt.Offset = 0;
    opt.Limit = 100;
    opt.AssetType = mAssetType;

    if (itemType.Name.compare("Asset") == 0) {
        std::vector<Asset> list = db->GetAssetRepository().List();
        for (auto &item : list) {
            new ContentListItem(itemType, item.Id, db, List);
            // cool->setIcon(QIcon(item.Icon));
        }
    }
}

void ContentBrowserWidget::Refresh() {
    RefreshEx(mItemType);
}

void ContentBrowserWidget::InitWidgets() {
    auto editor = dynamic_cast<DatabaseEditor*>(parent());

    MainWidget = new QWidget(this);
    setWidget(MainWidget);

    MainLayout = new QVBoxLayout(MainWidget);

    ItemTypeDropdown = new QComboBox();

    AssetFilterDropdownLayout = new QHBoxLayout(MainWidget);

    AssetCategoryDropdown = new QComboBox();
    AssetCategoryDropdown->addItem("Any");

    AssetTypeDropdown = new QComboBox();

    for (const Reflection::ItemType &itemType : Reflection::GetItemTypes()) {
        QString str = QString::fromStdString(itemType.Name);
        ItemTypeDropdown->addItem(QIcon(""), str, QVariant::fromValue((Reflection::ItemType*)&itemType));
    }

    for (int i = 0; i <= AssetCategoryCount; i++) {
        auto assetTypeCategory = static_cast<AssetCategory>(i);
        QString assetTypeCategoryStr = AssetCategoryAsTranslatableString(assetTypeCategory);
        AssetCategoryDropdown->addItem(assetTypeCategoryStr, i);
    }

    AssetFilterDropdownLayout->addWidget(AssetCategoryDropdown);
    AssetFilterDropdownLayout->addWidget(AssetTypeDropdown);

    NoDatabaseFoundLabel = new QLabel("No database loaded, there is no content to show", MainWidget);
    List = new QListWidget(MainWidget);
    SearchBox = new QLineEdit(MainWidget);

    NoDatabaseFoundLabel->setWordWrap(true);
    List->setAutoFillBackground(true);
    List->setMovement(QListView::Static);
    List->setViewMode(QListView::IconMode);
    List->setIconSize(QSize(64, 64));
    List->setWordWrap(true);
    SearchBox->setPlaceholderText("Search..."); // seeeaaaaaarch.... you know you wanna search...

    MainLayout->addWidget(ItemTypeDropdown);
    MainLayout->addLayout(AssetFilterDropdownLayout);
    MainLayout->addWidget(SearchBox);
    MainLayout->addWidget(NoDatabaseFoundLabel);
    MainLayout->addWidget(List);

    connect(ItemTypeDropdown, &QComboBox::currentIndexChanged, this, [this](int index) {
        RefreshEx(*ItemTypeDropdown->currentData().value<Reflection::ItemType*>());
    });
    connect(AssetCategoryDropdown, &QComboBox::currentIndexChanged, this, [this](int index) {
        RefreshAssetCategory();
    });
    connect(AssetTypeDropdown, &QComboBox::currentIndexChanged, this, [this](int index) {
        Refresh();
    });
    connect(List, &QListWidget::itemDoubleClicked, this, [editor](QListWidgetItem *item) {
        auto *contentItem = dynamic_cast<ContentListItem*>(item);
        contentItem->Configure(editor);
    });

    InitPageCounter();
    RefreshAssetCategory();
    GoToPage(1);
}

void ContentBrowserWidget::InitPageCounter() {
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

void ContentBrowserWidget::GoToPage(int num) {

}
