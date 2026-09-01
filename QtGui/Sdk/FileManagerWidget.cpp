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
// File: FileManagerWidget.cpp
// Started by: Hattozo
// Started on: 7/2/2025
// Description: A widget that allows the user to reorganize content in a way they like by putting them into directories.
// This is different from the regular item browser because that organizes stuff on its own according to what it's
// seeing in the SQLite database. This lets you organize it yourself, just like a traditional file manager: tree
// views, folders, documents, and shortcuts to Roblox items, all backed by the database's FsNode table.
#include "FileManagerWidget.h"
#include "Sdk.h"
#include "Sdk/Project/EmuDb/EmuDbProject.h"
#include "Sdk/Item/ItemDialog.h"
#include "Sdk/Item/ItemOpenSaveDialog.h"
#include "Sdk/CodeEditorWidget.h"

#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/EmuDb/ItemType.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QHeaderView>
#include <QListWidget>
#include <QTabWidget>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QDateTime>
#include <QEvent>
#include <QPushButton>
#include <QLocale>
#include <QDir>
#include <QFile>

#include <fstream>

using namespace NoobWarrior;

// Roles used to stash the FsNode id / type on view items.
static constexpr int kIdRole = Qt::UserRole + 1;
static constexpr int kTypeRole = Qt::UserRole + 2;
// On the Date Modified (col 2) and Size (col 4) columns we also stash the raw underlying value in
// Qt::UserRole so the tree can sort them numerically instead of by their formatted text.
static constexpr int kSortRole = Qt::UserRole;

namespace {
// A tree item that sorts the Date Modified and Size columns by their stored numeric value (epoch
// seconds / byte count) rather than the displayed string, so "10 KB" sorts after "2 KB" and dates
// sort chronologically. Other columns fall back to a case-insensitive text compare.
class FmTreeItem : public QTreeWidgetItem {
public:
    using QTreeWidgetItem::QTreeWidgetItem;
    bool operator<(const QTreeWidgetItem &other) const override {
        const int col = treeWidget() ? treeWidget()->sortColumn() : 1;
        if (col == 2)
            return data(2, kSortRole).toLongLong() < other.data(2, kSortRole).toLongLong();
        if (col == 4)
            return data(4, kSortRole).toULongLong() < other.data(4, kSortRole).toULongLong();
        return text(col).compare(other.text(col), Qt::CaseInsensitive) < 0;
    }
};

void FocusEditor(SourceEditorContainer* editor) {
    if (editor == nullptr)
        return;
    if (editor->isWindow()) {
        editor->raise();
        editor->activateWindow();
        return;
    }
    QWidget* page = editor;
    for (QWidget* w = editor->parentWidget(); w != nullptr; w = w->parentWidget()) {
        if (auto* tabs = qobject_cast<QTabWidget*>(w)) {
            const int index = tabs->indexOf(page);
            if (index >= 0)
                tabs->setCurrentIndex(index);
            page = tabs;
        }
    }
}
}

FileManagerWidget::FileManagerWidget(QWidget *parent) : QDockWidget(parent)
{
    assert(dynamic_cast<Sdk*>(this->parent()) != nullptr && "FileManagerWidget should not be parented to anything other than Sdk");
    setWindowTitle("File Manager");
    InitWidgets();
    Reload();
}

FileManagerWidget::~FileManagerWidget() {}

