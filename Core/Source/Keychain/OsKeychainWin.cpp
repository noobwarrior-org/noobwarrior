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
// File: OsKeychainWin.cpp
// Started by: Hattozo
// Started on: 2/11/2026
// Description: This file is based off https://github.com/hrantzsch/keychain/blob/master/src/keychain_win.cpp
// You can read the appropriate MIT license by clicking the link

// clang-format off
// make sure windows.h is included before wincred.h
#include <NoobWarrior/Keychain/OsKeychain.h>

#include <algorithm>
#include <memory>
#include <string>

#define UNICODE

#include <windows.h>
#include <wincred.h>
#define DWORD_MAX 0xffffffffUL

using namespace NoobWarrior;

static const DWORD kCredType = CRED_TYPE_GENERIC;

struct LpwstrDeleter {
    void operator()(WCHAR *p) const { delete[] p; }
};

//! Wrapper around a WCHAR pointer a.k.a. LPWStr to take care of memory handling
using ScopedLpwstr = std::unique_ptr<WCHAR, LpwstrDeleter>;

/*! \brief Converts a UTF-8 std::string to wide char
 *
 * Uses MultiByteToWideChar to convert the input string and wraps the result in
 * a ScopedLpwstr. Returns nullptr on failure.
 */
ScopedLpwstr utf8ToWideChar(const std::string &utf8) {
    int requiredBufSize = MultiByteToWideChar(
        CP_UTF8,
        0, // flags must be 0 for UTF-8
        utf8.c_str(),
        -1,      // rely on null-terminated input string
        nullptr, // no out-buffer needed
        0);      // return required buffer size; don't write to out-buffer

    // 0 means MultiByteToWideChar did not succeed. Note that even an empty
    // string yields 1 on success due to the terminating null character needed
    // in the out-buffer.
    if (requiredBufSize == 0) {
        return nullptr;
    }

    ScopedLpwstr lwstr(new WCHAR[requiredBufSize]);
    int bytesWritten = MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), -1, lwstr.get(), requiredBufSize);

    if (bytesWritten == 0) {
        return nullptr;
    }

    return lwstr;
}

/*! \brief Converts a wide char pointer to a std::string
 *
 * Note that this function provides no reliable indication of errors and simply
 * returns an empty string in case it fails.
 */
std::string wideCharToAnsi(LPWSTR wChar) {
    std::string result;
    if (wChar == nullptr) {
        return result;
    }

    int requiredBufSize = WideCharToMultiByte(
        CP_ACP,
        0, // flags
        wChar,
        -1,       // rely on null-terminated input string
        nullptr,  // no out-buffer needed
        0,        // return required buffer size; don't write to out-buffer
        nullptr,  // use system default for non representable characters
        nullptr); // unused output parameter

    // 0 indicates error; see comment in utf8ToWideChar.
    if (requiredBufSize == 0) {
        return result;
    }

    std::unique_ptr<char[]> buffer(new char[requiredBufSize]);
    int bytesWritten = WideCharToMultiByte(
        CP_ACP, 0, wChar, -1, buffer.get(), requiredBufSize, nullptr, nullptr);

    if (bytesWritten != 0) {
        result = std::string(buffer.get());
    }

    return result;
}

/*! /brief Get an explanatory message for an error code obtained via
 * ::GetLastError()
 */
std::string getErrorMessage(DWORD errorCode) {
    std::string errMsg;
    LPWSTR errBuffer = nullptr;
    auto written = ::FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr, // ignored for the flags we use
        errorCode,
        0, // figure out LANGID automatically
        reinterpret_cast<LPWSTR>(&errBuffer),
        0,        // figure out out-buffer size automatically
        nullptr); // no additional arguments

    if (written > 0 && errBuffer != nullptr) {
        errMsg = wideCharToAnsi(errBuffer);
        LocalFree(errBuffer);
    }
    return errMsg;
}

