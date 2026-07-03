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
// File: AuthUtil.cpp
// Started by: Hattozo
// Started on: 7/2/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/Log.h>

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/core_names.h>
#include <openssl/evp.h>

#include <charconv>
#include <format>

namespace NoobWarrior {
namespace AuthUtil {

std::string ToHex(const unsigned char *bytes, size_t len) {
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; i++)
        out += std::format("{:02x}", bytes[i]);
    return out;
}

bool FromHex(const std::string &hex, std::vector<unsigned char> &out) {
    if (hex.size() % 2 != 0) return false;
    out.clear();
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        unsigned int byte = 0;
        auto [ptr, ec] = std::from_chars(hex.data() + i, hex.data() + i + 2, byte, 16);
        if (ec != std::errc() || ptr != hex.data() + i + 2) return false;
        out.push_back(static_cast<unsigned char>(byte));
    }
    return true;
}

static constexpr char kB64Url[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

std::string Base64UrlEncode(std::string_view data) {
    std::string out;
    out.reserve((data.size() + 2) / 3 * 4);
    size_t i = 0;
    for (; i + 3 <= data.size(); i += 3) {
        uint32_t n = (static_cast<unsigned char>(data[i]) << 16) |
                     (static_cast<unsigned char>(data[i + 1]) << 8) |
                     static_cast<unsigned char>(data[i + 2]);
        out += kB64Url[(n >> 18) & 63];
        out += kB64Url[(n >> 12) & 63];
        out += kB64Url[(n >> 6) & 63];
        out += kB64Url[n & 63];
    }
    if (size_t rem = data.size() - i; rem == 1) {
        uint32_t n = static_cast<unsigned char>(data[i]) << 16;
        out += kB64Url[(n >> 18) & 63];
        out += kB64Url[(n >> 12) & 63];
    } else if (rem == 2) {
        uint32_t n = (static_cast<unsigned char>(data[i]) << 16) |
                     (static_cast<unsigned char>(data[i + 1]) << 8);
        out += kB64Url[(n >> 18) & 63];
        out += kB64Url[(n >> 12) & 63];
        out += kB64Url[(n >> 6) & 63];
    }
    return out;
}

std::optional<std::string> Base64UrlDecode(std::string_view encoded) {
    auto value = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '-') return 62;
        if (c == '_') return 63;
        return -1;
    };
    std::string out;
    out.reserve(encoded.size() / 4 * 3 + 2);
    uint32_t buf = 0;
    int bits = 0;
    for (char c : encoded) {
        int v = value(c);
        if (v < 0)
            return std::nullopt;
        buf = (buf << 6) | static_cast<uint32_t>(v);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out += static_cast<char>((buf >> bits) & 0xFF);
        }
    }
    return out;
}

bool RandomBytes(unsigned char *out, size_t len) {
    return RAND_bytes(out, static_cast<int>(len)) == 1;
}

std::string RandomHex(size_t numBytes) {
    std::vector<unsigned char> bytes(numBytes);
    if (!RandomBytes(bytes.data(), numBytes))
        return "";
    return ToHex(bytes.data(), numBytes);
}

std::string HashPassword(const std::string &password, const std::vector<unsigned char> &salt) {
    if (salt.size() != kSaltLength)
        return "";

    unsigned char hash[kHashLength];
    uint32_t t_cost = 2, m_cost = 1u << 16, lanes = 1, threads = 1;
    EVP_KDF *kdf = EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr);
    EVP_KDF_CTX *kctx = kdf ? EVP_KDF_CTX_new(kdf) : nullptr;
    EVP_KDF_free(kdf);
    OSSL_PARAM params[] = {
        OSSL_PARAM_octet_string(OSSL_KDF_PARAM_PASSWORD, const_cast<char*>(password.data()), password.size()),
        OSSL_PARAM_octet_string(OSSL_KDF_PARAM_SALT, const_cast<unsigned char*>(salt.data()), kSaltLength),
        OSSL_PARAM_uint32(OSSL_KDF_PARAM_ITER, &t_cost),
        OSSL_PARAM_uint32(OSSL_KDF_PARAM_ARGON2_MEMCOST, &m_cost),
        OSSL_PARAM_uint32(OSSL_KDF_PARAM_ARGON2_LANES, &lanes),
        OSSL_PARAM_uint32(OSSL_KDF_PARAM_THREADS, &threads),
        OSSL_PARAM_END
    };
    bool ok = kctx && EVP_KDF_derive(kctx, hash, kHashLength, params) > 0;
    EVP_KDF_CTX_free(kctx);
    if (!ok)
        return "";
    return ToHex(hash, kHashLength);
}