void FileManagerWidget::InitWidgets() {
    MainWidget = new QWidget(this);
    setWidget(MainWidget);

    MainLayout = new QVBoxLayout(MainWidget);
    MainLayout->setContentsMargins(2, 2, 2, 2);

    // ---- navigation row: back / forward / up / refresh + address + search ----
    auto navRow = new QHBoxLayout();
    navRow->setContentsMargins(0, 0, 0, 0);

    auto makeToolButton = [this](const QString &icon, const QString &tip) {
        auto btn = new QToolButton(MainWidget);
        btn->setIcon(QIcon(icon));
        btn->setToolTip(tip);
        btn->setAutoRaise(true);
        return btn;
    };

    BackButton = makeToolButton(":/images/silk/arrow_left.png", "Back");
    ForwardButton = makeToolButton(":/images/silk/arrow_right.png", "Forward");
    UpButton = makeToolButton(":/images/silk/arrow_up.png", "Up one level");
    RefreshButton = makeToolButton(":/images/silk/arrow_refresh.png", "Refresh");

    AddressBar = new QLineEdit(MainWidget);
    AddressBar->setPlaceholderText("/");
    AddressBar->setClearButtonEnabled(true);

    SearchBar = new QLineEdit(MainWidget);
    SearchBar->setPlaceholderText("Search");
    SearchBar->setClearButtonEnabled(true);
    SearchBar->setMaximumWidth(160);

    navRow->addWidget(BackButton);
    navRow->addWidget(ForwardButton);
    navRow->addWidget(UpButton);
    navRow->addWidget(RefreshButton);
    navRow->addWidget(AddressBar, 1);
    navRow->addWidget(SearchBar);
    MainLayout->addLayout(navRow);

    connect(BackButton, &QToolButton::clicked, this, &FileManagerWidget::GoBack);
    connect(ForwardButton, &QToolButton::clicked, this, &FileManagerWidget::GoForward);
    connect(UpButton, &QToolButton::clicked, this, &FileManagerWidget::GoUp);
    connect(RefreshButton, &QToolButton::clicked, this, [this]() { Populate(); });
    connect(AddressBar, &QLineEdit::returnPressed, this, [this]() { NavigateTo(AddressBar->text()); });
    connect(SearchBar, &QLineEdit::textChanged, this, [this](const QString &text) {
        mSearchFilter = text;
        Populate();
    });

    // ---- the views (details tree + list/icons), stacked with a placeholder ----
    ViewStack = new QStackedWidget(MainWidget);

    DetailsView = new QTreeWidget(ViewStack);
    DetailsView->setColumnCount(5);
    DetailsView->setHeaderLabels({ "", "Name", "Date Modified", "Type", "Size" });
    DetailsView->setColumnHidden(0, true);
    DetailsView->setTreePosition(1);
    DetailsView->setRootIsDecorated(false);
    DetailsView->setSortingEnabled(true);
    DetailsView->sortByColumn(1, Qt::AscendingOrder);
    DetailsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    DetailsView->setContextMenuPolicy(Qt::CustomContextMenu);
    DetailsView->header()->setStretchLastSection(false);
    DetailsView->header()->setSectionsMovable(true);
    DetailsView->header()->setMinimumSectionSize(40);
    for (int col = 1; col < DetailsView->columnCount(); ++col)
        DetailsView->header()->setSectionResizeMode(col, QHeaderView::Interactive);
    DetailsView->header()->resizeSection(1, 260);
    DetailsView->header()->resizeSection(2, 130);
    DetailsView->header()->resizeSection(3, 120);
    DetailsView->header()->resizeSection(4, 90);
    DetailsView->viewport()->installEventFilter(this);

    ListView = new QListWidget(ViewStack);
    ListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ListView->setContextMenuPolicy(Qt::CustomContextMenu);
    ListView->setResizeMode(QListView::Adjust);
    ListView->setMovement(QListView::Static);
    ListView->setUniformItemSizes(true);

    PlaceholderLabel = new QLabel("Open a database project to use the file manager.", ViewStack);
    PlaceholderLabel->setAlignment(Qt::AlignCenter);
    PlaceholderLabel->setWordWrap(true);

    ViewStack->addWidget(DetailsView);      // index 0
    ViewStack->addWidget(ListView);         // index 1
    ViewStack->addWidget(PlaceholderLabel); // index 2
    MainLayout->addWidget(ViewStack, 1);

    connect(DetailsView, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
        if (item) ActivateNode(item->data(0, kIdRole).toLongLong());
    });
    connect(ListView, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        if (item) ActivateNode(item->data(kIdRole).toLongLong());
    });

    connect(DetailsView, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        bool onItem = DetailsView->itemAt(pos) != nullptr;
        ShowContextMenu(DetailsView->viewport()->mapToGlobal(pos), onItem);
    });
    connect(ListView, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        bool onItem = ListView->itemAt(pos) != nullptr;
        ShowContextMenu(ListView->viewport()->mapToGlobal(pos), onItem);
    });

    SetViewMode(ViewMode::Details);
}

EmuDb* FileManagerWidget::GetDatabase() {
    auto sdk = dynamic_cast<Sdk*>(parent());
    if (sdk == nullptr)
        return nullptr;
    auto dbProj = dynamic_cast<EmuDbProject*>(sdk->GetFocusedProject());
    return dbProj != nullptr ? dbProj->GetDb() : nullptr;
}

DatabaseFileSystem* FileManagerWidget::EnsureFileSystem() {
    EmuDb* db = GetDatabase();
    if (db != mFsDb) {
        mFs.reset(db != nullptr ? new DatabaseFileSystem(db) : nullptr);
        mFsDb = db;
        // The focused database changed: drop back to its root and forget history.
        mCurrentPath = "/";
        mBackStack.clear();
        mForwardStack.clear();
    }
    return mFs.get();
}

void FileManagerWidget::Refresh(const QString &address) {
    NavigateTo(address, true);
}

void FileManagerWidget::Reload() {
    EnsureFileSystem();
    Populate();
}

