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
// File: OsKeychainGeneric.cpp
// Started by: Hattozo
// Started on: 2/11/2026
// Description: This file is somewhat based off the code found in https://github.com/hrantzsch/keychain/tree/master/src
// You can read the appropriate MIT license by browsing its repository
// This is a generic keychain wrapper for operating systems that we have not written a proper keychain wrapper for.
// It does not use any OS api's.
#include <NoobWarrior/Keychain/OsKeychain.h>

#include <map>
#include <string>

using namespace NoobWarrior;

typedef std::map<std::string, std::string> IdentifierPassword;

void OsKeychain::SetPassword(const std::string &package, const std::string &service,
                 const std::string &user, const std::string &password,
                 Error &err)
{
    
}

std::string OsKeychain::GetPassword(const std::string &package, const std::string &service,
                        const std::string &user, Error &err)
{
    return "";
}

void OsKeychain::DeletePassword(const std::string &package, const std::string &service,
                    const std::string &user, Error &err)
{

}