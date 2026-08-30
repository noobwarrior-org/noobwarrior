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
// File: ExplorerWidget.cpp
// Started by: Hattozo
// Started on: 8/29/2026
// Description: A poor replication of Roblox Studio's Explorer widget
#include "ExplorerWidget.h"

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlRobloxFile.h>

#include <QFile>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QShortcut>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QDropEvent>
#include <QItemSelectionModel>
#include <QTabWidget>

#include <optional>

#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
#include <NoobWarrior/Roblox/FileFormat/Generated/EnumItems.h>
#include <NoobWarrior/Roblox/FileFormat/Generated/Registry.h>
#endif

using namespace NoobWarrior;

class ExplorerTree : public QTreeWidget {
public:
    using QTreeWidget::QTreeWidget;
    std::function<void(QTreeWidgetItem* target)> OnDropRequested;
    // True for scripts: double-click opens an editor instead of toggling expansion.
    std::function<bool(QTreeWidgetItem* item)> SuppressExpandToggleFor;
protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override {
        QTreeWidgetItem* item = itemAt(event->position().toPoint());
        if (item != nullptr && SuppressExpandToggleFor && SuppressExpandToggleFor(item)) {
            emit itemDoubleClicked(item, 0); // activation only; skip the base's expand toggle
            return;
        }
        QTreeWidget::mouseDoubleClickEvent(event);
    }
    void dropEvent(QDropEvent* event) override {
        QTreeWidgetItem* target = itemAt(event->position().toPoint());
        if (target == nullptr || !OnDropRequested) {
            event->ignore();
            return;
        }
        DropIndicatorPosition indicator = dropIndicatorPosition();
        if (indicator == QAbstractItemView::AboveItem || indicator == QAbstractItemView::BelowItem)
            target = target->parent();
        if (target == nullptr) {
            event->ignore();
            return;
        }
        OnDropRequested(target);
        event->setDropAction(Qt::IgnoreAction); // the callback moved everything itself
        event->accept();
    }
};

// Detached copy of a subtree (clipboard/duplication). Ref-typed values point into the source
// document and are dropped.
struct InstanceSnapshot {
    std::string ClassName;
    std::string Name;
    std::map<std::string, Roblox::Property> Properties;
    std::vector<InstanceSnapshot> Children;
};

static InstanceSnapshot SnapshotSubtree(Roblox::Instance* instance) {
    InstanceSnapshot snap;
    snap.ClassName = instance->ClassName;
    snap.Name = instance->Name;
    for (const auto& [name, prop] : instance->GetProperties()) {
        if (prop.CastValue<Roblox::Instance*>() != nullptr)
            continue; // Ref into the source document
        snap.Properties.emplace(name, prop);
    }
    for (Roblox::Instance* child : instance->GetChildren())
        snap.Children.push_back(SnapshotSubtree(child));
    return snap;
}

static Roblox::Instance* MaterializeSnapshot(Roblox::RobloxFile* file, const InstanceSnapshot& snap,
                                      Roblox::Instance* parent) {
    Roblox::Instance* instance = file->CreateInstance(snap.ClassName, snap.Name, parent);
    if (instance == nullptr)
        return nullptr;
    for (const auto& [name, prop] : snap.Properties) {
        Roblox::Property copy = prop;
        copy.File = nullptr; // stale pointer into the snapshot's source document
        copy.RawBuffer.clear(); // the source file's encoding; '<' would become a CFrame orient id
        instance->AddProperty(copy); // re-points Property::Object at the new instance
    }
    instance->SetName(snap.Name); // keep the field and the serialized Name property agreeing
    for (const InstanceSnapshot& child : snap.Children)
        MaterializeSnapshot(file, child, instance);
    return instance;
}

// The explorer clipboard: shared across previews so Copy in one and Paste Into in another works.
static std::vector<InstanceSnapshot> sExplorerClipboard;

static bool IsFileRoot(Roblox::Instance* instance) {
    return dynamic_cast<Roblox::RobloxFile*>(instance) != nullptr;
}

