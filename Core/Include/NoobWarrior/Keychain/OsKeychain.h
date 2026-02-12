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
// File: OsKeychain.h
// Started by: Hattozo
// Started on: 2/11/2026
// Description: This file is based off https://github.com/hrantzsch/keychain/blob/master/include/keychain/keychain.h
// You can read the appropriate MIT license by clicking the link
#pragma once
#include <string>

/*! \brief A thin wrapper to provide cross-platform access to the operating
 *         system's credentials storage.
 *
 * keychain provides the functions getPassword, setPassword, and deletePassword.
 *
 * All of these functions require three input parameters to identify the
 * credential that should be retrieved or manipulated: `package`, `service`, and
 * `user`. These identifiers will be mangled differently on each OS to
 * correspond to the OS API.
 * While none of the supported OSes has specific requirements to the format
 * identifiers, the reverse domain name format is recommended for the `package`
 * parameter in order to correspond with conventions.
 *
 * In addition, each function expects an instance of `keychain::Error` as an
 * output parameter to indicate success or failure. Note that previous states of
 * the Error are ignored and potentially overwritten.
 *
 * Also note that all three functions are blocking (potentially indefinitely)
 * for example if the OS prompts the user to unlock their credentials storage.
 */
namespace NoobWarrior::OsKeychain {

struct Error;

/*! \brief Retrieve a password
 *
 * \param package, service, user Used to identify the password to get
 * \param err Output parameter communicating success or error details
 *
 * \return The password, if the function was successful
 */
std::string GetPassword(const std::string &package, const std::string &service,
                        const std::string &user, Error &err);

/*! \brief Insert or update a password
 *
 * Existing passwords will be overwritten.
 *
 * \param package, service, user Used to identify the password to set
 * \param password The new password
 * \param err Output parameter communicating success or error details
 */
void SetPassword(const std::string &package, const std::string &service,
                 const std::string &user, const std::string &password,
                 Error &err);

/*! \brief Insert or update a password
 *
 * Trying to delete a password that does not exist will result in a NotFound
 * error.
 *
 * \param package, service, user Used to identify the password to delete
 * \param err Output parameter communicating success or error details
 */
void DeletePassword(const std::string &package, const std::string &service,
                    const std::string &user, Error &err);

enum class ErrorType {
    // update CATCH_REGISTER_ENUM in tests.cpp when changing this
    NoError = 0,
    GenericError,
    NotFound,
    // OS-specific errors
    PasswordTooLong = 10, // Windows only
    AccessDenied,         // macOS only
};

/*! \brief A struct to collect error information
 *
 * An instance of this struct is used as an output parameter to indicate success
 * or failure.
 */
struct Error {
    Error() : Type(ErrorType::NoError), Code(0) {}

    /*! \brief The type or reason of the error
     *
     * Note that some types of errors can only occur on certain platforms. In
     * cases where a platform-specific error occurs on one platform, both
     * NoError or some other (more generic) error might occur on others.
     */
    ErrorType Type;

    /*! \brief A human-readable explanatory error message
     *
     * In most cases this message is obtained from the operating system.
     */
    std::string Message;

    /*! \brief The "native" error code set by the operating system
     *
     * Even for the same type of error this value will differ across platforms.
     */
    int Code;

    //! \brief Checks if the error type is not NoError
    operator bool() const { return ErrorType::NoError != Type; }
};

}