QString FileManagerWidget::ChildPath(const QString &name) const {
    QString base = (mCurrentPath == "/") ? QString() : mCurrentPath;
    std::string joined = (base + "/" + name).toStdString();
    return QString::fromStdString(DatabaseFileSystem::NormalizePath(joined));
}

void FileManagerWidget::NavigateTo(const QString &path, bool pushHistory) {
    DatabaseFileSystem* fs = EnsureFileSystem();
    QString norm = QString::fromStdString(DatabaseFileSystem::NormalizePath(path.toStdString()));

    if (fs != nullptr) {
        std::optional<DatabaseFileSystem::Node> node = fs->GetNodeByPath(norm.toStdString());
        if (!node || node->Type != DatabaseFileSystem::NodeType::Directory) {
            QMessageBox::warning(this, "Cannot Open Path", QString("\"%1\" is not a folder in this database.").arg(norm));
            AddressBar->setText(mCurrentPath);
            return;
        }
    }

    if (pushHistory && norm != mCurrentPath) {
        mBackStack.push_back(mCurrentPath);
        mForwardStack.clear();
    }
    mCurrentPath = norm;
    Populate();
}

void FileManagerWidget::GoBack() {
    if (mBackStack.isEmpty())
        return;
    mForwardStack.push_back(mCurrentPath);
    mCurrentPath = mBackStack.takeLast();
    Populate();
}

void FileManagerWidget::GoForward() {
    if (mForwardStack.isEmpty())
        return;
    mBackStack.push_back(mCurrentPath);
    mCurrentPath = mForwardStack.takeLast();
    Populate();
}

void FileManagerWidget::GoUp() {
    if (mCurrentPath == "/")
        return;
    auto [parent, name] = DatabaseFileSystem::SplitParentAndName(mCurrentPath.toStdString());
    NavigateTo(QString::fromStdString(parent));
}

void FileManagerWidget::UpdateNavButtons() {
    BackButton->setEnabled(!mBackStack.isEmpty());
    ForwardButton->setEnabled(!mForwardStack.isEmpty());
    UpButton->setEnabled(mCurrentPath != "/");
}

void FileManagerWidget::Populate() {
    DatabaseFileSystem* fs = EnsureFileSystem();
    AddressBar->setText(mCurrentPath);

    if (fs == nullptr) {
        DetailsView->clear();
        ListView->clear();
        ViewStack->setCurrentWidget(PlaceholderLabel);
        UpdateNavButtons();
        return;
    }

    // If the current path vanished (e.g. after a project switch), fall back to the root.
    if (mCurrentPath != "/") {
        std::optional<DatabaseFileSystem::Node> here = fs->GetNodeByPath(mCurrentPath.toStdString());
        if (!here || here->Type != DatabaseFileSystem::NodeType::Directory) {
            mCurrentPath = "/";
            AddressBar->setText(mCurrentPath);
        }
    }

    PruneDocumentEditors(fs);

    DetailsView->setSortingEnabled(false);
    DetailsView->clear();
    ListView->clear();

    QString filter = mSearchFilter.trimmed();
    QLocale locale;
    for (const DatabaseFileSystem::Node &node : fs->ListChildrenByPath(mCurrentPath.toStdString())) {
        QString name = QString::fromStdString(node.Name);
        if (!filter.isEmpty() && !name.contains(filter, Qt::CaseInsensitive))
            continue;

        QString typeText = NodeTypeText(node);
        QString dateText = node.ModifiedAt > 0
            ? QDateTime::fromSecsSinceEpoch(node.ModifiedAt).toString("yyyy-MM-dd hh:mm")
            : QString();
        QString sizeText = node.Type == DatabaseFileSystem::NodeType::File
            ? locale.formattedDataSize(static_cast<qint64>(node.Size))
            : QString();
        QIcon icon = NodeIcon(node);

        // Details (tree)
        auto treeItem = new QTreeWidgetItem(DetailsView);
        treeItem->setText(0, name);
        treeItem->setIcon(1, icon);
        treeItem->setText(1, name);
        treeItem->setText(2, dateText);
        treeItem->setText(3, typeText);
        treeItem->setText(4, sizeText);
        // Right-align and sort the size column numerically rather than lexically.
        treeItem->setTextAlignment(4, Qt::AlignRight | Qt::AlignVCenter);
        treeItem->setData(4, Qt::InitialSortOrderRole, static_cast<qulonglong>(node.Size));
        treeItem->setData(0, kIdRole, static_cast<qlonglong>(node.Id));
        treeItem->setData(0, kTypeRole, static_cast<int>(node.Type));

        // List / icons
        auto listItem = new QListWidgetItem(icon, name, ListView);
        listItem->setData(kIdRole, static_cast<qlonglong>(node.Id));
        listItem->setData(kTypeRole, static_cast<int>(node.Type));
    }

    DetailsView->setSortingEnabled(true);
    UpdateNavButtons();

    if (ViewStack->currentWidget() == PlaceholderLabel)
        SetViewMode(mViewMode);
}

