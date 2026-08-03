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
// File: PluginTreeWidget.cpp
// Started by: Hattozo
// Started on: 6/23/2026
// Description:
#include "PluginTreeWidget.h"
#include "../Application.h"
#include "NoobWarrior/Plugin.h"

#include <NoobWarrior/Macros.h>

#include <QIcon>
#include <QPixmap>
#include <QMessageBox>
#include <filesystem>

using namespace NoobWarrior;

static constexpr int RoleIdentifier = Qt::UserRole;
static constexpr int RoleFilePath   = Qt::UserRole + 1;
static constexpr int RolePrivileged = Qt::UserRole + 2;

PluginTreeWidget::PluginTreeWidget(QWidget* parent) : QTreeWidget(parent) {
    setColumnCount(5);
    setHeaderLabels({"", "Enabled", "Icon", "Info", "Description"});
    setColumnHidden(0, true);

    connect(this, &QTreeWidget::itemChanged, this, &PluginTreeWidget::OnItemChanged);
}

PluginTreeWidget::~PluginTreeWidget() {

}

void PluginTreeWidget::Refresh(int flags) {
    mRefreshing = true;
    clear();

    PluginManager* plgMgr = gApp->GetCore()->GetPluginManager();

    std::vector<Plugin::Properties> allProps;
    if (flags & NW_PRIVILEGED_PLUGINS) {
        for (const Plugin::Properties &privProp : plgMgr->GetPrivilegedPluginProperties())
            allProps.push_back(privProp);
    }
    if (flags & NW_NON_PRIVILEGED_PLUGINS) {
        for (const Plugin::Properties &prop : plgMgr->GetPluginProperties())
            allProps.push_back(prop);
    }

    for (const Plugin::Properties &props : allProps) {
        std::string authors;
        for (std::string author : props.Authors)
            authors.append(author + ", ");

        auto *item = new QTreeWidgetItem({
            "", // Empty so that you can rearrange all headers
            "", // Checkmark box for enabling it
            "", // Plugin icon
            QString("%1\n%2\n%3").arg(props.Title, props.Version, authors),
            QString::fromStdString(props.Description)
        });

        item->setData(0, RoleIdentifier, QString::fromStdString(props.Identifier));
        item->setData(0, RoleFilePath, QString::fromStdString(props.FilePath.string()));
        item->setData(0, RolePrivileged, props.IsPrivileged);

        if (props.IsPrivileged) {
            // PRIVILEGED PLUGINS CANNOT BE DISABLED
            item->setFlags(Qt::ItemIsEnabled);
            item->setCheckState(1, Qt::Checked);
        } else {
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            item->setCheckState(1, plgMgr->IsPluginMounted(props.Identifier) ? Qt::Checked : Qt::Unchecked);
        }

        if (!props.IconData.empty()) {
            QPixmap pixmap;
            if (pixmap.loadFromData(props.IconData.data(), static_cast<uint>(props.IconData.size())))
                item->setIcon(2, QIcon(pixmap));
        }

        addTopLevelItem(item);
    }

    mRefreshing = false;
}

void PluginTreeWidget::OnItemChanged(QTreeWidgetItem* item, int column) {
    // Only react to the user toggling the "Enabled" checkbox
    if (mRefreshing || column != 1)
        return;
    if (item->data(0, RolePrivileged).toBool())
        return;

    if (!mSeenDisclaimer) {
        mSeenDisclaimer = true;
        QMessageBox::warning(this, "Notice", "Changes to plugins do not apply until " NOOBWARRIOR_BRAND " is restarted.");
    }

    PluginManager* plgMgr = gApp->GetCore()->GetPluginManager();
    std::string identifier = item->data(0, RoleIdentifier).toString().toStdString();
    std::filesystem::path filePath = item->data(0, RoleFilePath).toString().toStdString();
    bool enabled = item->checkState(1) == Qt::Checked;
    plgMgr->SetPluginSelected(filePath.filename().string(), enabled);
}