void updateError(OsKeychain::Error &err) {
    const auto code = ::GetLastError();
    if (code == ERROR_SUCCESS) {
        err = OsKeychain::Error{};
        return;
    }

    err.Message = getErrorMessage(code);
    err.Code = code;
    err.Type = err.Code == ERROR_NOT_FOUND ? OsKeychain::ErrorType::NotFound
                                           : OsKeychain::ErrorType::GenericError;
}

/*! /brief Create the target name used to lookup and store credentials
 *
 * The result is wrapped in a ScopedLpwstr. If `chunkSuffix` is non-empty it
 * is appended to the base target name — used to spread an oversized blob
 * across multiple Windows credentials.
 */
ScopedLpwstr makeTargetName(const std::string &package,
                            const std::string &service, const std::string &user,
                            const std::string &chunkSuffix,
                            OsKeychain::Error &err) {
    auto result = utf8ToWideChar(package + "." + service + '/' + user + chunkSuffix);
    if (!result) {
        updateError(err);

        // make really sure that we set an error code if we will return nullptr
        if (!err) {
            err.Type = OsKeychain::ErrorType::GenericError;
            err.Message = "Failed to create credential target name.";
            err.Code = -1; // generic non-zero
        }
    }

    return result;
}

ScopedLpwstr makeTargetName(const std::string &package,
                            const std::string &service, const std::string &user,
                            OsKeychain::Error &err) {
    return makeTargetName(package, service, user, std::string{}, err);
}

static std::string makeChunkSuffix(size_t chunkIndex) {
    return ".chunk" + std::to_string(chunkIndex);
}

static bool writeCredentialBlob(LPWSTR targetName, LPWSTR userName,
                                const char *data, size_t size,
                                OsKeychain::Error &err) {
    CREDENTIAL cred = {};
    cred.Type = kCredType;
    cred.TargetName = targetName;
    cred.UserName = userName;
    cred.CredentialBlobSize = static_cast<DWORD>(size);
    cred.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char *>(data));
    cred.Persist = CRED_PERSIST_ENTERPRISE;

    if (::CredWrite(&cred, 0) == FALSE) {
        updateError(err);
        return false;
    }
    return true;
}

// Delete chunks .chunkN, .chunk(N+1), ... until a NotFound is encountered.
// Used to clear leftover chunks after switching from a large to a smaller
// blob (or back to an unchunked write).
static void deleteChunksFrom(const std::string &package,
                             const std::string &service,
                             const std::string &user, size_t startIndex) {
    for (size_t i = startIndex;; ++i) {
        OsKeychain::Error scratch;
        auto chunkTarget =
            makeTargetName(package, service, user, makeChunkSuffix(i), scratch);
        if (scratch) return;
        if (::CredDelete(chunkTarget.get(), kCredType, 0) == FALSE) {
            // Stop on the first missing chunk — there cannot be more beyond it.
            if (::GetLastError() == ERROR_NOT_FOUND) return;
            // For any other failure, stop quietly — this is best-effort cleanup
            // and surfacing the error would mask the caller's primary result.
            return;
        }
    }
}

