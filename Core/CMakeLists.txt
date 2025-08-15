set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
CPMAddPackage("gh:WohlSoft/LuaJIT#a5da8f4")
CPMAddPackage("gh:nlohmann/json#v3.11.3")
CPMAddPackage(
    URI "gh:civetweb/civetweb#5864b55"
    OPTIONS "CIVETWEB_ENABLE_ASAN OFF"
)
CPMAddPackage(
    URI "gh:pantor/inja#2d1f0d0"
    OPTIONS "INJA_USE_EMBEDDED_JSON FALSE"
)
CPMAddPackage(
    URI "gh:nih-at/libzip#542fc1c"
    OPTIONS "BUILD_SHARED_LIBS OFF"
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

# I recommend that you just manually install these yourself using your package manager. if you're on windows then use msys2
# CPMAddPackage("gh:sqlite/sqlite#version-3.49.0")
# CPMAddPackage("gh:curl/curl#curl-8_12_1")

find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)

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
    Source/Database/Statement.cpp
    Source/Database/Database.cpp
    Source/Database/DatabaseManager.cpp
    Source/Database/ContentImages.cpp
    Source/Auth.cpp
    Source/BaseConfig.cpp
    Source/Config.cpp
    Source/Log.cpp
    Source/Shell.cpp
    Source/RccServiceManager.cpp
    Source/RobloxClient.cpp
    Source/AssetRequest.cpp
    Source/HttpServer/HttpServer.cpp
    Source/HttpServer/Handler.cpp
    Source/HttpServer/WebHandler.cpp
    Source/HttpServer/Api/Roblox/AssetHandler.cpp
    Source/Roblox/DataModel/RobloxFile.cpp
    Source/Roblox/DataModel/XmlFormat/XmlRobloxFile.cpp
    Source/Roblox/DataModel/Tree/Instance.cpp
    Source/base64.cpp
)
target_include_directories(NoobWarrior.Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_link_libraries(NoobWarrior.Core PUBLIC -static-libgcc -static-libstdc++ sqlite3 curl libluajit nlohmann_json civetweb-c-library inja libzip::zip)
target_compile_features(NoobWarrior.Core PUBLIC cxx_std_23)