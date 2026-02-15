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
// File: ItemDialog.h
// Started by: Hattozo
// Started on: 10/27/2025
// Description: Dialog window that allows you to edit or create an item.
#pragma once
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QDateTimeEdit>
#include <QMessageBox>

#include <NoobWarrior/EmuDb/EmuDb.h>

#include <memory>
#include <optional>
#include <fstream>
#include <qlineedit.h>

#include "Sdk/Sdk.h"
#include "Sdk/Project/EmuDb/EmuDbProject.h"

namespace NoobWarrior {
class ItemDialog : public QDialog {
public:
    ItemDialog(QWidget *parent = nullptr, std::optional<int> id = std::nullopt);

    void RegenWidgets();

    EmuDb* GetDatabase();
protected:
    virtual void AddCustomWidgets() = 0;
    virtual void OnSave() = 0;

    std::optional<int> mId;

    Sdk* mSdk;

    QLabel* mIcon;

    QHBoxLayout* mLayout;
    QVBoxLayout* mSidebarLayout;
    QFormLayout* mContentLayout;

    QLineEdit* mIdInput;
    QLineEdit* mNameInput;
    QLineEdit* mDescriptionInput;
    QDateTimeEdit* mCreatedInput;
    QDateTimeEdit* mUpdatedInput;

    QDialogButtonBox* mButtonBox;
};
}