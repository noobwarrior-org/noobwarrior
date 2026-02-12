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
// File: OsKeychainLinux.cpp
// Started by: Hattozo
// Started on: 2/11/2026
// Description: This file is based off https://github.com/hrantzsch/keychain/blob/master/src/keychain_linux.cpp
// You can read the appropriate MIT license by clicking the link
#include <NoobWarrior/Keychain/OsKeychain.h>

#include <libsecret/secret.h>

using namespace NoobWarrior;

const char *ServiceFieldName = "service";
const char *AccountFieldName = "username";

// disable warnings about missing initializers in SecretSchema
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

SecretSchema makeSchema(const std::string &package) {
    return SecretSchema{package.c_str(),
                        SECRET_SCHEMA_NONE,
                        {
                            {ServiceFieldName, SECRET_SCHEMA_ATTRIBUTE_STRING},
                            {AccountFieldName, SECRET_SCHEMA_ATTRIBUTE_STRING},
                            {NULL, SecretSchemaAttributeType(0)},
                        }};
}

std::string makeLabel(const std::string &service, const std::string &user) {
    std::string label = service;

    if (!user.empty()) {
        label += " (" + user + ")";
    }

    return label;
}

void updateError(OsKeychain::Error &err, GError *error) {
    if (error == NULL) {
        err = OsKeychain::Error{};
        return;
    }

    err.Type = OsKeychain::ErrorType::GenericError;
    err.Message = error->message;
    err.Code = error->code;
    g_error_free(error);
}

void setErrorNotFound(OsKeychain::Error &err) {
    err.Type = OsKeychain::ErrorType::NotFound;
    err.Message = "Password not found.";
    err.Code = -1; // generic non-zero
}

void OsKeychain::SetPassword(const std::string &package, const std::string &service,
                 const std::string &user, const std::string &password,
                 Error &err) {
    err = Error{};
    const auto schema = makeSchema(package);
    const auto label = makeLabel(service, user);
    GError *error = NULL;

    secret_password_store_sync(&schema,
                               SECRET_COLLECTION_DEFAULT,
                               label.c_str(),
                               password.c_str(),
                               NULL, // not cancellable
                               &error,
                               ServiceFieldName,
                               service.c_str(),
                               AccountFieldName,
                               user.c_str(),
                               NULL);

    if (error != NULL) {
        updateError(err, error);
    }
}

std::string OsKeychain::GetPassword(const std::string &package, const std::string &service,
                        const std::string &user, Error &err) {
    err = Error{};
    const auto schema = makeSchema(package);
    GError *error = NULL;

    gchar *raw_passwords = secret_password_lookup_sync(&schema,
                                                       NULL, // not cancellable
                                                       &error,
                                                       ServiceFieldName,
                                                       service.c_str(),
                                                       AccountFieldName,
                                                       user.c_str(),
                                                       NULL);

    std::string password;

    if (error != NULL) {
        updateError(err, error);
    } else if (raw_passwords == NULL) {
        // libsecret reports no error if the password was not found
        setErrorNotFound(err);
    } else {
        password = raw_passwords;
        secret_password_free(raw_passwords);
    }

    return password;
}

void OsKeychain::DeletePassword(const std::string &package, const std::string &service,
                    const std::string &user, Error &err) {
    err = Error{};
    const auto schema = makeSchema(package);
    GError *error = NULL;

    bool deleted = secret_password_clear_sync(&schema,
                                              NULL, // not cancellable
                                              &error,
                                              ServiceFieldName,
                                              service.c_str(),
                                              AccountFieldName,
                                              user.c_str(),
                                              NULL);

    if (error != NULL) {
        updateError(err, error);
    } else if (!deleted) {
        // libsecret reports no error if the password did not exist
        setErrorNotFound(err);
    }
}
