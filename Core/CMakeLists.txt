# install these with your package manager
# if you are on windows then install msys2
# if you are on mac then install homebrew
# if you are on linux and you don't have a package manager then i don't know what to tell you
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)

# packages that might not be included in some package managers are included here:
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
CPMAddPackage("gh:nlohmann/json#v3.11.3")
CPMAddPackage(
    URI "gh:libevent/libevent#a994a52"
    OPTIONS "EVENT__LIBRARY_TYPE STATIC"
)
CPMAddPackage(
    URI "gh:pantor/inja#2d1f0d0"
    OPTIONS "INJA_USE_EMBEDDED_JSON FALSE"
)
CPMAddPackage(
    URI "gh:madler/zlib#v1.3.1"
)
CPMAddPackage(
    URI "gh:nih-at/libzip#e16526d"
    OPTIONS "BUILD_SHARED_LIBS OFF" "LIBZIP_DO_INSTALL OFF" "ENABLE_ZSTD OFF"
)
CPMAddPackage(
    URI "gh:hrantzsch/keychain#8846e78"
)
CPMAddPackage(
    URI "gh:zeux/pugixml#61c9448"
)
CPMAddPackage(
    URI "gh:rm5248/libcppgenerate#6bf89d9"
)

#if (LuaJIT_ADDED)
#    FILE(GLOB LuaJIT_sources ${LuaJIT_SOURCE_DIR}/src/*.c)
#    list(REMOVE_ITEM LuaJIT_sources "${LuaJIT_SOURCE_DIR}/src/lua.c" "${LuaJIT_SOURCE_DIR}/src/luac.c")
#    add_library(LuaJIT STATIC ${LuaJIT_sources})
#
#    target_include_directories(LuaJIT
#            PUBLIC
#            $<BUILD_INTERFACE:${LuaJIT_SOURCE_DIR}/src>
#    )
#endif()

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
    Source/SqlDb/SqlDb.cpp
    Source/SqlDb/Statement.cpp
    Source/EmuDb/EmuDb.cpp
    Source/EmuDb/EmuDbManager.cpp
    Source/EmuDb/ContentImages.cpp
    Source/EmuDb/Repository/Item/AssetRepository.cpp
    Source/FileSystem/VirtualFileSystem.cpp
    Source/FileSystem/OverlayFileSystem.cpp
    Source/FileSystem/StdFileSystem.cpp
    Source/FileSystem/ZipFileSystem.cpp
    Source/Auth/NoobWarriorAuth.cpp
    Source/Auth/MasterServerAuth.cpp
    Source/Auth/ServerEmulatorAuth.cpp
    Source/Auth/RobloxAuth.cpp
    Source/Lua/LuaState.cpp
    Source/Lua/LuaScript.cpp
    Source/Lua/LuaHypertextPreprocessor.cpp
    Source/Lua/Bridge/LhpBridge.cpp
    Source/Lua/Bridge/LuaObjectBridge.cpp
    Source/Lua/Bridge/LuaScriptBridge.cpp
    Source/Lua/Bridge/PluginBridge.cpp
    Source/Lua/Bridge/HttpServerBridge.cpp
    Source/Lua/Bridge/VfsBridge.cpp
    Source/BaseConfig.cpp
    Source/Config.cpp
    Source/Plugin.cpp
    Source/PluginManager.cpp
    Source/Log.cpp
    Source/Shell.cpp
    Source/Url.cpp
    Source/RccServiceManager.cpp
    Source/RobloxClient.cpp
    Source/NetClient.cpp
    Source/Backup.cpp
    Source/HttpServer/Base/HttpServer.cpp
    Source/HttpServer/Base/Handler.cpp
    Source/HttpServer/Base/RootHandler.cpp
    Source/HttpServer/Base/TestHandler.cpp
    Source/HttpServer/Emulator/ServerEmulator.cpp
    Source/HttpServer/Emulator/AssetHandler.cpp
    Source/HttpServer/Emulator/ClientSettingsHandler.cpp
    Source/Roblox/FileFormat/RobloxFile.cpp
    Source/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.cpp
    Source/Roblox/FileFormat/XmlFormat/XmlRobloxFile.cpp
    Source/Roblox/FileFormat/XmlFormat/XmlFileReader.cpp
    Source/Roblox/FileFormat/Tree/Instance.cpp
    Source/base64.cpp
)
target_include_directories(NoobWarrior.Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_link_libraries(NoobWarrior.Core PUBLIC sqlite3 curl libluajit nlohmann_json event_core event_extra inja libzip::zip keychain pugixml-static cppgenerate_static)
target_compile_features(NoobWarrior.Core PUBLIC cxx_std_23)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(NoobWarrior.Core PUBLIC -static-libgcc -static-libstdc++)
endif()