void OsKeychain::SetPassword(const std::string &package, const std::string &service,
                 const std::string &user, const std::string &password,
                 Error &err) {
    err = Error{};
    auto target_name = makeTargetName(package, service, user, err);
    if (err) {
        return;
    }

    ScopedLpwstr user_name(utf8ToWideChar(user));
    if (!user_name) {
        updateError(err);
        return;
    }

    if (password.size() > DWORD_MAX) {
        err.Type = ErrorType::PasswordTooLong;
        err.Message = "Password too long.";
        err.Code = -1; // generic non-zero
        return;
    }

    // Small enough to live in a single credential — preserves the original
    // on-disk layout for callers whose data fits.
    if (password.size() <= CRED_MAX_CREDENTIAL_BLOB_SIZE) {
        if (!writeCredentialBlob(target_name.get(), user_name.get(),
                                 password.data(), password.size(), err)) {
            return;
        }
        // Clear any chunks left over from a previous larger write.
        deleteChunksFrom(package, service, user, 0);
        return;
    }

    // Oversized blob: split across .chunk0, .chunk1, ... and drop the
    // unchunked credential so a subsequent read finds the chunked form.
    size_t chunkIndex = 0;
    size_t offset = 0;
    while (offset < password.size()) {
        size_t chunkSize = std::min<size_t>(CRED_MAX_CREDENTIAL_BLOB_SIZE,
                                            password.size() - offset);
        auto chunkTarget = makeTargetName(package, service, user,
                                          makeChunkSuffix(chunkIndex), err);
        if (err) return;

        if (!writeCredentialBlob(chunkTarget.get(), user_name.get(),
                                 password.data() + offset, chunkSize, err)) {
            return;
        }
        offset += chunkSize;
        ++chunkIndex;
    }

    // Best-effort cleanup of the old unchunked credential and any trailing
    // chunks from a previous, larger write.
    ::CredDelete(target_name.get(), kCredType, 0);
    deleteChunksFrom(package, service, user, chunkIndex);
}

std::string OsKeychain::GetPassword(const std::string &package, const std::string &service,
                        const std::string &user, Error &err) {
    err = Error{};
    std::string password;

    auto target_name = makeTargetName(package, service, user, err);
    if (err) {
        return password;
    }

    // Try the single-credential layout first.
    CREDENTIAL *cred = nullptr;
    if (::CredRead(target_name.get(), kCredType, 0, &cred) == TRUE) {
        password.assign(reinterpret_cast<char *>(cred->CredentialBlob),
                        cred->CredentialBlobSize);
        ::CredFree(cred);
        return password;
    }

    const DWORD firstErr = ::GetLastError();
    if (firstErr != ERROR_NOT_FOUND) {
        updateError(err);
        return password;
    }

    // Fall through to the chunked layout.
    for (size_t i = 0;; ++i) {
        auto chunkTarget =
            makeTargetName(package, service, user, makeChunkSuffix(i), err);
        if (err) return std::string{};

        CREDENTIAL *chunkCred = nullptr;
        if (::CredRead(chunkTarget.get(), kCredType, 0, &chunkCred) == TRUE) {
            password.append(reinterpret_cast<char *>(chunkCred->CredentialBlob),
                            chunkCred->CredentialBlobSize);
            ::CredFree(chunkCred);
            continue;
        }

        const DWORD chunkErr = ::GetLastError();
        if (chunkErr == ERROR_NOT_FOUND) {
            if (i == 0) {
                // No unchunked credential and no chunks — report NotFound.
                updateError(err);
            }
            return password;
        }

        updateError(err);
        return std::string{};
    }
}

void OsKeychain::DeletePassword(const std::string &package, const std::string &service,
                    const std::string &user, Error &err) {
    err = Error{};
    auto target_name = makeTargetName(package, service, user, err);
    if (err) {
        return;
    }

    bool deletedAny = false;
    DWORD lastErr = ERROR_NOT_FOUND;

    if (::CredDelete(target_name.get(), kCredType, 0) == TRUE) {
        deletedAny = true;
    } else {
        lastErr = ::GetLastError();
        if (lastErr != ERROR_NOT_FOUND) {
            updateError(err);
            return;
        }
    }

    for (size_t i = 0;; ++i) {
        auto chunkTarget =
            makeTargetName(package, service, user, makeChunkSuffix(i), err);
        if (err) return;

        if (::CredDelete(chunkTarget.get(), kCredType, 0) == TRUE) {
            deletedAny = true;
            continue;
        }
        lastErr = ::GetLastError();
        if (lastErr == ERROR_NOT_FOUND) break;
        updateError(err);
        return;
    }

    if (!deletedAny) {
        // Preserve the original behavior of reporting NotFound when there was
        // nothing to delete.
        ::SetLastError(ERROR_NOT_FOUND);
        updateError(err);
    }
}