bool VerifyPassword(const std::string &password, const std::string &saltHex, const std::string &storedHashHex) {
    std::vector<unsigned char> salt;
    if (!FromHex(saltHex, salt) || salt.size() != kSaltLength)
        return false;

    std::string computed = HashPassword(password, salt);
    if (computed.empty() || computed.size() != storedHashHex.size())
        return false;
    return CRYPTO_memcmp(computed.data(), storedHashHex.data(), computed.size()) == 0;
}

std::string ExtractCookieValue(const char *cookieHeader, std::string_view name) {
    if (cookieHeader == nullptr) return "";
    std::string_view hdr(cookieHeader);
    size_t pos = 0;
    while (pos < hdr.size()) {
        while (pos < hdr.size() && (hdr[pos] == ' ' || hdr[pos] == ';')) pos++;
        size_t eq = hdr.find('=', pos);
        if (eq == std::string_view::npos) break;
        size_t end = hdr.find(';', eq);
        if (end == std::string_view::npos) end = hdr.size();
        if (hdr.substr(pos, eq - pos) == name)
            return std::string(hdr.substr(eq + 1, end - eq - 1));
        pos = end;
    }
    return "";
}

bool GenerateEd25519(std::string &privHex, std::string &pubHex) {
    EVP_PKEY *pkey = EVP_PKEY_Q_keygen(nullptr, nullptr, "ED25519");
    if (pkey == nullptr)
        return false;
    unsigned char priv[32], pub[32];
    size_t privLen = sizeof(priv), pubLen = sizeof(pub);
    bool ok = EVP_PKEY_get_raw_private_key(pkey, priv, &privLen) == 1 && privLen == 32 &&
              EVP_PKEY_get_raw_public_key(pkey, pub, &pubLen) == 1 && pubLen == 32;
    EVP_PKEY_free(pkey);
    if (!ok)
        return false;
    privHex = ToHex(priv, sizeof(priv));
    pubHex = ToHex(pub, sizeof(pub));
    return true;
}

std::string Ed25519Sign(const std::string &privHex, std::string_view message) {
    std::vector<unsigned char> priv;
    if (!FromHex(privHex, priv) || priv.size() != 32)
        return "";
    EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, priv.data(), priv.size());
    if (pkey == nullptr)
        return "";
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    unsigned char sig[64];
    size_t sigLen = sizeof(sig);
    std::string out;
    if (ctx != nullptr && EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, pkey) == 1 &&
        EVP_DigestSign(ctx, sig, &sigLen, reinterpret_cast<const unsigned char *>(message.data()), message.size()) == 1 &&
        sigLen == 64)
        out = ToHex(sig, sigLen);
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return out;
}

bool Ed25519Verify(const std::string &pubHex, std::string_view message, const std::string &sigHex) {
    std::vector<unsigned char> pub, sig;
    if (!FromHex(pubHex, pub) || pub.size() != 32)
        return false;
    if (!FromHex(sigHex, sig) || sig.size() != 64)
        return false;
    EVP_PKEY *pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, pub.data(), pub.size());
    if (pkey == nullptr)
        return false;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    bool ok = ctx != nullptr && EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey) == 1 &&
              EVP_DigestVerify(ctx, sig.data(), sig.size(),
                               reinterpret_cast<const unsigned char *>(message.data()), message.size()) == 1;
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return ok;
}

