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
// File: AddMasterServerDialog.h
// Started by: Hattozo
// Started on: 8/23/2026
// Description: Prompt for a master server address, resolving its branding before it is added
#pragma once
#include "MasterServerStore.h"

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;

namespace NoobWarrior {
// Asks only for an address. The master's real name comes from its own /fed/v1/info handshake, which
// this dialog performs before accepting, so a typo or an unreachable host is caught here rather than
// leaving a dead node in the sidebar. Signing in happens later, on the master's Profile page.
class AddMasterServerDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddMasterServerDialog(QWidget *parent = nullptr);

    // Valid after exec() returns Accepted.
    const MasterServerEntry &Result() const { return mResult; }

    // Every way of confirming the dialog (the Add button, Enter in the address field, the default
    // button) funnels through accept(), so none of them can close it before the address resolves.
    void accept() override;

private:
    void InitWidgets();
    void Resolve();
    void SetBusy(bool busy);

    MasterServerEntry mResult {};
    QLineEdit *mUrlInput { nullptr };
    QLabel *mStatusLabel { nullptr };
    QPushButton *mAddButton { nullptr };
};
}
