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
// File: MasterHttp.cpp
// Started by: Hattozo
// Started on: 8/23/2026
// Description: Async HTTP against a master server, authenticated with that master's keychain session
#include <cpr/cpr.h>

#include "MasterHttp.h"
#include "MasterServerStore.h"
#include "../Application.h"

#include <QPointer>
#include <QTimer>

#include <chrono>
#include <format>
#include <thread>

using namespace NoobWarrior;

static constexpr int kTimeoutMs = 10000;

Account *MasterHttp::FindAccount(const QString &masterUrl) {
    QString target = MasterServerStore::NormalizeUrl(masterUrl);
    if (target.isEmpty())
        return nullptr;

    MasterKeychain *keychain = gApp->GetCore()->GetMasterKeychain();
    for (Account &account : keychain->GetAccounts()) {
        if (QString::fromStdString(account.Url).compare(target, Qt::CaseInsensitive) == 0)
            return &account;
    }
    return nullptr;
}

bool MasterHttp::IsSignedIn(const QString &masterUrl) {
    return FindAccount(masterUrl) != nullptr;
}

std::string MasterHttp::SessionToken(const QString &masterUrl) {
    Account *account = FindAccount(masterUrl);
    return account != nullptr ? account->Token : std::string();
}

void MasterHttp::SignIn(QObject *owner, const QString &masterUrl, const QString &username,
                        const QString &password, std::function<void(bool)> callback) {
    Core *core = gApp->GetCore();
    std::string url = MasterServerStore::NormalizeUrl(masterUrl).toStdString();
    std::string user = username.toStdString();
    std::string pass = password.toStdString();

    QPointer<QObject> guard(owner);
    std::thread([core, url, user, pass, guard, callback = std::move(callback)]() {
        bool ok = core->LoginToMaster(url, user, pass);
        QTimer::singleShot(0, qApp, [ok, guard, callback]() {
            if (guard.isNull())
                return;
            callback(ok);
        });
    }).detach();
}

void MasterHttp::SignOut(const QString &masterUrl) {
    MasterKeychain *keychain = gApp->GetCore()->GetMasterKeychain();
    std::vector<Account> &accounts = keychain->GetAccounts();

    Account *account = FindAccount(masterUrl);
    if (account == nullptr)
        return;

    int index = static_cast<int>(account - accounts.data());
    keychain->RemoveAccount(index);
    keychain->WriteToKeychain();
}

QString MasterHttp::ResolveUrl(const QString &masterUrl, const QString &path) {
    QString base = MasterServerStore::NormalizeUrl(masterUrl);
    if (path.startsWith('/'))
        return base + path;
    return base + "/" + path;
}

static void RunAsync(QObject *owner, std::function<MasterResponse()> request, MasterHttp::Callback callback) {
    QPointer<QObject> guard(owner);
    std::thread([request = std::move(request), callback = std::move(callback), guard]() {
        MasterResponse response = request();
        QTimer::singleShot(0, qApp, [callback, guard, response = std::move(response)]() {
            if (guard.isNull())
                return;
            callback(response);
        });
    }).detach();
}

static cpr::Header SessionHeader(const QString &masterUrl, const char *accept) {
    cpr::Header header {{"Accept", accept}};
    std::string token = MasterHttp::SessionToken(masterUrl);
    if (!token.empty())
        header["Cookie"] = ".LOGINSESSION=" + token;
    return header;
}

static MasterResponse ToMasterResponse(const cpr::Response &res) {
    MasterResponse response {};
    response.Status = res.status_code;
    response.Body = res.text;
    if (res.error.code != cpr::ErrorCode::OK) {
        response.Error = res.error.message.empty() ? "Could not reach the master server" : res.error.message;
        return response;
    }
    if (res.status_code >= 400) {
        response.Error = res.text.empty()
            ? std::format("The master server answered HTTP {}", res.status_code)
            : res.text;
        return response;
    }
    response.Ok = true;
    return response;
}

void MasterHttp::Get(QObject *owner, const QString &masterUrl, const QString &path, Callback callback) {
    std::string url = ResolveUrl(masterUrl, path).toStdString();
    cpr::Header header = SessionHeader(masterUrl, "application/json");

    RunAsync(owner, [url = std::move(url), header = std::move(header)]() {
        return ToMasterResponse(cpr::Get(
            cpr::Url{url},
            header,
            cpr::Timeout{std::chrono::milliseconds(kTimeoutMs)},
            cpr::VerifySsl{false}));
    }, std::move(callback));
}

void MasterHttp::Post(QObject *owner, const QString &masterUrl, const QString &path,
                      const std::vector<Field> &fields, Callback callback) {
    std::string url = ResolveUrl(masterUrl, path).toStdString();
    cpr::Header header = SessionHeader(masterUrl, "application/json");

    cpr::Payload payload {};
    for (const Field &field : fields)
        payload.Add(cpr::Pair{field.first, field.second});

    RunAsync(owner, [url = std::move(url), header = std::move(header), payload = std::move(payload)]() {
        return ToMasterResponse(cpr::Post(
            cpr::Url{url},
            header,
            payload,
            cpr::Redirect{false},
            cpr::Timeout{std::chrono::milliseconds(kTimeoutMs)},
            cpr::VerifySsl{false}));
    }, std::move(callback));
}