// Source is a ProtectedString in XML but a plain String column in binary files.
static std::optional<std::string> ScriptSourceFromProperty(const Roblox::Property& prop) {
    if (const auto* ps = prop.CastValue<Roblox::DataTypes::ProtectedString>())
        return ps->ToString();
    if (const auto* s = prop.CastValue<std::string>())
        return *s;
    return std::nullopt;
}

static bool IsScriptSourceProperty(const std::string& propName, const Roblox::Property& prop) {
    return propName == "Source" && ScriptSourceFromProperty(prop).has_value();
}

QIcon NoobWarrior::StudioIconForClassName(const std::string& className, bool isService) {
    std::string current = className;
    for (int depth = 0; depth < 16 && !current.empty(); depth++) {
        QString path = ":/images/studio/" + QString::fromStdString(current) + ".png";
        if (QFile::exists(path))
            return QIcon(path);
#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
        const Roblox::ClassDescriptor* desc = Roblox::FindClass(current);
        current = desc != nullptr ? std::string(desc->Superclass) : std::string();
#else
        break;
#endif
    }
    return QIcon(":/images/silk/shape_square.png");
}

ExplorerWidget::ExplorerWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    mFilter = new QLineEdit(this);
    mFilter->setPlaceholderText("Filter workspace (Ctrl+Shift+X)");
    mFilter->setClearButtonEnabled(true);
    layout->addWidget(mFilter);

    auto* tree = new ExplorerTree(this);
    mTree = tree;
    mTree->setColumnCount(1);
    mTree->header()->hide();
    mTree->setSelectionMode(QAbstractItemView::ExtendedSelection); // Group/Delete work on many
    mTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTree->setUniformRowHeights(true);
    mTree->setIndentation(14);
    mTree->setStyleSheet("QTreeWidget { border: none; }");
    mTree->setContextMenuPolicy(Qt::CustomContextMenu);
    // Drag to reparent; services never get the drag flag (AddInstanceRecursive).
    mTree->setDragEnabled(true);
    mTree->setAcceptDrops(true);
    mTree->setDropIndicatorShown(true);
    mTree->setDragDropMode(QAbstractItemView::InternalMove);
    tree->SuppressExpandToggleFor = [this](QTreeWidgetItem* item) {
        Roblox::Instance* instance = InstanceForItem(item);
        return instance != nullptr && instance->GetProperty("Source") != nullptr;
    };
    tree->OnDropRequested = [this](QTreeWidgetItem* targetItem) {
        Roblox::Instance* targetInstance = InstanceForItem(targetItem);
        if (targetInstance == nullptr)
            return;
        Roblox::RobloxFile* targetFile = FileForInstance(targetInstance);
        bool moved = false;
        for (QTreeWidgetItem* item : TopMostSelectedItems()) {
            Roblox::Instance* instance = InstanceForItem(item);
            if (instance == nullptr || instance->IsService || IsFileRoot(instance) ||
                item == targetItem)
                continue;
            // Cross-document moves would break ownership.
            if (FileForInstance(instance) != targetFile)
                continue;
            // Never reparent something into its own subtree.
            bool targetInsideDragged = false;
            for (QTreeWidgetItem* p = targetItem; p != nullptr; p = p->parent()) {
                if (p == item) {
                    targetInsideDragged = true;
                    break;
                }
            }
            if (targetInsideDragged)
                continue;
            if (!instance->SetParent(targetInstance))
                continue;
            if (QTreeWidgetItem* oldParent = item->parent())
                oldParent->removeChild(item);
            else
                mTree->takeTopLevelItem(mTree->indexOfTopLevelItem(item));
            targetItem->addChild(item);
            moved = true;
        }
        if (moved) {
            targetItem->setExpanded(true);
            emit TreeEdited(targetFile);
        }
    };
    layout->addWidget(mTree, 1);

    connect(mFilter, &QLineEdit::textChanged, this, &ExplorerWidget::ApplyFilter);
    auto* focusFilter = new QShortcut(QKeySequence("Ctrl+Shift+X"), this);
    focusFilter->setContext(Qt::WidgetWithChildrenShortcut);
    connect(focusFilter, &QShortcut::activated, [this]() { mFilter->setFocus(); });

    connect(mTree, &QTreeWidget::itemSelectionChanged,
            this, &ExplorerWidget::EmitSelectionChanged);
    connect(mTree, &QTreeWidget::itemDoubleClicked, [this](QTreeWidgetItem* item, int) {
        if (Roblox::Instance* instance = InstanceForItem(item))
            emit InstanceDoubleClicked(instance);
    });
    connect(mTree, &QTreeWidget::customContextMenuRequested, this,
            &ExplorerWidget::ShowContextMenu);
}