bool FileManagerWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == DetailsView->viewport() && event->type() == QEvent::Resize)
        FitDetailsColumns();
    return QDockWidget::eventFilter(watched, event);
}

void FileManagerWidget::FitDetailsColumns() {
    if (mFittingColumns)
        return;

    QHeaderView* header = DetailsView->header();
    const int available = DetailsView->viewport()->width();
    if (available <= 0)
        return;

    std::vector<int> columns;
    int total = 0;
    for (int i = 0; i < header->count(); ++i) {
        if (header->isSectionHidden(i))
            continue;
        columns.push_back(i);
        total += header->sectionSize(i);
    }
    if (columns.empty() || total <= 0 || total == available)
        return;

    const int minimum = header->minimumSectionSize();
    if (available < static_cast<int>(columns.size()) * minimum)
        return;

    mFittingColumns = true;
    int used = 0;
    for (size_t n = 0; n < columns.size(); ++n) {
        int size = n + 1 == columns.size()
            ? available - used
            : qRound(header->sectionSize(columns[n]) * static_cast<double>(available) / total);
        size = qMax(size, minimum);
        header->resizeSection(columns[n], size);
        used += size;
    }
    mFittingColumns = false;
}

void FileManagerWidget::SetViewMode(ViewMode mode) {
    mViewMode = mode;
    if (mode == ViewMode::Details) {
        ViewStack->setCurrentWidget(DetailsView);
        return;
    }

    // The list widget serves the three non-tree modes with different flow/icon settings.
    switch (mode) {
    case ViewMode::List:
        ListView->setViewMode(QListView::ListMode);
        ListView->setFlow(QListView::TopToBottom);
        ListView->setWrapping(false);
        ListView->setIconSize(QSize(16, 16));
        ListView->setGridSize(QSize());
        break;
    case ViewMode::Columns:
        ListView->setViewMode(QListView::ListMode);
        ListView->setFlow(QListView::TopToBottom);
        ListView->setWrapping(true);
        ListView->setIconSize(QSize(16, 16));
        ListView->setGridSize(QSize());
        break;
    case ViewMode::Icons:
        ListView->setViewMode(QListView::IconMode);
        ListView->setFlow(QListView::LeftToRight);
        ListView->setWrapping(true);
        ListView->setIconSize(QSize(48, 48));
        ListView->setGridSize(QSize(80, 80));
        break;
    default:
        break;
    }
    ViewStack->setCurrentWidget(ListView);
}

std::vector<int64_t> FileManagerWidget::SelectedNodeIds() {
    std::vector<int64_t> ids;
    if (ViewStack->currentWidget() == DetailsView) {
        for (QTreeWidgetItem *item : DetailsView->selectedItems())
            ids.push_back(item->data(0, kIdRole).toLongLong());
    } else {
        for (QListWidgetItem *item : ListView->selectedItems())
            ids.push_back(item->data(kIdRole).toLongLong());
    }
    return ids;
}

std::optional<int64_t> FileManagerWidget::CurrentDirId() {
    DatabaseFileSystem* fs = EnsureFileSystem();
    if (fs == nullptr || mCurrentPath == "/")
        return std::nullopt;
    return fs->ResolvePath(mCurrentPath.toStdString());
}

QString FileManagerWidget::NodeTypeText(const DatabaseFileSystem::Node &node) {
    switch (node.Type) {
    case DatabaseFileSystem::NodeType::Directory:
        return "File folder";
    case DatabaseFileSystem::NodeType::Shortcut: {
        if (node.ShortcutItemType)
            return QString("Shortcut (%1)").arg(QString::fromStdString(GetTableNameFromItemType(static_cast<ItemType>(*node.ShortcutItemType))));
        return "Shortcut";
    }
    case DatabaseFileSystem::NodeType::File:
    default:
        return "Document";
    }
}

QIcon FileManagerWidget::NodeIcon(const DatabaseFileSystem::Node &node) {
    switch (node.Type) {
    case DatabaseFileSystem::NodeType::Directory:
        return QIcon(":/images/silk/folder.png");
    case DatabaseFileSystem::NodeType::Shortcut:
        return QIcon(":/images/silk/brick_link.png");
    case DatabaseFileSystem::NodeType::File:
    default:
        return QIcon(":/images/silk/page_white.png");
    }
}

