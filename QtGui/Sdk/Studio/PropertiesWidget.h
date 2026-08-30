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
// File: PropertiesWidget.h
// Started by: Hattozo
// Started on: 8/29/2026
// Description: A poor replication of Roblox Studio's Properties widget
#pragma once
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>

#include <functional>
#include <QWidget>
#include <QLineEdit>
#include <QTreeWidget>

namespace NoobWarrior {
// Fetches an asset's decoded bytes by id for inline previews (sounds).
using AssetDataResolver = std::function<bool(int64_t assetId, std::vector<unsigned char>* out)>;

class PropertiesWidget : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesWidget(QWidget* parent = nullptr);

    void SetInstance(Roblox::Instance* instance);
    // Multi-select: rows show the property intersection, differing values blank, edits apply to
    // every selected instance.
    void SetInstances(std::vector<Roblox::Instance*> instances);
    size_t SelectionSize() const { return mInstances.size(); }

    // Enables inline sound previews.
    void SetAssetDataResolver(AssetDataResolver resolver) { mAssetResolver = std::move(resolver); }
signals:
    // The user changed something on this instance (property value or name).
    void InstanceEdited(NoobWarrior::Roblox::Instance* instance);
private:
    void ApplyFilter(const QString& text);
    void Rebuild();
    void OnItemChanged(QTreeWidgetItem* item, int column);
    void OnItemDoubleClicked(QTreeWidgetItem* item, int column);
    bool ApplyEdit(QTreeWidgetItem* item);
    void RefreshRowFromValue(QTreeWidgetItem* item, const std::string& propName);
    QTreeWidgetItem* FindPropertyRow(const std::string& propName, int component) const;
    void RefreshSyntheticBrickColorRow();

    QLineEdit* mFilter;
    QTreeWidget* mTree;
    Roblox::Instance* mInstance { nullptr }; // primary (first) selected instance
    std::vector<Roblox::Instance*> mInstances;
    AssetDataResolver mAssetResolver;
    bool mRebuilding { false };
};
}