Roblox::Instance* ExplorerWidget::InstanceForItem(QTreeWidgetItem* item) const {
    if (item == nullptr)
        return nullptr;
    return static_cast<Roblox::Instance*>(item->data(0, Qt::UserRole).value<void*>());
}

QList<QTreeWidgetItem*> ExplorerWidget::TopMostSelectedItems() const {
    // Drop items whose ancestor is also selected so subtree operations never run twice.
    QList<QTreeWidgetItem*> selected = mTree->selectedItems();
    QList<QTreeWidgetItem*> result;
    for (QTreeWidgetItem* item : selected) {
        bool ancestorSelected = false;
        for (QTreeWidgetItem* p = item->parent(); p != nullptr; p = p->parent()) {
            if (selected.contains(p)) {
                ancestorSelected = true;
                break;
            }
        }
        if (!ancestorSelected)
            result.append(item);
    }
    return result;
}

QTreeWidgetItem* ExplorerWidget::AddInstanceRecursive(Roblox::Instance* instance,
                                                              QTreeWidgetItem* parentItem) {
    auto* item = parentItem != nullptr ? new QTreeWidgetItem(parentItem)
                                       : new QTreeWidgetItem(mTree);
    item->setText(0, QString::fromStdString(instance->Name.empty() ? instance->ClassName : instance->Name));
    item->setIcon(0, StudioIconForClassName(instance->ClassName, instance->IsService));
    item->setToolTip(0, QString::fromStdString(instance->ClassName));
    item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void*>(instance)));
    // Anything can receive drops; services never move themselves.
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
    if (!instance->IsService)
        flags |= Qt::ItemIsDragEnabled;
    item->setFlags(flags);

    for (Roblox::Instance* child : instance->GetChildren())
        AddInstanceRecursive(child, item);
    return item;
}

// Root items keep their base title at Qt::UserRole + 1.
void ExplorerWidget::AddFile(Roblox::RobloxFile* file, const QString& title) {
    if (file == nullptr || RootItemForFile(file) != nullptr)
        return;
    mFiles.push_back(file);
    auto* root = new QTreeWidgetItem(mTree);
    root->setText(0, title);
    root->setData(0, Qt::UserRole,
                  QVariant::fromValue(static_cast<void*>(static_cast<Roblox::Instance*>(file))));
    root->setData(0, Qt::UserRole + 1, title);
    root->setIcon(0, StudioIconForClassName("DataModel", true));
    root->setToolTip(0, "File");
    root->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled);
    for (Roblox::Instance* child : file->GetChildren())
        AddInstanceRecursive(child, root);
    root->setExpanded(true);
}

void ExplorerWidget::RemoveFile(Roblox::RobloxFile* file) {
    if (QTreeWidgetItem* root = RootItemForFile(file))
        delete root;
    std::erase(mFiles, file);
}

void ExplorerWidget::FocusFile(Roblox::RobloxFile* file) {
    QTreeWidgetItem* root = RootItemForFile(file);
    if (root == nullptr)
        return;
    mTree->clearSelection();
    root->setSelected(true);
    mTree->setCurrentItem(root, 0, QItemSelectionModel::NoUpdate);
    mTree->scrollToItem(root);
}

void ExplorerWidget::SetFileDirty(Roblox::RobloxFile* file, bool dirty) {
    if (QTreeWidgetItem* root = RootItemForFile(file))
        root->setText(0, root->data(0, Qt::UserRole + 1).toString() + (dirty ? "*" : ""));
}