void FileManagerWidget::ActivateNode(int64_t id) {
    DatabaseFileSystem* fs = EnsureFileSystem();
    if (fs == nullptr)
        return;
    std::optional<DatabaseFileSystem::Node> node = fs->GetNode(id);
    if (!node)
        return;

    switch (node->Type) {
    case DatabaseFileSystem::NodeType::Directory:
        NavigateTo(ChildPath(QString::fromStdString(node->Name)));
        break;
    case DatabaseFileSystem::NodeType::Shortcut: {
        if (!node->ShortcutItemType || !node->ShortcutItemId) {
            QMessageBox::warning(this, "Broken Shortcut", "This shortcut has no target.");
            return;
        }
        ItemType type = static_cast<ItemType>(*node->ShortcutItemType);
        if (!fs->GetDatabase()->DoesItemExist(type, *node->ShortcutItemId)) {
            QMessageBox::warning(this, "Broken Shortcut",
                QString("The %1 with id %2 this shortcut points to no longer exists.")
                    .arg(QString::fromStdString(GetTableNameFromItemType(type)))
                    .arg(*node->ShortcutItemId));
            return;
        }
        // ItemDialog asserts that its parent is the Sdk (it resolves the Sdk via parent()), so parent
        // it to our Sdk rather than to this dock widget.
        auto sdk = dynamic_cast<Sdk*>(parent());
        ItemDialog dialog(fs->GetDatabase(), type, *node->ShortcutItemId, sdk);
        dialog.exec();
        break;
    }
    case DatabaseFileSystem::NodeType::File:
        DoOpenDocument(id);
        break;
    }
}

void FileManagerWidget::ShowContextMenu(const QPoint &globalPos, bool onItem) {
    DatabaseFileSystem* fs = EnsureFileSystem();
    if (fs == nullptr)
        return;

    std::vector<int64_t> selected = SelectedNodeIds();
    QMenu menu(this);

    if (onItem && !selected.empty()) {
        std::optional<DatabaseFileSystem::Node> first = fs->GetNode(selected.front());
        bool single = selected.size() == 1;

        QAction* openAct = menu.addAction("Open");
        connect(openAct, &QAction::triggered, this, [this, id = selected.front()]() { ActivateNode(id); });
        openAct->setEnabled(single);

        QAction* downloadAct = menu.addAction(QIcon(":/images/silk/disk.png"), "Download...");
        connect(downloadAct, &QAction::triggered, this, [this, selected]() { DoDownload(selected); });

        menu.addSeparator();
        QAction* cutAct = menu.addAction("Cut");
        connect(cutAct, &QAction::triggered, this, [this, selected]() { DoCopy(selected, true); });
        QAction* copyAct = menu.addAction(QIcon(":/images/silk/page_copy.png"), "Copy");
        connect(copyAct, &QAction::triggered, this, [this, selected]() { DoCopy(selected, false); });

        if (single && first && first->Type == DatabaseFileSystem::NodeType::Directory && !mClipboardIds.empty()) {
            QAction* pasteIntoAct = menu.addAction("Paste Into Folder");
            connect(pasteIntoAct, &QAction::triggered, this, [this, id = selected.front()]() {
                DoPaste(std::optional<int64_t>(id));
            });
        }

        menu.addSeparator();
        QAction* renameAct = menu.addAction("Rename");
        connect(renameAct, &QAction::triggered, this, [this, id = selected.front()]() { DoRename(id); });
        renameAct->setEnabled(single);

        QAction* deleteAct = menu.addAction(QIcon(":/images/silk/cross.png"), "Delete");
        connect(deleteAct, &QAction::triggered, this, [this, selected]() { DoDelete(selected); });

        menu.addSeparator();
        QAction* propsAct = menu.addAction("Properties");
        connect(propsAct, &QAction::triggered, this, [this, id = selected.front()]() { DoProperties(id); });
        propsAct->setEnabled(single);
    } else {
        QMenu* newMenu = menu.addMenu("New");
        QAction* newFolder = newMenu->addAction(QIcon(":/images/silk/folder_add.png"), "New Folder");
        connect(newFolder, &QAction::triggered, this, [this]() { DoNewFolder(); });
        QAction* newDoc = newMenu->addAction(QIcon(":/images/silk/page_white_add.png"), "New Document");
        connect(newDoc, &QAction::triggered, this, [this]() { DoNewDocument(); });
        QAction* newShortcut = newMenu->addAction(QIcon(":/images/silk/brick_link.png"), "New Item Shortcut");
        connect(newShortcut, &QAction::triggered, this, [this]() { DoNewShortcut(); });

        QAction* pasteAct = menu.addAction("Paste");
        connect(pasteAct, &QAction::triggered, this, [this]() { DoPaste(CurrentDirId()); });
        pasteAct->setEnabled(!mClipboardIds.empty());

        menu.addSeparator();

        QMenu* viewMenu = menu.addMenu("View");
        auto addViewMode = [this, viewMenu](const QString &label, ViewMode mode) {
            QAction* act = viewMenu->addAction(label);
            act->setCheckable(true);
            act->setChecked(mViewMode == mode);
            connect(act, &QAction::triggered, this, [this, mode]() { SetViewMode(mode); });
        };
        addViewMode("Details", ViewMode::Details);
        addViewMode("List", ViewMode::List);
        addViewMode("Columns", ViewMode::Columns);
        addViewMode("Icons", ViewMode::Icons);

        QMenu* sortMenu = menu.addMenu("Sort by");
        auto addSort = [this, sortMenu](const QString &label, int column) {
            QAction* act = sortMenu->addAction(label);
            connect(act, &QAction::triggered, this, [this, column]() {
                DetailsView->sortByColumn(column, Qt::AscendingOrder);
            });
        };
        addSort("Name", 1);
        addSort("Date Modified", 2);
        addSort("Type", 3);
        addSort("Size", 4);

        menu.addSeparator();
        QAction* refreshAct = menu.addAction(QIcon(":/images/silk/arrow_refresh.png"), "Refresh");
        connect(refreshAct, &QAction::triggered, this, [this]() { Populate(); });
    }

    menu.exec(globalPos);
}