std::optional<SessionUser> ResolveSessionUser(EmuDb *master, const std::string &token, int64_t ttlSeconds) {
    if (master == nullptr || master->Fail() || token.empty())
        return std::nullopt;

    // ttlSeconds <= 0 disables expiry; otherwise a session idle longer than the TTL no longer resolves.
    Statement stmt = master->PrepareStatement(
        "SELECT u.Id, u.Name, u.DisplayName FROM LoginSession s "
        "JOIN User u ON u.Id = s.UserId "
        "WHERE s.Token = ? AND (? <= 0 OR (unixepoch() - s.LastUsedTimestamp) < ?);"
    );
    stmt.Bind(1, token);
    stmt.Bind(2, ttlSeconds);
    stmt.Bind(3, ttlSeconds);
    if (stmt.Step() != SQLITE_ROW)
        return std::nullopt;

    SessionUser user;
    user.id = stmt.GetInt64FromColumnIndex(0);
    user.name = stmt.GetStringFromColumnIndex(1);
    user.displayName = stmt.GetStringFromColumnIndex(2);
    if (user.displayName.empty())
        user.displayName = user.name;

    // Keep the session fresh so the idle-TTL check above (and ReapExpiredSessions) doesn't reap active players.
    Statement touch = master->PrepareStatement(
        "UPDATE LoginSession SET LastUsedTimestamp = unixepoch() WHERE Token = ?;"
    );
    touch.Bind(1, token);
    if (touch.Step() == SQLITE_DONE)
        master->MarkDirty();

    return user;
}

int ReapExpiredSessions(EmuDb *master, int64_t ttlSeconds) {
    if (master == nullptr || master->Fail() || ttlSeconds <= 0)
        return 0;
    Statement stmt = master->PrepareStatement(
        "DELETE FROM LoginSession WHERE (unixepoch() - LastUsedTimestamp) >= ?;"
    );
    stmt.Bind(1, ttlSeconds);
    if (stmt.Step() != SQLITE_DONE)
        return 0;
    int reaped = sqlite3_changes(master->Get());
    if (reaped > 0)
        master->MarkDirty();
    return reaped;
}

std::string MintAuthTicket(EmuDb *master, int64_t userId, int64_t placeId) {
    if (master == nullptr || master->Fail())
        return "";

    std::string ticket = RandomHex(32);
    if (ticket.empty())
        return "";

    Statement stmt = master->PrepareStatement(
        "INSERT INTO AuthTicket (Ticket, UserId, PlaceId) VALUES (?, ?, ?);"
    );
    stmt.Bind(1, ticket);
    stmt.Bind(2, userId);
    stmt.Bind(3, placeId);
    if (stmt.Step() != SQLITE_DONE) {
        Out("AuthUtil", "Failed to mint auth ticket for user {}: {}", userId, master->GetLastErrorMsg());
        return "";
    }
    master->MarkDirty();
    return ticket;
}

std::optional<SessionUser> RedeemAuthTicket(EmuDb *master, const std::string &ticket, int64_t ttlSeconds) {
    if (master == nullptr || master->Fail() || ticket.empty())
        return std::nullopt;

    Statement stmt = master->PrepareStatement(
        "SELECT u.Id, u.Name, u.DisplayName FROM AuthTicket t "
        "JOIN User u ON u.Id = t.UserId "
        "WHERE t.Ticket = ? AND t.Redeemed = 0 AND (unixepoch() - t.CreatedTimestamp) < ?;"
    );
    stmt.Bind(1, ticket);
    stmt.Bind(2, ttlSeconds);
    if (stmt.Step() != SQLITE_ROW)
        return std::nullopt;

    SessionUser user;
    user.id = stmt.GetInt64FromColumnIndex(0);
    user.name = stmt.GetStringFromColumnIndex(1);
    user.displayName = stmt.GetStringFromColumnIndex(2);
    if (user.displayName.empty())
        user.displayName = user.name;

    // single-use: only the caller that flips Redeemed 0->1 wins the ticket
    Statement redeem = master->PrepareStatement(
        "UPDATE AuthTicket SET Redeemed = 1 WHERE Ticket = ? AND Redeemed = 0;"
    );
    redeem.Bind(1, ticket);
    if (redeem.Step() != SQLITE_DONE || sqlite3_changes(master->Get()) == 0)
        return std::nullopt;
    master->MarkDirty();

    return user;
}