void ExplorerWidget::RefreshFileNode(Roblox::RobloxFile* file) {
    QTreeWidgetItem* root = RootItemForFile(file);
    if (root == nullptr)
        return;
    while (root->childCount() > 0)
        delete root->takeChild(0);
    for (Roblox::Instance* child : file->GetChildren())
        AddInstanceRecursive(child, root);
    root->setExpanded(true);
}

QTreeWidgetItem* ExplorerWidget::RootItemForFile(Roblox::RobloxFile* file) const {
    for (int i = 0; i < mTree->topLevelItemCount(); i++) {
        QTreeWidgetItem* item = mTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).value<void*>() ==
            static_cast<void*>(static_cast<Roblox::Instance*>(file)))
            return item;
    }
    return nullptr;
}

Roblox::RobloxFile* ExplorerWidget::FileForItem(QTreeWidgetItem* item) const {
    while (item != nullptr && item->parent() != nullptr)
        item = item->parent();
    return dynamic_cast<Roblox::RobloxFile*>(InstanceForItem(item));
}

Roblox::RobloxFile* ExplorerWidget::FileForInstance(Roblox::Instance* instance) const {
    for (Roblox::Instance* p = instance; p != nullptr; p = p->GetParent())
        if (auto* file = dynamic_cast<Roblox::RobloxFile*>(p))
            return file;
    return nullptr;
}

Roblox::RobloxFile* ExplorerWidget::ActiveFile() const {
    if (Roblox::RobloxFile* file = FileForItem(mTree->currentItem()))
        return file;
    return mFiles.size() == 1 ? mFiles.front() : nullptr;
}

void ExplorerWidget::EmitSelectionChanged() {
    std::vector<Roblox::Instance*> instances;
    for (QTreeWidgetItem* item : mTree->selectedItems())
        if (Roblox::Instance* instance = InstanceForItem(item))
            if (!IsFileRoot(instance)) // file nodes have no properties to show
                instances.push_back(instance);
    emit SelectionChanged(instances);
}

void ExplorerWidget::RefreshInstanceLabel(Roblox::Instance* instance) {
    std::function<bool(QTreeWidgetItem*)> visit = [&](QTreeWidgetItem* item) -> bool {
        if (static_cast<Roblox::Instance*>(item->data(0, Qt::UserRole).value<void*>()) == instance) {
            item->setText(0, QString::fromStdString(instance->Name.empty() ? instance->ClassName
                                                                           : instance->Name));
            return true;
        }
        for (int i = 0; i < item->childCount(); i++)
            if (visit(item->child(i)))
                return true;
        return false;
    };
    for (int i = 0; i < mTree->topLevelItemCount(); i++)
        if (visit(mTree->topLevelItem(i)))
            return;
}

