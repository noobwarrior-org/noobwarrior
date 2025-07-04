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
    MainWidget(nullptr),
    MainLayout(nullptr)
{
    setWindowTitle("Content Browser");
    InitWidgets();
}

ContentBrowserWidget::~ContentBrowserWidget() {}

void ContentBrowserWidget::Refresh() {
    auto editor = dynamic_cast<DatabaseEditor*>(parent());
    Database *db = editor->GetCurrentlyEditingDatabase();

    if (db == nullptr) {
        auto *label = new QLabel("No database loaded, there is no content to show", MainWidget);
        MainLayout->addWidget(label);
        return;
    }

    SearchOptions opt;
    opt.Offset = 0;
    opt.Limit = 100;

    for (auto asset : db->SearchAssets(opt)) {
        auto *label = new QLabel(QString::fromStdString(asset.Name), MainWidget);
        MainLayout->addWidget(label);
    }
}

void ContentBrowserWidget::InitWidgets() {
    MainWidget = new QWidget(this);
    setWidget(MainWidget);

    SearchBox = new QLineEdit(MainWidget);

    MainLayout = new QVBoxLayout(MainWidget);
    MainLayout->addWidget(SearchBox);

    SearchBox->setPlaceholderText("Search..."); // seeeaaaaaarch.... you know you wanna search...

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
