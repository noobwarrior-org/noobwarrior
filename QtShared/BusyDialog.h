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
// File: BusyDialog.h
// Started by: Hattozo
// Started on: 7/3/2026
// Description:
#pragma once
#include <QDialog>

class QLabel;

namespace NoobWarrior {
class BusyDialog : public QDialog {
    Q_OBJECT
public:
    explicit BusyDialog(const QString& message, QWidget* parent = nullptr);
    void SetMessage(const QString& message);
private:
    QLabel* mLabel;
};
}