void ExplorerWidget::ShowContextMenu(const QPoint& point) {
    QTreeWidgetItem* clicked = mTree->itemAt(point);
    if (clicked == nullptr) {
        if (!mOpenMenuEnabled)
            return;
        QMenu openMenu;
        QAction* fromDb = openMenu.addAction(QIcon(":/images/silk/database_gear.png"),
                                             "Open from Database...");
        connect(fromDb, &QAction::triggered, [this]() { emit OpenFromDatabaseRequested(); });
        QAction* fromFile = openMenu.addAction(QIcon(":/images/silk/folder_page.png"),
                                               "Open from File...");
        connect(fromFile, &QAction::triggered, [this]() { emit OpenFromFileRequested(); });
        openMenu.exec(mTree->mapToGlobal(point));
        return;
    }
    Roblox::Instance* instance = InstanceForItem(clicked);
    if (instance == nullptr)
        return;
    Roblox::RobloxFile* clickedFile = FileForItem(clicked);
    if (clickedFile == nullptr)
        return;

    // Right-clicking outside the selection retargets it.
    QList<QTreeWidgetItem*> targets = TopMostSelectedItems();
    if (!targets.contains(clicked)) {
        mTree->clearSelection();
        clicked->setSelected(true);
        mTree->setCurrentItem(clicked);
        targets = {clicked};
    }
    // Services and file roots are fixtures: never cut/copied/deleted/duplicated.
    targets.removeIf([this](QTreeWidgetItem* item) {
        Roblox::Instance* inst = InstanceForItem(item);
        return inst == nullptr || inst->IsService || IsFileRoot(inst);
    });

    auto snapshotTargets = [&]() {
        std::vector<InstanceSnapshot> snaps;
        for (QTreeWidgetItem* item : targets)
            if (Roblox::Instance* inst = InstanceForItem(item))
                snaps.push_back(SnapshotSubtree(inst));
        return snaps;
    };
    auto deleteTargets = [&]() {
        for (QTreeWidgetItem* item : targets) {
            if (Roblox::Instance* inst = InstanceForItem(item)) {
                Roblox::RobloxFile* file = FileForInstance(inst);
                emit SubtreeRemoved(inst); // while the subtree is still walkable
                file->DestroySubtree(inst);
                delete item;
                emit TreeEdited(file);
            } else {
                delete item;
            }
        }
    };
    auto insertClass = [&](const std::string& className) {
        Roblox::Instance* created = clickedFile->CreateInstance(className, className, instance);
        if (created == nullptr)
            return;
        AddInstanceRecursive(created, clicked);
        clicked->setExpanded(true);
        emit TreeEdited(clickedFile);
    };

    QMenu menu;

    if (IsFileRoot(instance)) {
        QAction* saveFile = menu.addAction(QIcon(":/images/silk/disk.png"), "Save File");
        connect(saveFile, &QAction::triggered,
                [this, clickedFile]() { emit FileSaveRequested(clickedFile); });
        QAction* closeFile = menu.addAction(QIcon(":/images/silk/cross.png"), "Close File");
        connect(closeFile, &QAction::triggered,
                [this, clickedFile]() { emit FileCloseRequested(clickedFile); });
        menu.addSeparator();
    }

    QAction* cut = menu.addAction(QIcon(":/images/silk/cut.png"), "Cut");
    cut->setEnabled(!targets.isEmpty());
    connect(cut, &QAction::triggered, [&]() {
        sExplorerClipboard = snapshotTargets();
        deleteTargets();
    });
    QAction* copy = menu.addAction(QIcon(":/images/silk/page_copy.png"), "Copy");
    copy->setEnabled(!targets.isEmpty());
    connect(copy, &QAction::triggered, [&]() { sExplorerClipboard = snapshotTargets(); });
    QAction* pasteInto = menu.addAction(QIcon(":/images/silk/paste_plain.png"), "Paste Into");
    pasteInto->setEnabled(!sExplorerClipboard.empty());
    connect(pasteInto, &QAction::triggered, [&]() {
        QList<QTreeWidgetItem*> pastedItems;
        for (const InstanceSnapshot& snap : sExplorerClipboard)
            if (Roblox::Instance* pasted = MaterializeSnapshot(clickedFile, snap, instance))
                pastedItems.append(AddInstanceRecursive(pasted, clicked));
        clicked->setExpanded(true);
        mTree->clearSelection();
        for (QTreeWidgetItem* pastedItem : pastedItems)
            pastedItem->setSelected(true);
        if (!pastedItems.isEmpty())
            mTree->setCurrentItem(pastedItems.first(), 0, QItemSelectionModel::NoUpdate);
        emit TreeEdited(clickedFile);
    });
    QAction* duplicate = menu.addAction(QIcon(":/images/silk/page_white_copy.png"), "Duplicate");
    duplicate->setEnabled(!targets.isEmpty());
    connect(duplicate, &QAction::triggered, [&]() {
        for (QTreeWidgetItem* item : targets) {
            Roblox::Instance* inst = InstanceForItem(item);
            if (inst == nullptr)
                continue;
            Roblox::RobloxFile* file = FileForInstance(inst);
            InstanceSnapshot snap = SnapshotSubtree(inst);
            if (Roblox::Instance* copyInst = MaterializeSnapshot(file, snap, inst->GetParent())) {
                AddInstanceRecursive(copyInst, item->parent());
                emit TreeEdited(file);
            }
        }
    });
    QAction* del = menu.addAction(QIcon(":/images/silk/cross.png"), "Delete");
    del->setEnabled(!targets.isEmpty());
    connect(del, &QAction::triggered, [&]() { deleteTargets(); });
    QAction* rename = menu.addAction(QIcon(":/images/silk/pencil.png"), "Rename");
    rename->setEnabled(!instance->IsService && !IsFileRoot(instance) && targets.size() == 1);
    connect(rename, &QAction::triggered, [&]() {
        bool ok = false;
        QString newName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal,
                                                QString::fromStdString(instance->Name), &ok);
        if (!ok)
            return;
        instance->SetName(newName.toStdString());
        clicked->setText(0, newName);
        emit TreeEdited(clickedFile);
        EmitSelectionChanged();
    });

    menu.addSeparator();

    QAction* group = menu.addAction(QIcon(":/images/silk/bricks.png"), "Group");
    group->setEnabled(!targets.isEmpty());
    connect(group, &QAction::triggered, [&]() {
        Roblox::Instance* first = InstanceForItem(targets.first());
        if (first == nullptr)
            return;
        Roblox::RobloxFile* file = FileForInstance(first);
        Roblox::Instance* model = file->CreateInstance("Model", "Model", first->GetParent());
        if (model == nullptr)
            return;
        for (QTreeWidgetItem* item : targets) {
            Roblox::Instance* inst = InstanceForItem(item);
            if (inst != nullptr && FileForInstance(inst) == file) // never group across documents
                inst->SetParent(model);
        }
        RefreshFileNode(file); // structural change: rebuild wholesale
        emit TreeEdited(file);
    });
    QAction* ungroup = menu.addAction(QIcon(":/images/silk/bricks.png"), "Ungroup");
    ungroup->setEnabled(targets.size() == 1 &&
                        (instance->ClassName == "Model" || instance->ClassName == "Folder"));
    connect(ungroup, &QAction::triggered, [&]() {
        // Reparent the children out FIRST, DestroySubtree would take them down with the shell.
        std::vector<Roblox::Instance*> children = instance->GetChildren();
        for (Roblox::Instance* child : children)
            child->SetParent(instance->GetParent());
        emit SubtreeRemoved(instance); // just the emptied shell by now
        clickedFile->DestroySubtree(instance);
        RefreshFileNode(clickedFile);
        emit TreeEdited(clickedFile);
    });
    QAction* selectChildren = menu.addAction("Select Children");
    selectChildren->setEnabled(clicked->childCount() > 0);
    connect(selectChildren, &QAction::triggered, [&]() {
        clicked->setExpanded(true);
        mTree->clearSelection();
        for (int i = 0; i < clicked->childCount(); i++)
            clicked->child(i)->setSelected(true);
        if (clicked->childCount() > 0)
            mTree->setCurrentItem(clicked->child(0));
    });

    menu.addSeparator();

    QAction* insertPart = menu.addAction(StudioIconForClassName("Part", false), "Insert Part");
    connect(insertPart, &QAction::triggered, [&]() { insertClass("Part"); });
    QMenu* insertObject = menu.addMenu("Insert Object");
    static const char* kInsertableClasses[] = {
        "Part", "WedgePart", "CornerWedgePart", "TrussPart", "MeshPart", "SpawnLocation", "Seat",
        "VehicleSeat", "Model", "Folder", "Script", "LocalScript", "ModuleScript", "Sound",
        "Decal", "Texture", "PointLight", "SpotLight", "SurfaceLight", "Humanoid", "Camera",
        "RemoteEvent", "RemoteFunction", "BindableEvent", "BindableFunction", "StringValue",
        "IntValue", "NumberValue", "BoolValue", "ObjectValue", "Attachment", "WeldConstraint",
        "ClickDetector", "Tool", "Accessory", "ScreenGui", "SurfaceGui", "BillboardGui", "Frame",
        "TextLabel", "TextButton", "ImageLabel",
    };
    for (const char* className : kInsertableClasses) {
        QAction* entry = insertObject->addAction(StudioIconForClassName(className, false), className);
        connect(entry, &QAction::triggered, [&insertClass, className]() { insertClass(className); });
    }
    QAction* insertFromFile = menu.addAction(QIcon(":/images/silk/folder_page.png"), "Insert from File...");
    connect(insertFromFile, &QAction::triggered, [&]() {
        QString path = QFileDialog::getOpenFileName(this, "Insert from File", QString(),
            "Roblox files (*.rbxm *.rbxmx *.rbxl *.rbxlx);;All files (*.*)");
        if (path.isEmpty())
            return;
        std::unique_ptr<Roblox::RobloxFile> imported;
        if (Roblox::RobloxFile::Open(imported, path.toStdString()) != Roblox::FileResponse::Success ||
            imported == nullptr) {
            QMessageBox::warning(this, "Insert from File",
                QString("\"%1\" could not be parsed as a Roblox place or model.").arg(path));
            return;
        }
        for (Roblox::Instance* child : imported->GetChildren()) {
            InstanceSnapshot snap = SnapshotSubtree(child);
            if (Roblox::Instance* pasted = MaterializeSnapshot(clickedFile, snap, instance))
                AddInstanceRecursive(pasted, clicked);
        }
        clicked->setExpanded(true);
        emit TreeEdited(clickedFile);
    });

    menu.addSeparator();

    QAction* saveToFile = menu.addAction(QIcon(":/images/silk/disk.png"), "Save to File...");
    saveToFile->setEnabled(!targets.isEmpty());
    connect(saveToFile, &QAction::triggered, [&]() {
        QString path = QFileDialog::getSaveFileName(this, "Save to File", QString(),
            "Roblox XML Model (*.rbxmx);;Roblox Binary Model (*.rbxm)");
        if (path.isEmpty())
            return;
        // The writer is picked by extension; Save() is virtual per format.
        std::unique_ptr<Roblox::RobloxFile> outFile;
        if (path.endsWith(".rbxm", Qt::CaseInsensitive))
            outFile = std::make_unique<Roblox::BinaryFormat::BinaryRobloxFile>();
        else
            outFile = std::make_unique<Roblox::XmlFormat::XmlRobloxFile>();
        for (QTreeWidgetItem* item : targets) {
            if (Roblox::Instance* inst = InstanceForItem(item)) {
                InstanceSnapshot snap = SnapshotSubtree(inst);
                MaterializeSnapshot(outFile.get(), snap, outFile.get());
            }
        }
        if (outFile->Save(std::string_view(path.toStdString())) != Roblox::FileResponse::Success)
            QMessageBox::warning(this, "Save to File",
                QString("Could not save to \"%1\".\n%2").arg(path,
                    QString::fromStdString(outFile->GetLastError())));
    });

    menu.exec(mTree->mapToGlobal(point));
}

