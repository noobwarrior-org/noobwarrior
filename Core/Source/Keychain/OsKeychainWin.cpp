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

#include <memory>

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
 * The result is wrapped in a ScopedLpwstr.
 */
ScopedLpwstr makeTargetName(const std::string &package,
                            const std::string &service, const std::string &user,
                            OsKeychain::Error &err) {
    auto result = utf8ToWideChar(package + "." + service + '/' + user);
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

    if (password.size() > CRED_MAX_CREDENTIAL_BLOB_SIZE ||
        password.size() > DWORD_MAX) {
        err.Type = ErrorType::PasswordTooLong;
        err.Message = "Password too long.";
        err.Code = -1; // generic non-zero
        return;
    }

    CREDENTIAL cred = {};
    cred.Type = kCredType;
    cred.TargetName = target_name.get();
    cred.UserName = user_name.get();
    cred.CredentialBlobSize = static_cast<DWORD>(password.size());
    cred.CredentialBlob = (LPBYTE)(password.data());
    cred.Persist = CRED_PERSIST_ENTERPRISE;

    if (::CredWrite(&cred, 0) == FALSE) {
        updateError(err);
    }
}

std::string OsKeychain::GetPassword(const std::string &package, const std::string &service,
                        const std::string &user, Error &err) {
    err = Error{};
    std::string password;

    auto target_name = makeTargetName(package, service, user, err);
    if (err) {
        return password;
    }

    CREDENTIAL *cred;
    bool result = ::CredRead(target_name.get(), kCredType, 0, &cred);

    if (result == TRUE) {
        password = std::string(reinterpret_cast<char *>(cred->CredentialBlob),
                               cred->CredentialBlobSize);
        ::CredFree(cred);
    } else {
        updateError(err);
    }

    return password;
}

void OsKeychain::DeletePassword(const std::string &package, const std::string &service,
                    const std::string &user, Error &err) {
    err = Error{};
    auto target_name = makeTargetName(package, service, user, err);
    if (err) {
        return;
    }

    if (::CredDelete(target_name.get(), kCredType, 0) == FALSE) {
        updateError(err);
    }
}
