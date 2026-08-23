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

#include <NoobWarrior/EmuDb/UserRank.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace NoobWarrior {
class EmuDb;
class Registry;

namespace AuthUtil {

// Argon2id parameters
inline constexpr int kHashLength = 32;
inline constexpr int kSaltLength = 16;

// A resolved user identity (from a login session, an auth ticket, a guest, or a federated master)
struct SessionUser {
    int64_t id {0};
    std::string name;
    std::string displayName;
    bool isGuest { false };
    bool isFederated { false };
    int64_t rank { kUserRankGuest };
};

// Hex helpers
std::string ToHex(const unsigned char *bytes, size_t len);
bool FromHex(const std::string &hex, std::vector<unsigned char> &out);

// URL-safe base64 (no padding). Used to pack arbitrary strings into ticket/cookie fields.
std::string Base64UrlEncode(std::string_view data);
std::optional<std::string> Base64UrlDecode(std::string_view encoded);

// Cryptographically-random helpers
bool RandomBytes(unsigned char *out, size_t len);
std::string RandomHex(size_t numBytes); // returns numBytes*2 hex chars, empty on failure

// Password hashing (Argon2id). HashPassword returns a hex-encoded digest, empty on failure.
// VerifyPassword re-derives with the stored salt and compares in constant time.
std::string HashPassword(const std::string &password, const std::vector<unsigned char> &salt);
bool VerifyPassword(const std::string &password, const std::string &saltHex, const std::string &storedHashHex);

// Parses a single cookie value out of a Cookie header. Returns "" if absent.
std::string ExtractCookieValue(const char *cookieHeader, std::string_view name);

// Ed25519 for federation signing. Keys and signatures are hex (privHex/pubHex are 64 chars = 32 bytes;
// a signature is 128 chars = 64 bytes). GenerateEd25519 fills priv/pub and returns true on success;
// Ed25519Sign returns the signature hex ("" on failure); Ed25519Verify checks it.
bool GenerateEd25519(std::string &privHex, std::string &pubHex);
std::string Ed25519Sign(const std::string &privHex, std::string_view message);
bool Ed25519Verify(const std::string &pubHex, std::string_view message, const std::string &sigHex);

// token -> account, via the LoginSession-User join. nullopt for empty/unknown tokens. When ttlSeconds
// > 0, a session idle (unused) for longer than that is treated as expired (nullopt); the lookup also
// refreshes LastUsedTimestamp so an active session never expires. ttlSeconds <= 0 disables expiry.
std::optional<SessionUser> ResolveSessionUser(EmuDb *master, const std::string &token, int64_t ttlSeconds = 0);

// Deletes login sessions idle for >= ttlSeconds. Returns how many were reaped (0 when ttlSeconds <= 0).
int ReapExpiredSessions(EmuDb *master, int64_t ttlSeconds);

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

// Federated user from another master (OnlineUserId, no local DB row). Like guests, the identity is
// carried inside the ticket string rather than an AuthTicket, so the game server can redeem it here.
std::string EncodeFederatedTicket(const SessionUser &user);
std::optional<SessionUser> DecodeFederatedTicket(const std::string &ticket);

// ---- Permissions ----
//
// A permission is a name under the emu.permissions.* registry tree holding the minimum rank that may
// perform it ("ban" -> emu.permissions.ban). Call sites name the permission and never a number, so an
// operator renaming or renumbering the ranks in emu.roles never requires a code change.

// The rank a request is actually judged at. This is the user's own rank, except that
// emu.ranks.owner_user_id is always the maximum - that override is the way back in when a bad rank or
// permission edit locks the operator out of their own control panel.
int64_t EffectiveRank(Registry *reg, int64_t userId, int64_t rank);

// The minimum rank emu.permissions.<permission> requires. Returns nullopt when the key does not
// exist, which HasPermission treats as a denial: a typo in a permission name must fail closed rather
// than quietly admit everyone.
std::optional<int64_t> PermissionFloor(Registry *reg, const std::string &permission);

// True when the identity may perform the named permission. The maximum rank always passes, so no
// configuration mistake can produce a server nobody can administer.
bool HasPermission(Registry *reg, int64_t userId, int64_t rank, const std::string &permission);
inline bool HasPermission(Registry *reg, const SessionUser &user, const std::string &permission) {
    return HasPermission(reg, user.id, user.rank, permission);
}

// Local accounts on the server emulator's master DB, the accounts players log in with in master
// mode (also what the master-server plugin authenticates against).
struct LocalAccount {
    int64_t id {0};
    std::string name;
    std::string displayName;
    int64_t joinDate {0};
};
std::vector<LocalAccount> ListLocalAccounts(EmuDb *master);
bool LocalAccountExists(EmuDb *master, const std::string &name);
// Hashes the password (Argon2id) and inserts a User row; returns the new id, or nullopt on failure
// (name taken, empty inputs, hash/insert error).
std::optional<int64_t> CreateLocalAccount(EmuDb *master, const std::string &name,
                                          const std::string &password, const std::string &displayName = "");
// Removes the account and its sessions/tickets. Returns false only if the DB is unusable.
bool DeleteLocalAccount(EmuDb *master, int64_t id);

}
}