void ExplorerWidget::ApplyFilter(const QString& text) {
    const QString needle = text.trimmed();

    std::function<bool(QTreeWidgetItem*)> visit = [&](QTreeWidgetItem* item) -> bool {
        bool selfMatch = needle.isEmpty() ||
            item->text(0).contains(needle, Qt::CaseInsensitive) ||
            item->toolTip(0).contains(needle, Qt::CaseInsensitive);
        bool childMatch = false;
        for (int i = 0; i < item->childCount(); i++)
            childMatch = visit(item->child(i)) || childMatch;
        item->setHidden(!(selfMatch || childMatch));
        if (childMatch && !needle.isEmpty())
            item->setExpanded(true);
        return selfMatch || childMatch;
    };
    for (int i = 0; i < mTree->topLevelItemCount(); i++)
        visit(mTree->topLevelItem(i));
    if (needle.isEmpty())
        mTree->expandToDepth(0);
}

void ExplorerWidget::OpenSourceEditorFor(Roblox::Instance* instance) {
    if (instance == nullptr)
        return;
    OpenSourceEditor(instance, "Source");
}

void ExplorerWidget::OpenSourceEditor(Roblox::Instance* instance, const std::string& propName) {
    Roblox::Property* prop = instance->GetProperty(propName);
    std::optional<std::string> source = prop != nullptr ? ScriptSourceFromProperty(*prop)
                                                        : std::nullopt;
    if (!source)
        return;

    std::erase_if(mSourceEditors, [](const auto& entry) { return entry.second.isNull(); });

    // One editor per script: re-opening refocuses instead of duplicating.
    if (auto it = mSourceEditors.find(instance); it != mSourceEditors.end() && it->second) {
        SourceEditorContainer* existing = it->second.data();
        if (existing->isWindow()) {
            existing->raise();
            existing->activateWindow();
        } else {
            // Every enclosing tab widget, or the tab ends up selected inside a hidden page.
            QWidget* page = existing;
            for (QWidget* w = existing->parentWidget(); w != nullptr; w = w->parentWidget()) {
                if (auto* tabs = qobject_cast<QTabWidget*>(w)) {
                    const int index = tabs->indexOf(page);
                    if (index >= 0)
                        tabs->setCurrentIndex(index);
                    page = tabs;
                }
            }
        }
        return;
    }

    const QString title =
        QString::fromStdString(instance->Name.empty() ? instance->ClassName : instance->Name);

    auto* container = new SourceEditorContainer();
    container->setAttribute(Qt::WA_DeleteOnClose);
    container->Editor->setPlainText(QString::fromStdString(*source));
    container->Editor->document()->setModified(false);

    connect(this, &QObject::destroyed, container, [container]() { container->ForceClose(); });

    QPointer<ExplorerWidget> self(this);
    CodeEditorWidget* editor = container->Editor;
    container->OnApply = [self, instance, propName, editor]() -> bool {
        Roblox::Property* liveProp = instance->GetProperty(propName);
        if (liveProp == nullptr)
            return false;
        // Write back in whatever shape the file stored it, the two formats disagree.
        std::string newText = editor->toPlainText().toStdString();
        if (liveProp->CastValue<Roblox::DataTypes::ProtectedString>() != nullptr)
            liveProp->Value = Roblox::DataTypes::ProtectedString(newText);
        else if (liveProp->CastValue<std::string>() != nullptr)
            liveProp->Value = std::move(newText);
        else
            return false;
        liveProp->RawBuffer.clear();
        editor->document()->setModified(false);
        if (self) {
            emit self->TreeEdited(self->FileForInstance(instance));
            self->EmitSelectionChanged(); // refreshes the Properties view's Source row
        }
        return true;
    };

    mSourceEditors[instance] = container;

    container->setWindowIcon(StudioIconForClassName(instance->ClassName, false));

    if (mSourceEditorHost && mSourceEditorHost(container, title)) {
        container->SetBaseTitle(title);
        return;
    }

    container->setWindowFlag(Qt::Window);
    container->SetBaseTitle(title);
    container->resize(720, 540);
    container->show();
}

