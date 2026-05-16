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
// File: ServerEmulatorPage.h
// Started by: Hattozo
// Started on: 7/24/2025
// Description:
#pragma once
#include "SettingsPage.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QIntValidator>

namespace NoobWarrior {
class ServerEmulatorPage : public SettingsPage {
public:
    ServerEmulatorPage(QWidget *parent = nullptr);
    const QString GetTitle() override;
    const QString GetDescription() override;
    const QIcon GetIcon() override;
    void Deserialize(Registry* reg) override;
    void Serialize(Registry* reg) override;
protected:
    void InitWidgets();
private:
    QFormLayout* mForm;
    QLineEdit* mPortInput;
};
}