void FileManagerWidget::DoNewFolder() {
    DatabaseFileSystem* fs = EnsureFileSystem();
    if (fs == nullptr)
        return;
    bool ok = false;
    QString name = QInputDialog::getText(this, "New Folder", "Folder name:", QLineEdit::Normal, "New Folder", &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    if (fs->CreateDirectory(CurrentDirId(), name.trimmed().toStdString()) != VirtualFileSystem::Response::Success) {
        QMessageBox::warning(this, "New Folder", "Could not create the folder (a file with that name may already exist).");
        return;
    }
    Populate();
}

void FileManagerWidget::DoNewDocument() {
    DatabaseFileSystem* fs = EnsureFileSystem();
    if (fs == nullptr)
        return;
    bool ok = false;
    QString name = QInputDialog::getText(this, "New Document", "Document name:", QLineEdit::Normal, "New Document.txt", &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    if (fs->CreateDocument(CurrentDirId(), name.trimmed().toStdString(), {}) != VirtualFileSystem::Response::Success) {
        QMessageBox::warning(this, "New Document", "Could not create the document (a file with that name may already exist).");
        return;
    }
    Populate();
}

void FileManagerWidget::DoNewShortcut() {
    DatabaseFileSystem* fs = EnsureFileSystem();
    if (fs == nullptr)
        return;
    EmuDb* db = fs->GetDatabase();

    // First pick which kind of item the shortcut targets, then pick the item itself.
    QStringList typeNames;
    for (int i = 0; i < ItemTypeCount; i++)
        typeNames << QString::fromStdString(GetTableNameFromItemType(static_cast<ItemType>(i)));

    bool ok = false;
    QString chosenType = QInputDialog::getItem(this, "New Item Shortcut", "Item type:", typeNames, 0, false, &ok);
    if (!ok)
        return;
    ItemType type = static_cast<ItemType>(typeNames.indexOf(chosenType));

    std::optional<int64_t> itemId = ItemOpenSaveDialog::GetOpenId(this, db, type, Roblox::AssetType::None, true);
    if (!itemId)
        return;

    // Default the shortcut's name to the item's own name, falling back to "Type id".
    std::optional<std::string> itemName = db->GetItemName(type, *itemId);
    QString defaultName = itemName && !itemName->empty()
        ? QString::fromStdString(*itemName)
        : QString("%1 %2").arg(chosenType).arg(*itemId);

    QString name = QInputDialog::getText(this, "New Item Shortcut", "Shortcut name:", QLineEdit::Normal, defaultName, &ok);
    if (!ok || name.trimmed().isEmpty())
        return;

    if (fs->CreateShortcut(CurrentDirId(), name.trimmed().toStdString(), static_cast<int>(type), *itemId)
            != VirtualFileSystem::Response::Success) {
        QMessageBox::warning(this, "New Item Shortcut", "Could not create the shortcut (a file with that name may already exist).");
        return;
    }
    Populate();
}

void FileManagerWidget::DoRename(int64_t id) {
    DatabaseFileSystem* fs = EnsureFileSystem();
    if (fs == nullptr)
        return;
    std::optional<DatabaseFileSystem::Node> node = fs->GetNode(id);
    if (!node)
        return;
    bool ok = false;
    QString name = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal,
                                         QString::fromStdString(node->Name), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    if (fs->RenameNode(id, name.trimmed().toStdString()) != VirtualFileSystem::Response::Success) {
        QMessageBox::warning(this, "Rename", "Could not rename (a file with that name may already exist).");
        return;
    }
    if (auto it = mOpenDocuments.find(DocumentKey { mFsDb, id }); it != mOpenDocuments.end() && it->second)
        it->second->SetBaseTitle(name.trimmed());
    Populate();
}

void FileManagerWidget::DoDelete(const std::vector<int64_t> &ids) {
    DatabaseFileSystem* fs = EnsureFileSystem();
    if (fs == nullptr || ids.empty())
        return;
    QString prompt = ids.size() == 1
        ? QString("Delete this item?")
        : QString("Delete these %1 items?").arg(ids.size());
    if (QMessageBox::question(this, "Delete", prompt + "\nFolders are deleted with all of their contents.",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    for (int64_t id : ids)
        fs->DeleteNode(id);
    Populate();
}

void FileManagerWidget::DoCopy(const std::vector<int64_t> &ids, bool cut) {
    if (ids.empty())
        return;
    mClipboardIds = ids;
    mClipboardDb = GetDatabase();
    mClipboardCut = cut;
}

void FileManagerWidget::DoPaste(const std::optional<int64_t> &destDir) {
    DatabaseFileSystem* fs = EnsureFileSystem();
    if (fs == nullptr || mClipboardIds.empty())
        return;
    if (mClipboardDb != GetDatabase()) {
        QMessageBox::information(this, "Paste",
            "The clipboard items belong to a different database. Pasting across databases isn't supported yet.");
        return;
    }

    for (int64_t id : mClipboardIds) {
        if (mClipboardCut)
            fs->MoveNode(id, destDir);
        else
            fs->CopyNode(id, destDir);
    }

    if (mClipboardCut) {
        mClipboardIds.clear();
        mClipboardDb = nullptr;
        mClipboardCut = false;
    }
    Populate();
}

void FileManagerWidget::DoOpenDocument(int64_t id) {
    DatabaseFileSystem* fs = EnsureFileSystem();
    if (fs == nullptr)
        return;
    std::optional<DatabaseFileSystem::Node> node = fs->GetNode(id);
    if (!node)
        return;

    PruneDocumentEditors(fs);

    const DocumentKey key { mFsDb, id };
    if (auto it = mOpenDocuments.find(key); it != mOpenDocuments.end()) {
        FocusEditor(it->second.data());
        return;
    }

    std::vector<unsigned char> content;
    fs->ReadFileContent(id, &content);

    const QString name = QString::fromStdString(node->Name);
    auto container = new SourceEditorContainer();
    container->setAttribute(Qt::WA_DeleteOnClose);
    container->Editor->SetLuaHighlighting(name.endsWith(".lua", Qt::CaseInsensitive) ||
                                          name.endsWith(".luau", Qt::CaseInsensitive));
    container->Editor->setPlainText(QString::fromUtf8(reinterpret_cast<const char*>(content.data()),
                                                     static_cast<int>(content.size())));
    container->Editor->document()->setModified(false);
    container->setWindowIcon(NodeIcon(*node));

    auto sdk = dynamic_cast<Sdk*>(parent());
    Project* project = sdk != nullptr ? sdk->GetFocusedProject() : nullptr;

    QPointer<FileManagerWidget> self(this);
    QPointer<Sdk> sdkGuard(sdk);
    CodeEditorWidget* editor = container->Editor;
    container->OnApply = [self, sdkGuard, project, db = mFsDb, id, editor]() -> bool {
        if (sdkGuard.isNull() || !sdkGuard->IsProjectOpen(project))
            return false;
        QByteArray bytes = editor->toPlainText().toUtf8();
        std::vector<unsigned char> data(bytes.begin(), bytes.end());
        DatabaseFileSystem fs(db);
        if (fs.WriteFileContent(id, data) != VirtualFileSystem::Response::Success) {
            QMessageBox::warning(editor, "Save", "Could not save the document.");
            return false;
        }
        editor->document()->setModified(false);
        if (self)
            self->Populate();
        return true;
    };

    QTabWidget* tabs = project != nullptr ? project->GetTabWidget() : nullptr;
    if (tabs != nullptr) {
        tabs->setCurrentIndex(tabs->addTab(container, NodeIcon(*node), name));
        container->SetBaseTitle(name);
    } else {
        container->setWindowFlag(Qt::Window);
        container->SetBaseTitle(name);
        container->resize(560, 420);
        container->show();
    }

    mOpenDocuments[key] = container;
}

void FileManagerWidget::PruneDocumentEditors(DatabaseFileSystem* fs) {
    for (auto it = mOpenDocuments.begin(); it != mOpenDocuments.end();) {
        if (it->second.isNull()) {
            it = mOpenDocuments.erase(it);
            continue;
        }
        if (fs != nullptr && it->first.first == mFsDb && !fs->GetNode(it->first.second)) {
            SourceEditorContainer* editor = it->second.data();
            it = mOpenDocuments.erase(it);
            editor->ForceClose();
            continue;
        }
        ++it;
    }
}

void FileManagerWidget::DoProperties(int64_t id) {
    DatabaseFileSystem* fs = EnsureFileSystem();
    if (fs == nullptr)
        return;
    std::optional<DatabaseFileSystem::Node> node = fs->GetNode(id);
    if (!node)
        return;

    QLocale locale;
    QString details;
    details += QString("Name:\t%1\n").arg(QString::fromStdString(node->Name));
    details += QString("Type:\t%1\n").arg(NodeTypeText(*node));
    details += QString("Location:\t%1\n").arg(mCurrentPath);
    if (node->Type == DatabaseFileSystem::NodeType::File)
        details += QString("Size:\t%1 (%2 bytes)\n").arg(locale.formattedDataSize(static_cast<qint64>(node->Size))).arg(node->Size);
    if (node->Type == DatabaseFileSystem::NodeType::Shortcut && node->ShortcutItemType && node->ShortcutItemId) {
        ItemType t = static_cast<ItemType>(*node->ShortcutItemType);
        details += QString("Target:\t%1 #%2\n").arg(QString::fromStdString(GetTableNameFromItemType(t))).arg(*node->ShortcutItemId);
        std::optional<std::string> itemName = fs->GetDatabase()->GetItemName(t, *node->ShortcutItemId);
        if (itemName)
            details += QString("Target name:\t%1\n").arg(QString::fromStdString(*itemName));
    }
    if (node->CreatedAt > 0)
        details += QString("Created:\t%1\n").arg(QDateTime::fromSecsSinceEpoch(node->CreatedAt).toString("yyyy-MM-dd hh:mm:ss"));
    if (node->ModifiedAt > 0)
        details += QString("Modified:\t%1\n").arg(QDateTime::fromSecsSinceEpoch(node->ModifiedAt).toString("yyyy-MM-dd hh:mm:ss"));

    QMessageBox box(this);
    box.setWindowTitle(QString("%1 Properties").arg(QString::fromStdString(node->Name)));
    box.setIcon(QMessageBox::NoIcon);
    box.setIconPixmap(NodeIcon(*node).pixmap(32, 32));
    box.setText(details);
    box.exec();
}

void FileManagerWidget::ExportNodeToDisk(int64_t id, const QString &destDir) {
    DatabaseFileSystem* fs = EnsureFileSystem();
    if (fs == nullptr)
        return;
    std::optional<DatabaseFileSystem::Node> node = fs->GetNode(id);
    if (!node)
        return;

    QString target = QDir(destDir).filePath(QString::fromStdString(node->Name));

    switch (node->Type) {
    case DatabaseFileSystem::NodeType::Directory: {
        QDir().mkpath(target);
        for (const DatabaseFileSystem::Node &child : fs->ListChildren(id))
            ExportNodeToDisk(child.Id, target);
        break;
    }
    case DatabaseFileSystem::NodeType::File: {
        std::vector<unsigned char> content;
        fs->ReadFileContent(id, &content);
        std::ofstream out(target.toStdString(), std::ios::binary | std::ios::trunc);
        if (!content.empty())
            out.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
        break;
    }
    case DatabaseFileSystem::NodeType::Shortcut:
        // Shortcuts are database-internal references; there's nothing to write to disk.
        break;
    }
}

void FileManagerWidget::DoDownload(const std::vector<int64_t> &ids) {
    DatabaseFileSystem* fs = EnsureFileSystem();
    if (fs == nullptr || ids.empty())
        return;

    // A single document downloads via a plain "Save File" dialog; anything else (a folder, or several
    // items) downloads into a chosen destination folder, mirroring the database tree on disk.
    if (ids.size() == 1) {
        std::optional<DatabaseFileSystem::Node> node = fs->GetNode(ids.front());
        if (node && node->Type == DatabaseFileSystem::NodeType::File) {
            QString path = QFileDialog::getSaveFileName(this, "Download Document", QString::fromStdString(node->Name));
            if (path.isEmpty())
                return;
            std::vector<unsigned char> content;
            fs->ReadFileContent(ids.front(), &content);
            std::ofstream out(path.toStdString(), std::ios::binary | std::ios::trunc);
            if (!content.empty())
                out.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
            return;
        }
        if (node && node->Type == DatabaseFileSystem::NodeType::Shortcut) {
            QMessageBox::information(this, "Download", "Shortcuts point at items inside the database and can't be downloaded as files.");
            return;
        }
    }

    QString destDir = QFileDialog::getExistingDirectory(this, "Download To Folder");
    if (destDir.isEmpty())
        return;
    for (int64_t id : ids)
        ExportNodeToDisk(id, destDir);
}
