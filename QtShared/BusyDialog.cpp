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
// File: BusyDialog.cpp
// Started by: Hattozo
// Started on: 7/3/2026
// Description:
#include "BusyDialog.h"

#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

using namespace NoobWarrior;

BusyDialog::BusyDialog(const QString& message, QWidget* parent) : QDialog(parent) {
    setWindowTitle("Please wait");
    setWindowModality(Qt::WindowModal);
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    auto* layout = new QVBoxLayout(this);
    mLabel = new QLabel(message, this);
    layout->addWidget(mLabel);

    auto* bar = new QProgressBar(this);
    bar->setRange(0, 0);
    bar->setTextVisible(false);
    layout->addWidget(bar);

    setMinimumWidth(280);
}

void BusyDialog::SetMessage(const QString& message) {
    mLabel->setText(message);
}
