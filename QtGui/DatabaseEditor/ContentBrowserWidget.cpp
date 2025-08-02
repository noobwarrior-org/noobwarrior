// === noobWarrior ===
// File: ContentBrowserWidget.cpp
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Dockable Qt widget that allows the user to explore the contents of a database in an easily-digestible format
// It's very similar to the Roblox Studio toolbox widget.
// Limitations are that this doesn't support tree view, only per-page icon view.
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Database/Record/IdRecord.h>
#include <NoobWarrior/Database/AssetCategory.h>

#include "ContentBrowserWidget.h"
#include "ContentListItem.h"
#include "DatabaseEditor.h"

#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "ContentEditorDialog.h"

#define ADD_ID_TYPE(IdType, iconPath) QString IdType##_Str = IdType::TableName; \
    IdTypeDropdown->addItem(QIcon(iconPath), IdType##_Str);

using namespace NoobWarrior;

ContentBrowserWidget::ContentBrowserWidget(QWidget *parent) : QDockWidget(parent),
    mAssetType(Roblox::AssetType::Model),
    mAssetCategory(AssetCategory::DevelopmentItem),
    MainWidget(nullptr),
    MainLayout(nullptr),
    AssetFilterDropdownLayout(nullptr),
    IdTypeDropdown(nullptr),
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

/*void ContentBrowserWidget::Refresh() {
    auto editor = dynamic_cast<DatabaseEditor*>(parent());
    Database *db = editor->GetCurrentlyEditingDatabase();

    mIdType = static_cast<IdType>(IdTypeDropdown->currentData().toInt());
    mAssetCategory = static_cast<AssetCategory>(AssetCategoryDropdown->currentData().toInt());

    AssetTypeDropdown->setVisible(mIdType == IdType::Asset);
    AssetCategoryDropdown->setVisible(mIdType == IdType::Asset);

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

    std::vector<std::unique_ptr<IdRecord>> list;

    if (mIdType == IdType::Asset) {
        std::vector<Asset> assetsList = db->SearchContent<Asset>(opt);
        for (auto asset : assetsList)
            list.push_back(std::make_unique<Asset>(asset));
    }

    for (const auto &item : list) {
        new ContentListItem(db, item.get(), List);
        // cool->setIcon(QIcon(item.Icon));
    }
}*/

void ContentBrowserWidget::InitWidgets() {
    auto editor = dynamic_cast<DatabaseEditor*>(parent());

    MainWidget = new QWidget(this);
    setWidget(MainWidget);

    MainLayout = new QVBoxLayout(MainWidget);

    IdTypeDropdown = new QComboBox();

    AssetFilterDropdownLayout = new QHBoxLayout(MainWidget);

    AssetCategoryDropdown = new QComboBox();
    AssetCategoryDropdown->addItem("Any");

    AssetTypeDropdown = new QComboBox();
    AssetTypeDropdown->addItem("All");

    ADD_ID_TYPE(Asset, ":/images/silk/page.png")
    ADD_ID_TYPE(Badge, ":/images/silk/medal_gold_1.png")

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
    List->setMovement(QListView::Static);
    List->setViewMode(QListView::IconMode);
    List->setIconSize(QSize(64, 64));
    List->setWordWrap(true);
    SearchBox->setPlaceholderText("Search..."); // seeeaaaaaarch.... you know you wanna search...

    MainLayout->addWidget(IdTypeDropdown);
    MainLayout->addLayout(AssetFilterDropdownLayout);
    MainLayout->addWidget(SearchBox);
    MainLayout->addWidget(NoDatabaseFoundLabel);
    MainLayout->addWidget(List);

    // connect(IdTypeDropdown, &QComboBox::currentIndexChanged, this, &ContentBrowserWidget::Refresh);
    // connect(AssetCategoryDropdown, &QComboBox::currentIndexChanged, this, &ContentBrowserWidget::Refresh);
    // connect(AssetTypeDropdown, &QComboBox::currentIndexChanged, this, &ContentBrowserWidget::Refresh);
    connect(List, &QListWidget::itemDoubleClicked, this, [editor](QListWidgetItem *item) {
        auto editDialog = ContentEditorDialog<Asset>(editor);
        editDialog.exec();
    });

    InitPageCounter();
    Refresh<Asset>();
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
