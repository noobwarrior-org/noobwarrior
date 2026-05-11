find_package(OpenSSL REQUIRED)

CPMAddPackage(
    URI "gh:madler/zlib#v1.3.1"
    OPTIONS "ZLIB_BUILD_STATIC ON" "ZLIB_BUILD_SHARED OFF"
)

if (zlib_ADDED)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)

    set(ZLIB_FOUND TRUE)
    set(ZLIB_LIBRARIES ZLIB::ZLIB)
    set(ZLIB_INCLUDE_DIR ${zlib_SOURCE_DIR})
else()
    message(FATAL_ERROR "Could not add zlib")
endif()

CPMAddPackage(
    URI "gh:facebook/zstd#48c0ed7"
    OPTIONS "ZSTD_BUILD_STATIC ON"
)

CPMAddPackage(
    URI "gh:curl/curl#85a895c"
    OPTIONS
    "BUILD_CURL_EXE OFF"
    "BUILD_SHARED_LIBS OFF"
    "CURL_DISABLE_INSTALL ON"
    "CURL_ENABLE_EXPORT_TARGET OFF"
    "CURL_USE_OPENSSL ON"
    "CURL_USE_SCHANNEL ON"
    "CURL_USE_SECTRANSP ON"
    "CURL_USE_ZLIB OFF"
    "CURL_ZLIB OFF"
    "CURL_USE_LIBPSL OFF"
    "USE_LIBIDN2 OFF"
    "CURL_BROTLI OFF"
    "CURL_ZSTD OFF"
    "CURL_LDAP OFF"
    "CURL_LDAPS OFF"
    "USE_NGHTTP2 OFF"
    "CURL_DISABLE_LDAP ON"
    "CURL_DISABLE_LDAPS ON"
    "ZLIB_FOUND FALSE"
    "CURL_ZLIB_FOUND FALSE"
)

add_library(sqlite3 STATIC ${CMAKE_SOURCE_DIR}/ThirdParty/sqlite3/sqlite3.c)
target_include_directories(sqlite3
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/ThirdParty/sqlite3>
)
# we need this enabled for full text search
target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_FTS5=1)

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    CPMAddPackage(
        URI "gh:WohlSoft/LuaJIT#59d961b"
        OPTIONS "LUAJIT_DISABLE_SSE2 ON"
    )
else()
    CPMAddPackage(
        URI "gh:WohlSoft/LuaJIT#59d961b"
    )
endif()

CPMAddPackage(
    URI "gh:ThePhD/sol2#c1f95a7"
)

CPMAddPackage("gh:nlohmann/json#v3.11.3")

CPMAddPackage(
    URI "gh:libevent/libevent#a994a52"
    OPTIONS "EVENT__LIBRARY_TYPE STATIC" "EVENT__DISABLE_OPENSSL OFF"
)

CPMAddPackage(
    URI "gh:nih-at/libzip#e16526d"
    OPTIONS "BUILD_SHARED_LIBS OFF" "LIBZIP_DO_INSTALL OFF" "ENABLE_ZSTD OFF"
)

CPMAddPackage(
    URI "gh:zeux/pugixml#61c9448"
)

if (NOOBWARRIOR_LIBTYPE_STATIC)
    message(STATUS "Configured noobWarrior as a static library")
    add_library(NoobWarrior.Core STATIC)
else()
    message(STATUS "Configured noobWarrior as a shared library")
    add_library(NoobWarrior.Core SHARED)