std::string CreateLoginSession(EmuDb *master, int64_t userId, const std::string &ip, const std::string &device) {
    if (master == nullptr || master->Fail())
        return "";

    std::string token = RandomHex(32);
    if (token.empty())
        return "";

    Statement stmt = master->PrepareStatement(
        "INSERT INTO LoginSession (Token, UserId, Ip, Device) VALUES (?, ?, ?, ?);"
    );
    stmt.Bind(1, token);
    stmt.Bind(2, userId);
    stmt.Bind(3, ip);
    stmt.Bind(4, device);
    if (stmt.Step() != SQLITE_DONE) {
        Out("AuthUtil", "Failed to create login session for user {}: {}", userId, master->GetLastErrorMsg());
        return "";
    }
    master->MarkDirty();
    return token;
}

std::vector<LocalAccount> ListLocalAccounts(EmuDb *master) {
    std::vector<LocalAccount> out;
    if (master == nullptr || master->Fail())
        return out;
    Statement stmt = master->PrepareStatement(
        "SELECT Id, Name, DisplayName, COALESCE(JoinDate, 0) FROM User "
        "WHERE PasswordHash IS NOT NULL AND PasswordHash != '' ORDER BY Id ASC;"
    );
    while (stmt.Step() == SQLITE_ROW) {
        LocalAccount a;
        a.id = stmt.GetInt64FromColumnIndex(0);
        a.name = stmt.GetStringFromColumnIndex(1);
        a.displayName = stmt.GetStringFromColumnIndex(2);
        if (a.displayName.empty())
            a.displayName = a.name;
        a.joinDate = stmt.GetInt64FromColumnIndex(3);
        out.push_back(std::move(a));
    }
    return out;
}

bool LocalAccountExists(EmuDb *master, const std::string &name) {
    if (master == nullptr || master->Fail())
        return false;
    Statement stmt = master->PrepareStatement("SELECT 1 FROM User WHERE Name = ? COLLATE NOCASE;");
    stmt.Bind(1, name);
    return stmt.Step() == SQLITE_ROW;
}

std::optional<int64_t> CreateLocalAccount(EmuDb *master, const std::string &name,
                                          const std::string &password, const std::string &displayName) {
    if (master == nullptr || master->Fail() || name.empty() || password.empty())
        return std::nullopt;
    if (LocalAccountExists(master, name))
        return std::nullopt;

    std::vector<unsigned char> salt(kSaltLength);
    if (!RandomBytes(salt.data(), salt.size()))
        return std::nullopt;
    std::string saltHex = ToHex(salt.data(), salt.size());
    std::string hashHex = HashPassword(password, salt);
    if (hashHex.empty())
        return std::nullopt;

    Statement stmt = master->PrepareStatement(
        "INSERT INTO User (Name, DisplayName, PasswordHash, PasswordSalt, JoinDate) VALUES (?, ?, ?, ?, unixepoch());"
    );
    stmt.Bind(1, name);
    stmt.Bind(2, displayName.empty() ? name : displayName);
    stmt.Bind(3, hashHex);
    stmt.Bind(4, saltHex);
    if (stmt.Step() != SQLITE_DONE) {
        Out("AuthUtil", "Failed to create local account \"{}\": {}", name, master->GetLastErrorMsg());
        return std::nullopt;
    }
    int64_t id = sqlite3_last_insert_rowid(master->Get());
    master->MarkDirty();
    return id;
}

