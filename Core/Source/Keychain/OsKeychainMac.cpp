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
// File: OsKeychainMac.cpp
// Started by: Hattozo
// Started on: 2/11/2026
// Description: This file is based off https://github.com/hrantzsch/keychain/blob/master/src/keychain_mac.cpp
// You can read the appropriate MIT license by clicking the link

#include <type_traits>
#include <vector>

#include <Security/Security.h>

#include <NoobWarrior/Keychain/OsKeychain.h>

using namespace NoobWarrior;

/*! \brief Converts a CFString to a std::string
 *
 * This either uses CFStringGetCStringPtr or (if that fails) CFStringGetCString.
 */
std::string CFStringToStdString(const CFStringRef cfstring) {
    const char *ccstr = CFStringGetCStringPtr(cfstring, kCFStringEncodingUTF8);

    if (ccstr != nullptr) {
        return std::string(ccstr);
    }

    auto utf16Pairs = CFStringGetLength(cfstring);
    auto maxUtf8Bytes =
        CFStringGetMaximumSizeForEncoding(utf16Pairs, kCFStringEncodingUTF8);

    std::vector<char> cstr(maxUtf8Bytes, '\0');
    auto result = CFStringGetCString(
        cfstring, cstr.data(), cstr.size(), kCFStringEncodingUTF8);

    return result ? std::string(cstr.data()) : std::string();
}

//! \brief Extracts a human readable string from a status code
std::string errorStatusToString(OSStatus status) {
    const auto errorMessage = SecCopyErrorMessageString(status, NULL);
    std::string errorString;

    if (errorMessage) {
        errorString = CFStringToStdString(errorMessage);
        CFRelease(errorMessage);
    }

    return errorString;
}

std::string makeServiceName(const std::string &package,
                            const std::string &service) {
    return package + "." + service;
}

/*! \brief Update error information
 *
 * If status indicates an error condition, set message, code and error type.
 * Otherwise, set err to success.
 */
void updateError(OsKeychain::Error &err, OSStatus status) {
    if (status == errSecSuccess) {
        err = OsKeychain::Error{};
        return;
    }

    err.Message = errorStatusToString(status);
    err.Code = status;

    switch (status) {
    case errSecItemNotFound:
        err.Type = OsKeychain::ErrorType::NotFound;
        break;

    // potential errors in case the user needs to unlock the keychain first
    case errSecUserCanceled:        // user pressed the Cancel button
    case errSecAuthFailed:          // too many failed password attempts
    case errSecInteractionRequired: // user interaction required but not allowed
        err.Type = OsKeychain::ErrorType::AccessDenied;
        break;

    default:
        err.Type = OsKeychain::ErrorType::GenericError;
    }
}

void setGenericError(OsKeychain::Error &err, const std::string &errorMessage) {
    err = OsKeychain::Error{};
    err.Message = errorMessage;
    err.Type = OsKeychain::ErrorType::GenericError;
    err.Code = -1;
}

/*! \brief Helper to manage the lifetime of CF-Objects
 *
 * This helper will CFRelease the managed CF-Object when it goes out of scope.
 * It assumes ownership of the managed object, so users should own the object in
 * terms of the Core Foundation "Create Rule" when passing it to the
 * ScopedCFRef. Consequently, the object should also not be released by anyone
 * else, at least not without calling CFRetain first.
 * */
template <typename T,
          typename = typename std::enable_if<std::is_pointer<T>::value>::type>
class ScopedCFRef {
  public:
    explicit ScopedCFRef(T ref) : _ref(ref) {}
    ~ScopedCFRef() { _release(); }

    ScopedCFRef(ScopedCFRef &&other) noexcept : _ref(other._ref) {
        other._ref = nullptr;
    }
    ScopedCFRef &operator=(ScopedCFRef &&other) {
        if (this != &other) {
            _release();
            _ref = other._ref;
            other._ref = nullptr;
        }
        return *this;
    }

    ScopedCFRef(const ScopedCFRef &) = delete;
    ScopedCFRef &operator=(const ScopedCFRef &) = delete;

    const T get() const { return _ref; }
    operator bool() const { return _ref != nullptr; }

  private:
    void _release() {
        if (_ref != nullptr) {
            CFRelease(_ref);
            _ref = nullptr;
        }
    }