void ExplorerWidget::CloseAllSourceEditors() {
    for (auto& [instance, editor] : mSourceEditors)
        if (!editor.isNull())
            editor->ForceClose();
    mSourceEditors.clear();
}

bool ExplorerWidget::HasUnappliedSourceEdits() const {
    for (const auto& [instance, editor] : mSourceEditors)
        if (!editor.isNull() && editor->IsDirty())
            return true;
    return false;
}

bool ExplorerWidget::HasUnappliedSourceEditsUnder(Roblox::Instance* subtreeRoot) const {
    for (const auto& [instance, editor] : mSourceEditors) {
        if (editor.isNull() || !editor->IsDirty())
            continue;
        for (Roblox::Instance* p = instance; p != nullptr; p = p->GetParent())
            if (p == subtreeRoot)
                return true;
    }
    return false;
}

void ExplorerWidget::CloseSourceEditorsUnder(Roblox::Instance* subtreeRoot) {
    if (subtreeRoot == nullptr)
        return;
    // Erase handled entries: later SubtreeRemoveds must not walk a key this pass freed.
    for (auto it = mSourceEditors.begin(); it != mSourceEditors.end();) {
        if (it->second.isNull()) {
            it = mSourceEditors.erase(it);
            continue;
        }
        bool inside = false;
        for (Roblox::Instance* p = it->first; p != nullptr; p = p->GetParent()) {
            if (p == subtreeRoot) {
                inside = true;
                break;
            }
        }
        if (inside) {
            it->second->ForceClose(); // the instance is about to be freed; no prompt can save it
            it = mSourceEditors.erase(it);
        } else {
            ++it;
        }
    }
}