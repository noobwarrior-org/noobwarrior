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
// File: MasterServerStore.h
// Started by: Hattozo
// Started on: 8/23/2026
// Description: The list of master servers the Online window shows, persisted in the registry
#pragma once
#include <QString>

#include <optional>
#include <vector>

namespace NoobWarrior {
struct MasterServerEntry {
    QString Url;     // normalised base url, no trailing slash
    QString Name;    // branding title reported by /fed/v1/info, else the host
    QString Domain;  // the master's federation domain, when it reported one
    QString Tagline;
};

class MasterServerStore {
public:
    static std::vector<MasterServerEntry> Load();
    static void Save(const std::vector<MasterServerEntry> &entries);

    // Returns false when the url is already listed.
    static bool Add(const MasterServerEntry &entry);
    static void Remove(const QString &url);
    // Updates the cached branding of an already-listed master. No-op if it isn't listed.
    static void UpdateBranding(const QString &url, const QString &name, const QString &domain,
                               const QString &tagline);

    static std::optional<MasterServerEntry> Find(const QString &url);

    // Adds a scheme if the user typed a bare host and drops any trailing slash, so the same master
    // typed two ways compares equal.
    static QString NormalizeUrl(const QString &url);
    // Host[:port] of a url, for labelling a master we couldn't reach.
    static QString HostOf(const QString &url);
};
}
