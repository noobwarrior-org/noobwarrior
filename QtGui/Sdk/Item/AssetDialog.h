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
// File: AssetDialog.h
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#pragma once
#include "ItemDialog.h"
#include <NoobWarrior/EmuDb/Item/Asset.h>
#include <NoobWarrior/EmuDb/Repository/Item/AssetRepository.h>

#include <QComboBox>

#include <optional>

namespace NoobWarrior {
class AssetDialog : public ItemDialog<Asset> {
    Q_OBJECT
public:
    AssetDialog(QWidget *parent = nullptr, std::optional<int64_t> id = std::nullopt);
    void AddCustomWidgets() override;
    void OnSave() override;
protected:
    QComboBox* mAssetTypeInput;
};
}