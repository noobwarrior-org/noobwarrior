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
// File: SourceEditorContainer.cpp
// Started by: Hattozo
// Started on: 8/29/2026
// Description:
#include "SourceEditorContainer.h"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QMessageBox>
#include <QCloseEvent>

using namespace NoobWarrior;

SourceEditorContainer::SourceEditorContainer(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    auto* header = new QHBoxLayout();
    ApplyButton = new QPushButton(QIcon(":/images/silk/accept.png"), "Apply Changes", this);
    ApplyButton->setShortcut(QKeySequence::Save);
    header->addWidget(ApplyButton);
    header->addStretch(1);
    layout->addLayout(header);

    Editor = new CodeEditorWidget(this);
    layout->addWidget(Editor, 1);

    connect(ApplyButton, &QPushButton::clicked, this, [this]() {
        if (OnApply)
            OnApply();
    });
    connect(Editor->document(), &QTextDocument::modificationChanged, this,
            [this](bool) { UpdateDirtyIndicator(); });
}

void SourceEditorContainer::SetBaseTitle(const QString& title) {
    mBaseTitle = title;
    UpdateDirtyIndicator();
}

void SourceEditorContainer::UpdateDirtyIndicator() {
    const bool dirty = IsDirty();
    if (isWindow()) {
        setWindowTitle(mBaseTitle + "[*]");
        setWindowModified(dirty);
        return;
    }
    for (QWidget* w = parentWidget(); w != nullptr; w = w->parentWidget()) {
        if (auto* tabs = qobject_cast<QTabWidget*>(w)) {
            const int index = tabs->indexOf(this);
            if (index >= 0)
                tabs->setTabText(index, mBaseTitle + (dirty ? "*" : ""));
            return;
        }
    }
}

bool SourceEditorContainer::IsDirty() const {
    return Editor->document()->isModified();
}

void SourceEditorContainer::ForceClose() {
    mForceClose = true;
    close();
}

void SourceEditorContainer::closeEvent(QCloseEvent* event) {
    if (mForceClose || !IsDirty()) {
        event->accept();
        return;
    }
    const auto choice = QMessageBox::question(this, "Unapplied Changes",
        "This script has changes that have not been applied. Apply them before closing?",
        QMessageBox::Apply | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Apply);
    if (choice == QMessageBox::Discard || (choice == QMessageBox::Apply && OnApply && OnApply())) {
        event->accept();
        return;
    }
    event->ignore();
}