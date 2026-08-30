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
// File: SourceEditorContainer.h
// Started by: Hattozo
// Started on: 8/29/2026
// Description:
#pragma once
#include <QWidget>
#include <QPushButton>

#include "Sdk/CodeEditorWidget.h"

namespace NoobWarrior {
using SourceEditorTabHost = std::function<bool(QWidget* editor, const QString& title)>;

class SourceEditorContainer : public QWidget {
    Q_OBJECT
public:
    explicit SourceEditorContainer(QWidget* parent = nullptr);

    bool IsDirty() const;
    void ForceClose();
    void SetBaseTitle(const QString& title);

    CodeEditorWidget* Editor;
    QPushButton* ApplyButton;
    std::function<bool()> OnApply;
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    void UpdateDirtyIndicator();

    QString mBaseTitle;
    bool mForceClose { false };
};
}