bool DeleteLocalAccount(EmuDb *master, int64_t id) {
    if (master == nullptr || master->Fail())
        return false;
    for (const char *sql : {"DELETE FROM User WHERE Id = ?;",
                            "DELETE FROM LoginSession WHERE UserId = ?;",
                            "DELETE FROM AuthTicket WHERE UserId = ?;"}) {
        Statement stmt = master->PrepareStatement(sql);
        stmt.Bind(1, id);
        stmt.Step();
    }
    master->MarkDirty();
    return true;
}

// UserId = -1 - n, so the (negative) id alone reconstructs the whole guest and never hits a real account.
static SessionUser GuestFromNumber(int guestNumber) {
    SessionUser guest;
    guest.isGuest = true;
    guest.id = -1 - static_cast<int64_t>(guestNumber);
    guest.name = std::format("Guest {}", guestNumber);
    guest.displayName = std::format("Guest {}", guestNumber);
    return guest;
}

SessionUser MakeGuestUser() {
    unsigned char rnd[4] = {0};
    RandomBytes(rnd, sizeof(rnd));
    uint32_t n = (static_cast<uint32_t>(rnd[0]) << 24) | (static_cast<uint32_t>(rnd[1]) << 16) |
                 (static_cast<uint32_t>(rnd[2]) << 8)  | static_cast<uint32_t>(rnd[3]);
    return GuestFromNumber(static_cast<int>(n % 9999));
}

std::string EncodeGuestTicket(const SessionUser &guest) {
    return std::format("guest:{}", guest.id);
}

std::optional<SessionUser> DecodeGuestTicket(const std::string &ticket) {
    static constexpr std::string_view kPrefix = "guest:";
    if (ticket.size() <= kPrefix.size() || ticket.compare(0, kPrefix.size(), kPrefix) != 0)
        return std::nullopt;

    int64_t id = 0;
    const char *begin = ticket.data() + kPrefix.size();
    const char *end = ticket.data() + ticket.size();
    auto [ptr, ec] = std::from_chars(begin, end, id);
    if (ec != std::errc() || ptr != end || id >= 0)
        return std::nullopt;

    int guestNumber = static_cast<int>(-id - 1);
    if (guestNumber < 0)
        return std::nullopt;
    return GuestFromNumber(guestNumber);
}

// "fed:<id>:<b64url name>:<b64url displayName>" — self-contained so a federated user (no local
// User row) can be redeemed without a DB lookup, mirroring the guest ticket.
std::string EncodeFederatedTicket(const SessionUser &user) {
    return std::format("fed:{}:{}:{}", user.id, Base64UrlEncode(user.name), Base64UrlEncode(user.displayName));
}

std::optional<SessionUser> DecodeFederatedTicket(const std::string &ticket) {
    static constexpr std::string_view kPrefix = "fed:";
    if (ticket.compare(0, kPrefix.size(), kPrefix) != 0)
        return std::nullopt;

    std::string_view rest = std::string_view(ticket).substr(kPrefix.size());
    size_t idEnd = rest.find(':');
    if (idEnd == std::string_view::npos)
        return std::nullopt;
    size_t nameEnd = rest.find(':', idEnd + 1);
    if (nameEnd == std::string_view::npos)
        return std::nullopt;

    int64_t id = 0;
    auto [ptr, ec] = std::from_chars(rest.data(), rest.data() + idEnd, id);
    if (ec != std::errc() || ptr != rest.data() + idEnd || id <= 0)
        return std::nullopt;

    std::optional<std::string> name = Base64UrlDecode(rest.substr(idEnd + 1, nameEnd - idEnd - 1));
    std::optional<std::string> displayName = Base64UrlDecode(rest.substr(nameEnd + 1));
    if (!name || !displayName)
        return std::nullopt;

    SessionUser user;
    user.id = id;
    user.name = *name;
    user.displayName = displayName->empty() ? *name : *displayName;
    user.isFederated = true;
    return user;
}

}
}
