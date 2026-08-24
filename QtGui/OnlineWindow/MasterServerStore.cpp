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
// File: MasterServerStore.cpp
// Started by: Hattozo
// Started on: 8/23/2026
// Description: The list of master servers the Online window shows, persisted in the registry
#include "MasterServerStore.h"
#include "../Application.h"

#include <QUrl>

using namespace NoobWarrior;

static QString TableString(const sol::table &tbl, const char *key) {
    sol::optional<std::string> value = tbl[key];
    if (!value.has_value())
        return QString();
    return QString::fromStdString(*value);
}

std::vector<MasterServerEntry> MasterServerStore::Load() {
    std::vector<MasterServerEntry> entries;

    Registry *reg = gApp->GetCore()->GetRegistry();
    std::optional<sol::table> masters = reg->GetKeyValue<sol::table>("master_servers");
    if (!masters.has_value())
        return entries;

    for (const auto &kv : masters.value()) {
        if (!kv.second.is<sol::table>())
            continue;
        sol::table master = kv.second.as<sol::table>();

        MasterServerEntry entry {};
        entry.Url = NormalizeUrl(TableString(master, "url"));
        if (entry.Url.isEmpty())
            continue;
        entry.Name = TableString(master, "name");
        entry.Domain = TableString(master, "domain");
        entry.Tagline = TableString(master, "tagline");
        if (entry.Name.isEmpty())
            entry.Name = HostOf(entry.Url);
        entries.push_back(std::move(entry));
    }

    return entries;
}

void MasterServerStore::Save(const std::vector<MasterServerEntry> &entries) {
    Core *core = gApp->GetCore();
    LuaState *lua = core->GetLuaState();

    sol::table mastersTbl = lua->create_table();
    int i = 1;
    for (const MasterServerEntry &entry : entries) {
        sol::table masterTbl = lua->create_table();
        masterTbl["url"] = entry.Url.toStdString();
        masterTbl["name"] = entry.Name.toStdString();
        masterTbl["domain"] = entry.Domain.toStdString();
        masterTbl["tagline"] = entry.Tagline.toStdString();
        mastersTbl[i++] = masterTbl;
    }

    Registry *reg = core->GetRegistry();
    reg->SetKeyValue("master_servers", mastersTbl);
    reg->Save();
}

bool MasterServerStore::Add(const MasterServerEntry &entry) {
    MasterServerEntry normalised = entry;
    normalised.Url = NormalizeUrl(entry.Url);
    if (normalised.Url.isEmpty())
        return false;
    if (normalised.Name.isEmpty())
        normalised.Name = HostOf(normalised.Url);

    std::vector<MasterServerEntry> entries = Load();
    for (const MasterServerEntry &existing : entries) {
        if (existing.Url.compare(normalised.Url, Qt::CaseInsensitive) == 0)
            return false;
    }

    entries.push_back(std::move(normalised));
    Save(entries);
    return true;
}

void MasterServerStore::Remove(const QString &url) {
    QString target = NormalizeUrl(url);
    std::vector<MasterServerEntry> entries = Load();
    std::erase_if(entries, [&target](const MasterServerEntry &entry) {
        return entry.Url.compare(target, Qt::CaseInsensitive) == 0;
    });
    Save(entries);
}

void MasterServerStore::UpdateBranding(const QString &url, const QString &name, const QString &domain,
                                       const QString &tagline) {
    QString target = NormalizeUrl(url);
    std::vector<MasterServerEntry> entries = Load();

    bool changed = false;
    for (MasterServerEntry &entry : entries) {
        if (entry.Url.compare(target, Qt::CaseInsensitive) != 0)
            continue;
        if (!name.isEmpty())
            entry.Name = name;
        entry.Domain = domain;
        entry.Tagline = tagline;
        changed = true;
        break;
    }

    if (changed)
        Save(entries);
}

std::optional<MasterServerEntry> MasterServerStore::Find(const QString &url) {
    QString target = NormalizeUrl(url);
    for (MasterServerEntry &entry : Load()) {
        if (entry.Url.compare(target, Qt::CaseInsensitive) == 0)
            return entry;
    }
    return std::nullopt;
}

QString MasterServerStore::NormalizeUrl(const QString &url) {
    QString trimmed = url.trimmed();
    if (trimmed.isEmpty())
        return QString();
    if (!trimmed.contains("://"))
        trimmed.prepend("http://");
    while (trimmed.endsWith('/'))
        trimmed.chop(1);
    return trimmed;
}

QString MasterServerStore::HostOf(const QString &url) {
    QUrl parsed(NormalizeUrl(url));
    QString host = parsed.host();
    if (host.isEmpty())
        return url;
    if (parsed.port() > 0)
        host += QString(":%1").arg(parsed.port());
    return host;
}
