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

# Roblox's binary chunks are LZ4-compressed. The reference implementation only ever writes LZ4,
# which is also the oldest codec every supported engine understands, so the writer needs a real
# encoder rather than just the hand-rolled decoder.
CPMAddPackage(
    URI "gh:lz4/lz4#v1.10.0"
    SOURCE_SUBDIR "build/cmake"
    OPTIONS
    "LZ4_BUILD_CLI OFF"
    "LZ4_BUILD_LEGACY_LZ4C OFF"
    "BUILD_SHARED_LIBS OFF"
    "BUILD_STATIC_LIBS ON"
)

# use old ass curl if we on android cuz google sux
if (ANDROID)
    set(_NW_CURL_REV "curl-8_11_0")
    set(_NW_CURL_SCHANNEL    OFF)
    set(_NW_CURL_SECTRANSP   OFF)
    set(_NW_CURL_DISABLE_NTLM ON)
else()
    set(_NW_CURL_REV "85a895c")
    set(_NW_CURL_SCHANNEL    ON)
    set(_NW_CURL_SECTRANSP   ON)
    set(_NW_CURL_DISABLE_NTLM OFF)
endif()
CPMAddPackage(
    URI "gh:curl/curl#${_NW_CURL_REV}"
    OPTIONS
    "BUILD_CURL_EXE OFF"
    "BUILD_EXAMPLES OFF"
    "BUILD_TESTING OFF"
    "BUILD_SHARED_LIBS OFF"
    "CURL_DISABLE_INSTALL ON"
    "CURL_ENABLE_EXPORT_TARGET OFF"
    "CURL_USE_OPENSSL ON"
    "CURL_USE_SCHANNEL ${_NW_CURL_SCHANNEL}"
    "CURL_USE_SECTRANSP ${_NW_CURL_SECTRANSP}"
    "CURL_DISABLE_NTLM ${_NW_CURL_DISABLE_NTLM}"
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

CPMAddPackage(
    URI "gh:libcpr/cpr#1.11.2"
    OPTIONS
    "CPR_ENABLE_SSL ON"
    "CPR_BUILD_TESTS OFF"
    "BUILD_SHARED_LIBS OFF"
    "CMAKE_SKIP_INSTALL_RULES ON"
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

# Studio 0.719's Player deliberately gathers relay candidates only. Core owns
# the lightweight TURN server, its public advertised address and short-lived
# credentials; LocalRCC remains the version-specific WebRTC/RemoteEvent bridge.
# The relay is currently Windows-only. libjuice is MPL-2.0.
if (WIN32)
    CPMAddPackage(
        NAME noobwarrior_libjuice
        GITHUB_REPOSITORY "paullouisageneau/libjuice"
        GIT_TAG "5948a4162d37bc213d6051b67ee2876ccc5a99a6"
        # This also changes CPM's source-cache key, bypassing any incomplete
        # checkout left by an interrupted configure.
        GIT_SHALLOW FALSE
        DOWNLOAD_ONLY YES
    )
    if (NOT EXISTS "${noobwarrior_libjuice_SOURCE_DIR}/src/addr.c" OR
        NOT EXISTS "${noobwarrior_libjuice_SOURCE_DIR}/include/juice/juice.h")
        message(FATAL_ERROR
            "libjuice CPM checkout is incomplete at "
            "${noobwarrior_libjuice_SOURCE_DIR}; clear only that package's "
            "CPM cache entry and configure again")
    endif()
    find_package(Threads REQUIRED)

    # Roblox's 0.719 TURN client starts a replacement allocation after a 438
    # stale-nonce response instead of retrying the existing allocation's
    # Refresh request. libjuice rotates its nonce key at the same ten-minute
    # cadence as allocations, so that replacement receives 437 Allocation
    # Mismatch while the old allocation is still alive and the established ICE
    # path subsequently closes. This relay already uses fixed LAN-only test
    # credentials, so keep its nonce stable for the relay process lifetime.
    # libjuice has no public nonce-lifetime option. Force-include a compatibility
    # header for server.c only, leaving both the pinned source and CPM cache
    # untouched.
    set(noobwarrior_voice_juice_generated_dir
        "${CMAKE_CURRENT_BINARY_DIR}/Generated/libjuice")
    file(MAKE_DIRECTORY "${noobwarrior_voice_juice_generated_dir}")
    file(READ "${noobwarrior_libjuice_SOURCE_DIR}/src/server.h"
        noobwarrior_voice_juice_server_header)
    set(noobwarrior_voice_juice_nonce_definition
        "#define SERVER_NONCE_KEY_LIFETIME 600 * 1000")
    string(FIND "${noobwarrior_voice_juice_server_header}"
        "${noobwarrior_voice_juice_nonce_definition}"
        noobwarrior_voice_juice_nonce_definition_offset)
    if (noobwarrior_voice_juice_nonce_definition_offset EQUAL -1)
        message(FATAL_ERROR
            "Pinned libjuice server nonce implementation changed; update the local TURN compatibility patch")
    endif()
    set(noobwarrior_voice_juice_compat_header
        "${noobwarrior_voice_juice_generated_dir}/server_compat.h")
    file(WRITE
        "${noobwarrior_voice_juice_compat_header}"
        "#include \"server.h\"\n"
        "#undef SERVER_NONCE_KEY_LIFETIME\n"
        "#define SERVER_NONCE_KEY_LIFETIME (INT64_MAX / 2)\n")
    set(noobwarrior_voice_juice_server_source
        "${noobwarrior_libjuice_SOURCE_DIR}/src/server.c")

    add_library(noobwarrior_voice_juice STATIC
        ${noobwarrior_libjuice_SOURCE_DIR}/src/addr.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/agent.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/base64.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/conn.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/conn_mux.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/conn_poll.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/conn_thread.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/const_time.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/crc32.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/hash.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/hmac.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/ice.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/juice.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/log.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/random.c
        ${noobwarrior_voice_juice_server_source}
        ${noobwarrior_libjuice_SOURCE_DIR}/src/stun.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/tcp.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/timestamp.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/turn.c
        ${noobwarrior_libjuice_SOURCE_DIR}/src/udp.c
    )
    target_include_directories(noobwarrior_voice_juice
        PUBLIC ${noobwarrior_libjuice_SOURCE_DIR}/include
        PRIVATE
            ${noobwarrior_libjuice_SOURCE_DIR}/include/juice
            ${noobwarrior_libjuice_SOURCE_DIR}/src
    )
    target_compile_definitions(noobwarrior_voice_juice
        PUBLIC JUICE_STATIC
        PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
            USE_NETTLE=0
            $<$<CONFIG:Release>:RELEASE=1>
    )
    target_link_libraries(noobwarrior_voice_juice PRIVATE
        Threads::Threads
        ws2_32
        bcrypt
    )
    set_property(TARGET noobwarrior_voice_juice PROPERTY
        POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET noobwarrior_voice_juice PROPERTY C_STANDARD 11)
    if (MSVC)
        set_property(SOURCE "${noobwarrior_voice_juice_server_source}"
            APPEND PROPERTY COMPILE_OPTIONS
            "/FI${noobwarrior_voice_juice_compat_header}")
    else()
        set_property(SOURCE "${noobwarrior_voice_juice_server_source}"
            APPEND PROPERTY COMPILE_OPTIONS
            "-include" "${noobwarrior_voice_juice_compat_header}")
    endif()
endif()

CPMAddPackage(
    URI "gh:libevent/libevent#a994a52"
    OPTIONS
    "EVENT__LIBRARY_TYPE STATIC"
    "EVENT__DISABLE_OPENSSL OFF"
    "EVENT__DISABLE_TESTS ON"
    "EVENT__DISABLE_REGRESS ON"
    "EVENT__DISABLE_SAMPLES ON"
)

CPMAddPackage(
    URI "gh:nih-at/libzip#e16526d"
    OPTIONS
    "BUILD_SHARED_LIBS OFF"
    "LIBZIP_DO_INSTALL OFF"
    "ENABLE_ZSTD OFF"
    "BUILD_TOOLS OFF"
    "BUILD_REGRESS OFF"
    "BUILD_EXAMPLES OFF"
    "BUILD_DOC OFF"
    "BUILD_OSSFUZZ OFF"
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
    Source/Console/Command/HelpCommand.cpp
    Source/Console/Command/ExitCommand.cpp
    Source/Console/Command/FuncCommand.cpp
    Source/Console/Command/StudioCommand.cpp
    Source/SqlDb/SqlDb.cpp
    Source/SqlDb/Statement.cpp
    Source/EmuDb/EmuDb.cpp
    Source/EmuDb/EmuDbManager.cpp
    Source/EmuDb/AssetEnricher.cpp
    Source/EmuDb/ContentImages.cpp
    Source/FileSystem/VirtualFileSystem.cpp
    Source/FileSystem/OverlayFileSystem.cpp
    Source/FileSystem/StdFileSystem.cpp
    Source/FileSystem/ZipFileSystem.cpp
    Source/FileSystem/DatabaseFileSystem.cpp
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
    Source/PluginDataModel.cpp
    Source/PluginTreeMaterializer.cpp
    Source/PluginManager.cpp
    Source/Log.cpp
    Source/Date.cpp
    Source/Shell.cpp
    Source/Url.cpp
    Source/RccServiceManager.cpp
    Source/Engine.cpp
    Source/PeFile.cpp
    Source/Backup.cpp
    Source/HttpServer/Base/HttpServer.cpp
    Source/HttpServer/Base/Handler.cpp
    Source/HttpServer/Base/RootHandler.cpp
    Source/HttpServer/Base/TestHandler.cpp
    Source/HttpServer/Emulator/ServerEmulator.cpp
    Source/HttpServer/Emulator/EmulatorProxy.cpp
    Source/HttpServer/Emulator/AuthUtil.cpp
    Source/HttpServer/Emulator/AuthInfoHandler.cpp
    Source/HttpServer/Emulator/ProcessPingHandler.cpp
    Source/HttpServer/Emulator/StaticJsonHandler.cpp
    Source/HttpServer/Emulator/EmuIconHandler.cpp
    Source/HttpServer/Emulator/EmuDbGamesHandler.cpp
    Source/HttpServer/Emulator/CreateAccountHandler.cpp
    Source/HttpServer/Emulator/EmuLoginHandler.cpp
    Source/HttpServer/Emulator/EmuLogoutHandler.cpp
    Source/HttpServer/Emulator/LoginHandler.cpp
    Source/HttpServer/Emulator/RunningGameServersHandler.cpp
    Source/HttpServer/Emulator/TeleportAuthorizeHandler.cpp
    Source/Video/VideoTranscoder.cpp
    Source/HttpServer/Emulator/AssetHandler.cpp
    Source/HttpServer/Emulator/VideoHandler.cpp
    Source/HttpServer/Emulator/AssetBatchHandler.cpp
    Source/HttpServer/Emulator/AssetThumbnailJsonHandler.cpp
    Source/HttpServer/Emulator/ClientSettingsHandler.cpp
    Source/HttpServer/Emulator/ClientSettingsV2StudioHandler.cpp
    Source/HttpServer/Emulator/ClientSettingsV2DesktopHandler.cpp
    Source/HttpServer/Emulator/StudioEditHandler.cpp
    Source/HttpServer/Emulator/StudioOpenPlaceHandler.cpp
    Source/HttpServer/Emulator/NegotiateHandler.cpp
    Source/HttpServer/Emulator/PlaceLauncherHandler.cpp
    Source/HttpServer/Emulator/JoinScriptJsonHandler.cpp
    Source/HttpServer/Emulator/GameJoinHandler.cpp
    Source/HttpServer/Emulator/AuthTicketRedeemHandler.cpp
    Source/HttpServer/Emulator/AvatarFetchHandler.cpp
    Source/HttpServer/Emulator/AvatarOverrideHandler.cpp
    Source/HttpServer/Emulator/AppBehaviorsHandler.cpp
    Source/HttpServer/Emulator/AvatarHandler.cpp
    Source/HttpServer/Emulator/AvatarAppearance.cpp
    Source/HttpServer/Emulator/AvatarRulesHandler.cpp
    Source/HttpServer/Emulator/AppLaunchInfoHandler.cpp
    Source/HttpServer/Emulator/RolesHandler.cpp
    Source/HttpServer/Emulator/LocalesHandler.cpp
    Source/HttpServer/Emulator/GamesHandler.cpp
    Source/HttpServer/Emulator/GameStartInfoHandler.cpp
    Source/HttpServer/Emulator/UserChannelHandler.cpp
    Source/HttpServer/Emulator/UserProfilesHandler.cpp
    Source/HttpServer/Emulator/PlaceDetailsHandler.cpp
    Source/HttpServer/Emulator/PlaceUniverseHandler.cpp
    Source/HttpServer/Emulator/MarketplaceProductInfoHandler.cpp
    Source/HttpServer/Emulator/ToolboxServiceHandler.cpp
    Source/HttpServer/Emulator/IdeToolboxHandler.cpp
    Source/HttpServer/Emulator/DevelopHandler.cpp
    Source/HttpServer/Emulator/DataUploadHandler.cpp
    Source/HttpServer/Emulator/IdePublishHandler.cpp
    Source/HttpServer/Emulator/DataStorePersistenceHandler.cpp
    Source/HttpServer/Emulator/ThumbnailHandler.cpp
    Source/HttpServer/Emulator/AssetPermissionsHandler.cpp
    Source/HttpServer/Emulator/OmniRecHandler.cpp
    Source/HttpServer/Emulator/GamesListHandler.cpp
    Source/HttpServer/Emulator/GamesSortsHandler.cpp
    Source/HttpServer/Emulator/OAuthDiscoveryHandler.cpp
    Source/HttpServer/Emulator/OAuthTokenHandler.cpp
    Source/HttpServer/Emulator/OAuthUserInfoHandler.cpp
    Source/HttpServer/Emulator/StudioLoginHandler.cpp
    Source/HttpServer/Emulator/ClientVersionStudioHandler.cpp
    Source/HttpServer/Emulator/StudioPbeHandler.cpp
    Source/HttpServer/Emulator/GuacBundlesStudioHandler.cpp
    Source/HttpServer/Emulator/UserModerationHandler.cpp
    Source/HttpServer/Emulator/VoiceChatHandler.cpp
    Source/HttpServer/Emulator/SignalRCoreHandler.cpp
    Source/HttpServer/Emulator/UniversalAppConfigStudioHandler.cpp
    Source/HttpServer/Emulator/MySettingsJsonHandler.cpp
    Source/HttpServer/Emulator/AuthenticatedUserHandler.cpp
    Source/HttpServer/Emulator/SessionCheckHandler.cpp
    Source/HttpServer/Emulator/AvatarSetHandler.cpp
    Source/HttpServer/Emulator/AvatarCatalogHandler.cpp
    Source/HttpServer/Emulator/AvatarThumbnailHandler.cpp
    Source/HttpServer/Emulator/CurrentUserHandler.cpp
    Source/HttpServer/Emulator/RequestAuthHandler.cpp
    Source/HttpServer/Emulator/GameIconHandler.cpp
    Source/HttpServer/Emulator/OAuthAuthorizeHandler.cpp
    Source/Roblox/DataType/BrickColor.cpp
    Source/Roblox/DataType/Color3.cpp
    Source/Roblox/FileFormat/RobloxFile.cpp
    Source/Roblox/FileFormat/StudioServerPlace.cpp
    Source/Roblox/FileFormat/StudioServerPlaceCache.cpp
    Source/Roblox/FileFormat/BinaryFormat/BinaryFileChunk.cpp
    Source/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.cpp
    Source/Roblox/FileFormat/BinaryFormat/BinaryFileReader.cpp
    Source/Roblox/FileFormat/BinaryFormat/BinaryFileWriter.cpp
    Source/Roblox/FileFormat/BinaryFormat/Chunks/INST.cpp
    Source/Roblox/FileFormat/BinaryFormat/Chunks/META.cpp
    Source/Roblox/FileFormat/BinaryFormat/Chunks/PRNT.cpp
    Source/Roblox/FileFormat/BinaryFormat/Chunks/PROP.cpp
    Source/Roblox/FileFormat/BinaryFormat/Chunks/SIGN.cpp
    Source/Roblox/FileFormat/BinaryFormat/Chunks/SSTR.cpp
    Source/Roblox/FileFormat/XmlFormat/XmlFileReader.cpp
    Source/Roblox/FileFormat/XmlFormat/XmlFileWriter.cpp
    Source/Roblox/FileFormat/XmlFormat/XmlRobloxFile.cpp
    Source/Roblox/FileFormat/Tree/RbxObject.cpp
    Source/Roblox/FileFormat/Tree/Instance.cpp
    Source/algorithm/base64.cpp
)
target_include_directories(NoobWarrior.Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_link_libraries(NoobWarrior.Core PUBLIC
    OpenSSL::SSL
    OpenSSL::Crypto
    zlibstatic
    libzstd_static
    lz4_static
    libcurl
    cpr::cpr
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
if (WIN32)
    target_link_libraries(NoobWarrior.Core PRIVATE
        noobwarrior_voice_juice
        iphlpapi)
    target_compile_definitions(NoobWarrior.Core PRIVATE
        NOOBWARRIOR_VOICE_TURN_RELAY=1)
endif()
target_compile_features(NoobWarrior.Core PUBLIC cxx_std_23)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Include/NoobWarrior/Roblox/FileFormat/Generated/Classes.cpp")
    # this file is auto-generated. check if it exists so that we don't end up in a catch 22 where
    # we need the generator to work but cmake doesn't let us do it because this file doesn't exist yet
    target_sources(NoobWarrior.Core PRIVATE Include/NoobWarrior/Roblox/FileFormat/Generated/Classes.cpp)
    # Lets the rest of Core ask the API dump for class and property facts, while still building
    # when nobody has run the generator yet.
    target_compile_definitions(NoobWarrior.Core PUBLIC NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API)
endif()

if (ANDROID)
    # TODO: replace with an Android Keystore-backed keychain (OsKeychainAndroid.cpp).
    target_sources(NoobWarrior.Core PRIVATE
        Source/Keychain/OsKeychainGeneric.cpp
        Source/JniBridge.cpp)

    target_link_libraries(NoobWarrior.Core
        PRIVATE
            event_pthreads
            log
            android)
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
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
            event_pthreads
            ${COREFOUNDATION_LIBRARY}
            ${SECURITY_LIBRARY})
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    target_sources(NoobWarrior.Core PRIVATE Source/Keychain/OsKeychainLinux.cpp)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLIB2 IMPORTED_TARGET glib-2.0)
    pkg_check_modules(LIBSECRET IMPORTED_TARGET libsecret-1)

    target_link_libraries(NoobWarrior.Core
        PRIVATE
            event_pthreads
            PkgConfig::GLIB2
            PkgConfig::LIBSECRET)
else()
    target_sources(NoobWarrior.Core PRIVATE Source/Keychain/OsKeychainGeneric.cpp)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(NoobWarrior.Core PUBLIC -static-libgcc -static-libstdc++)
endif()
