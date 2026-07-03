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
// File: ServerLoginDialog.h
// Started by: Hattozo
// Started on: 7/2/2026
// Description: Modal login prompt shown when connecting to a server that requires authentication.
#pragma once
#include <QDialog>
#include <QString>

class QLineEdit;

namespace NoobWarrior {
class ServerLoginDialog : public QDialog {
public:
    enum class Mode { Password, Guest };

    ServerLoginDialog(QWidget* parent, const QString& title, const QString& tagline,
                      bool passwordBased, bool allowGuests);

    // Valid after exec() returns Accepted.
    Mode SelectedMode() const { return mMode; }
    QString Username() const;
    QString Password() const;

private:
    Mode mMode { Mode::Password };
    QLineEdit* mUsername { nullptr };
    QLineEdit* mPassword { nullptr };
};
}
