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

std::optional<SessionUser> ResolveSessionUser(EmuDb *master, const std::string &token) {
    if (master == nullptr || master->Fail() || token.empty())
        return std::nullopt;

    Statement stmt = master->PrepareStatement(
        "SELECT u.Id, u.Name, u.DisplayName FROM LoginSession s "
        "JOIN User u ON u.Id = s.UserId WHERE s.Token = ?;"
    );
    stmt.Bind(1, token);
    if (stmt.Step() != SQLITE_ROW)
        return std::nullopt;

    SessionUser user;
    user.id = stmt.GetInt64FromColumnIndex(0);
    user.name = stmt.GetStringFromColumnIndex(1);
    user.displayName = stmt.GetStringFromColumnIndex(2);
    if (user.displayName.empty())
        user.displayName = user.name;

    // Keep the session fresh so idle-session sweeps (if any) don't reap active players.
    Statement touch = master->PrepareStatement(
        "UPDATE LoginSession SET LastUsedTimestamp = unixepoch() WHERE Token = ?;"
    );
    touch.Bind(1, token);
    if (touch.Step() == SQLITE_DONE)
        master->MarkDirty();

    return user;
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

}
}
