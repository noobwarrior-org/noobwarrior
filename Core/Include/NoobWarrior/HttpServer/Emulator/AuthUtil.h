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
// File: AuthUtil.h
// Started by: Hattozo
// Started on: 7/2/2026
// Description:
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace NoobWarrior {
class EmuDb;

namespace AuthUtil {

// Argon2id parameters
inline constexpr int kHashLength = 32;
inline constexpr int kSaltLength = 16;

// A resolved user identity (from a login session, an auth ticket, or a guest)
struct SessionUser {
    int64_t id {0};
    std::string name;
    std::string displayName;
    bool isGuest {false};
};

// Hex helpers
std::string ToHex(const unsigned char *bytes, size_t len);
bool FromHex(const std::string &hex, std::vector<unsigned char> &out);

// Cryptographically-random helpers
bool RandomBytes(unsigned char *out, size_t len);
std::string RandomHex(size_t numBytes); // returns numBytes*2 hex chars, empty on failure

// Password hashing (Argon2id). HashPassword returns a hex-encoded digest, empty on failure.
// VerifyPassword re-derives with the stored salt and compares in constant time.
std::string HashPassword(const std::string &password, const std::vector<unsigned char> &salt);
bool VerifyPassword(const std::string &password, const std::string &saltHex, const std::string &storedHashHex);

// Parses a single cookie value out of a Cookie header. Returns "" if absent.
std::string ExtractCookieValue(const char *cookieHeader, std::string_view name);

// token -> account, via the LoginSession-User join. nullopt for empty/unknown tokens.
std::optional<SessionUser> ResolveSessionUser(EmuDb *master, const std::string &token);

// Creates a LoginSession for an already-authenticated user; returns the token ("" on failure).
std::string CreateLoginSession(EmuDb *master, int64_t userId, const std::string &ip = "", const std::string &device = "");

// One-time game-join ticket bound to userId. Mint returns the ticket ("" on failure); Redeem consumes
// it and returns the account if it exists, is unredeemed and younger than ttlSeconds, else nullopt.
std::string MintAuthTicket(EmuDb *master, int64_t userId, int64_t placeId);
std::optional<SessionUser> RedeemAuthTicket(EmuDb *master, const std::string &ticket, int64_t ttlSeconds);

// Transient pre-2017-style guest (negative UserId, never persisted). Guests have no DB row, so their
// identity is carried inside the ticket string itself rather than a real AuthTicket.
SessionUser MakeGuestUser();
std::string EncodeGuestTicket(const SessionUser &guest);
std::optional<SessionUser> DecodeGuestTicket(const std::string &ticket);

}
}