    T _ref;
};

ScopedCFRef<CFStringRef> createCFStringWithCString(const std::string &str,
                                                   OsKeychain::Error &err) {
    auto result = ScopedCFRef<CFStringRef>(CFStringCreateWithCString(
        kCFAllocatorDefault, str.c_str(), kCFStringEncodingUTF8));
    if (!result)
        setGenericError(err, "Failed to create CFString");
    return result;
}

ScopedCFRef<CFMutableDictionaryRef>
createCFMutableDictionary(OsKeychain::Error &err) {
    auto result = ScopedCFRef<CFMutableDictionaryRef>(
        CFDictionaryCreateMutable(kCFAllocatorDefault,
                                  0,
                                  &kCFTypeDictionaryKeyCallBacks,
                                  &kCFTypeDictionaryValueCallBacks));
    if (!result)
        setGenericError(err, "Failed to create CFMutableDictionary");
    return result;
}

ScopedCFRef<CFDataRef> createCFData(const std::string &data,
                                    OsKeychain::Error &err) {
    auto result = ScopedCFRef<CFDataRef>(
        CFDataCreate(kCFAllocatorDefault,
                     reinterpret_cast<const UInt8 *>(data.c_str()),
                     data.length()));
    if (!result)
        setGenericError(err, "Failed to create CFData");
    return result;
}

ScopedCFRef<CFMutableDictionaryRef> createQuery(const std::string &serviceName,
                                                const std::string &user,
                                                OsKeychain::Error &err) {
    const auto cfServiceName = createCFStringWithCString(serviceName, err);
    const auto cfUser = createCFStringWithCString(user, err);
    auto query = createCFMutableDictionary(err);

    if (err.Type != OsKeychain::ErrorType::NoError)
        return query;

    CFDictionaryAddValue(query.get(), kSecClass, kSecClassGenericPassword);
    CFDictionaryAddValue(query.get(), kSecAttrAccount, cfUser.get());
    CFDictionaryAddValue(query.get(), kSecAttrService, cfServiceName.get());

    return query;
}

void OsKeychain::SetPassword(const std::string &package, const std::string &service,
                 const std::string &user, const std::string &password,
                 Error &err) {
    err = Error{};
    const auto serviceName = makeServiceName(package, service);
    const auto cfPassword = createCFData(password, err);
    auto query = createQuery(serviceName, user, err);

    if (err.Type != OsKeychain::ErrorType::NoError)
        return;

    auto attributesToUpdate = createCFMutableDictionary(err);
    if (err.Type != OsKeychain::ErrorType::NoError)
        return;
    CFDictionaryAddValue(attributesToUpdate.get(), kSecValueData, cfPassword.get());

    OSStatus status = SecItemUpdate(query.get(), attributesToUpdate.get());

    if (status == errSecItemNotFound) {
        CFDictionaryAddValue(query.get(), kSecValueData, cfPassword.get());
        status = SecItemAdd(query.get(), NULL);
    }

    updateError(err, status);
}

std::string OsKeychain::GetPassword(const std::string &package, const std::string &service,
                        const std::string &user, Error &err) {
    err = Error{};
    const auto serviceName = makeServiceName(package, service);
    auto query = createQuery(serviceName, user, err);

    if (err.Type != OsKeychain::ErrorType::NoError)
        return "";

    CFDictionaryAddValue(query.get(), kSecReturnData, kCFBooleanTrue);

    CFTypeRef result = nullptr;
    updateError(err, SecItemCopyMatching(query.get(), &result));
    const auto cfPassword = ScopedCFRef<CFDataRef>((CFDataRef)result);

    if (!cfPassword || err.Type != OsKeychain::ErrorType::NoError)
        return "";

    return std::string(
        reinterpret_cast<const char *>(CFDataGetBytePtr(cfPassword.get())),
        CFDataGetLength(cfPassword.get()));
}

void OsKeychain::DeletePassword(const std::string &package, const std::string &service,
                    const std::string &user, Error &err) {
    err = Error{};
    const auto serviceName = makeServiceName(package, service);
    const auto query = createQuery(serviceName, user, err);

    if (err.Type != OsKeychain::ErrorType::NoError)
        return;

    updateError(err, SecItemDelete(query.get()));
}
