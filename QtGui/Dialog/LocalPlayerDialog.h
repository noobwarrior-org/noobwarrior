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
// File: LocalPlayerDialog.h
// Started by: Hattozo
// Started on: 4/23/2026
// Description:
#pragma once
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>

namespace NoobWarrior {
class LocalPlayerDialog : public QDialog {
    Q_OBJECT
public:
    LocalPlayerDialog(QWidget *parent = nullptr);
    ~LocalPlayerDialog();
protected:
    void InitWidgets();
private:
    QHBoxLayout* mLayout;
    QVBoxLayout* mSideLayout;
    QVBoxLayout* mOtherSideLayout;
    QFormLayout* mFormLayout;
    QLineEdit* mIdInput;
    QLineEdit* mNameInput;
    QLineEdit* mDisplayNameInput;
    QDialogButtonBox* mButtonBox;
};
}