endif()
set_target_properties(NoobWarrior.Core PROPERTIES
    OUTPUT_NAME "noobwarrior"
)
target_sources(NoobWarrior.Core PRIVATE
    Source/NoobWarrior.cpp
    Source/noobwarrior_c.cpp
    Source/Console/Console.cpp
    Source/Console/Command/Command.cpp
    Source/SqlDb/SqlDb.cpp
    Source/SqlDb/Statement.cpp
    Source/EmuDb/EmuDb.cpp
    Source/EmuDb/EmuDbManager.cpp
    Source/EmuDb/ContentImages.cpp
    Source/FileSystem/VirtualFileSystem.cpp
    Source/FileSystem/OverlayFileSystem.cpp
    Source/FileSystem/StdFileSystem.cpp
    Source/FileSystem/ZipFileSystem.cpp
    Source/Keychain/Keychain.cpp
    Source/Keychain/EmuKeychain.cpp
    Source/Keychain/MasterKeychain.cpp
    Source/Keychain/RbxKeychain.cpp
    Source/Lua/LuaState.cpp
    Source/Lua/LuaScript.cpp
    Source/Lua/LuaSignal.cpp
    Source/Lua/Lhp.cpp
    Source/BaseRegistry.cpp
    Source/Registry.cpp
    Source/Plugin.cpp
    Source/PluginManager.cpp
    Source/Log.cpp
    Source/Shell.cpp
    Source/Url.cpp
    Source/RccServiceManager.cpp
    Source/Engine.cpp
    Source/NetClient.cpp
    Source/Backup.cpp
    Source/HttpServer/Base/HttpServer.cpp
    Source/HttpServer/Base/Handler.cpp
    Source/HttpServer/Base/RootHandler.cpp
    Source/HttpServer/Base/TestHandler.cpp
    Source/HttpServer/Emulator/ServerEmulator.cpp
    Source/HttpServer/Emulator/ProcessPingHandler.cpp
    Source/HttpServer/Emulator/CreateAccountHandler.cpp
    Source/HttpServer/Emulator/RunningGameServersHandler.cpp
    Source/HttpServer/Emulator/AssetHandler.cpp
    Source/HttpServer/Emulator/AssetThumbnailJsonHandler.cpp
    Source/HttpServer/Emulator/ClientSettingsHandler.cpp
    Source/HttpServer/Emulator/StudioEditHandler.cpp
    Source/HttpServer/Emulator/NegotiateHandler.cpp
    Source/HttpServer/Emulator/PlaceLauncherHandler.cpp
    Source/HttpServer/Emulator/JoinScriptJsonHandler.cpp
    Source/HttpServer/Emulator/MySettingsJsonHandler.cpp
    Source/HttpServer/Emulator/AuthenticatedUserHandler.cpp
    Source/HttpServer/Emulator/CurrentUserHandler.cpp
    Source/HttpServer/Emulator/RequestAuthHandler.cpp
    Source/HttpServer/Emulator/GameIconHandler.cpp
    Source/Roblox/FileFormat/RobloxFile.cpp
    Source/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.cpp
    Source/Roblox/FileFormat/XmlFormat/XmlRobloxFile.cpp
    Source/Roblox/FileFormat/XmlFormat/XmlFileReader.cpp
    Source/Roblox/FileFormat/Tree/Instance.cpp
    Source/algorithm/base64.cpp
)
target_include_directories(NoobWarrior.Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_link_libraries(NoobWarrior.Core PUBLIC
    OpenSSL::SSL
    OpenSSL::Crypto
    zlibstatic
    libzstd_static
    libcurl
    sqlite3
    libluajit
    sol2
    nlohmann_json
    event_core
    event_extra
    event_openssl
    libzip::zip
    pugixml-static
)
target_compile_features(NoobWarrior.Core PUBLIC cxx_std_23)

# some code here borrowed from https://github.com/hrantzsch/keychain/blob/master/CMakeLists.txt
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    target_sources(NoobWarrior.Core PRIVATE Source/Keychain/OsKeychainWin.cpp)

    target_link_libraries(NoobWarrior.Core
        PRIVATE
            crypt32)
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    target_sources(NoobWarrior.Core PRIVATE Source/Keychain/OsKeychainMac.cpp)

    find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
    find_library(SECURITY_LIBRARY Security REQUIRED)

    target_link_libraries(NoobWarrior.Core
        PRIVATE
            ${COREFOUNDATION_LIBRARY}
            ${SECURITY_LIBRARY})
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_sources(NoobWarrior.Core PRIVATE Source/Keychain/OsKeychainLinux.cpp)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLIB2 IMPORTED_TARGET glib-2.0)
    pkg_check_modules(LIBSECRET IMPORTED_TARGET libsecret-1)

    target_link_libraries(NoobWarrior.Core
        PRIVATE
            PkgConfig::GLIB2
            PkgConfig::LIBSECRET)
else()
    target_sources(NoobWarrior.Core PRIVATE Source/Keychain/OsKeychainGeneric.cpp)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(NoobWarrior.Core PUBLIC -static-libgcc -static-libstdc++)
endif()
