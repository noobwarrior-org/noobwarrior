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
// File: MasterHttp.h
// Started by: Hattozo
// Started on: 8/23/2026
// Description: Async HTTP against a master server, authenticated with that master's keychain session
#pragma once
#include <NoobWarrior/Keychain/Keychain.h>

#include <QObject>
#include <QString>

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace NoobWarrior {
struct MasterResponse {
    bool Ok { false }; // the request went through and the master answered with < 400
    long Status { 0 };
    std::string Body;
    std::string Error; // transport error, or the master's error body
};

// Every page in the Online window talks to a master the same way: a request on a detached thread so
// the UI keeps running, the reply delivered back on the Qt main thread, and the caller's session
// cookie for that master attached when there is one.
class MasterHttp {
public:
    using Callback = std::function<void(const MasterResponse &)>;
    using Field = std::pair<std::string, std::string>;
    
    static Account *FindAccount(const QString &masterUrl);
    static bool IsSignedIn(const QString &masterUrl);
    static std::string SessionToken(const QString &masterUrl);

    static void Get(QObject *owner, const QString &masterUrl, const QString &path, Callback callback);
    static void Post(QObject *owner, const QString &masterUrl, const QString &path,
                     const std::vector<Field> &fields, Callback callback);

    static void SignIn(QObject *owner, const QString &masterUrl, const QString &username,
                       const QString &password, std::function<void(bool)> callback);
    static void SignOut(const QString &masterUrl);

    static QString ResolveUrl(const QString &masterUrl, const QString &path);
};
}
