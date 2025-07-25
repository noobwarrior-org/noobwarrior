// === noobWarrior ===
// File: ContentBrowserWidget.cpp
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Dockable Qt widget that allows the user to explore the contents of a database in an easily-digestible format
// Limitations are that this doesn't support tree view, only per-page icon view.
#include <NoobWarrior/NoobWarrior.h>
#include "ContentBrowserWidget.h"
#include "DatabaseEditor.h"

#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

using namespace NoobWarrior;

ContentBrowserWidget::ContentBrowserWidget(QWidget *parent) : QDockWidget(parent),
    mIdType(IdType::Asset),
    mAssetType(Roblox::AssetType::Model),
    MainWidget(nullptr),
    MainLayout(nullptr),
    FilterDropdownLayout(nullptr),
    IdTypeDropdown(nullptr),
    AssetTypeDropdown(nullptr),
    SearchBox(nullptr),
    List(nullptr),
    NoDatabaseFoundLabel(nullptr)
{
    assert(dynamic_cast<DatabaseEditor*>(this->parent()) != nullptr && "ContentBrowserWidget should not be parented to anything other than DatabaseEditor");
    setWindowTitle("Content Browser");
    InitWidgets();
}

ContentBrowserWidget::~ContentBrowserWidget() {}

void ContentBrowserWidget::Refresh() {
    auto editor = dynamic_cast<DatabaseEditor*>(parent());
    Database *db = editor->GetCurrentlyEditingDatabase();

    NoDatabaseFoundLabel->setVisible(db == nullptr);
    List->setVisible(db != nullptr);
    if (db == nullptr)
        return;

    SearchOptions opt;
    opt.Offset = 0;
    opt.Limit = 100;

    for (auto asset : db->SearchAssets(opt)) {
        auto *cool = new QListWidgetItem(List);
        cool->setText(QString::fromStdString(asset.Name));
        cool->setIcon(QIcon(":/images/silk/page.png"));
    }
}

void ContentBrowserWidget::InitWidgets() {
    MainWidget = new QWidget(this);
    setWidget(MainWidget);

    MainLayout = new QVBoxLayout(MainWidget);

    FilterDropdownLayout = new QHBoxLayout(MainWidget);

    IdTypeDropdown = new QComboBox();

    AssetTypeDropdown = new QComboBox();
    AssetTypeDropdown->addItem("All");

    for (int i = 0; i <= 7; i++) {
        auto idType = static_cast<IdType>(i);
        QString idTypeStr = IdTypeAsString(idType);
        IdTypeDropdown->addItem(QIcon(GetIconForIdType(idType)), idTypeStr);
    }

    for (int i = 1; i <= 79; i++) {
        auto assetType = static_cast<Roblox::AssetType>(i);
        QString assetTypeStr = Roblox::AssetTypeAsTranslatableString(assetType);
        if (assetTypeStr.compare("None") != 0)
            AssetTypeDropdown->addItem(assetTypeStr);
    }

    FilterDropdownLayout->addWidget(IdTypeDropdown);
    FilterDropdownLayout->addWidget(AssetTypeDropdown);

    NoDatabaseFoundLabel = new QLabel("No database loaded, there is no content to show", MainWidget);
    List = new QListWidget(MainWidget);
    SearchBox = new QLineEdit(MainWidget);

    NoDatabaseFoundLabel->setWordWrap(true);
    SearchBox->setPlaceholderText("Search..."); // seeeaaaaaarch.... you know you wanna search...

    MainLayout->addLayout(FilterDropdownLayout);
    MainLayout->addWidget(SearchBox);
    MainLayout->addWidget(NoDatabaseFoundLabel);
    MainLayout->addWidget(List);

    connect(IdTypeDropdown, &QComboBox::currentIndexChanged, this, &ContentBrowserWidget::Refresh);
    connect(AssetTypeDropdown, &QComboBox::currentIndexChanged, this, &ContentBrowserWidget::Refresh);

    InitPageCounter();
    Refresh();
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
