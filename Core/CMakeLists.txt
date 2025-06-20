cmake_minimum_required(VERSION 3.26)
project(NoobWarrior.Core)

set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cache/CPM")
include("${CMAKE_SOURCE_DIR}/CMake/CPM.cmake")

CPMAddPackage("gh:nlohmann/json#v3.11.3")
CPMAddPackage(
    URI "gh:civetweb/civetweb#5864b55"
    OPTIONS "CIVETWEB_ENABLE_ASAN OFF"
)
CPMAddPackage(
    URI "gh:pantor/inja#2d1f0d0"
    OPTIONS "INJA_USE_EMBEDDED_JSON FALSE"
)

# I recommend that you just manually install these yourself using your package manager. if you're on windows then use msys2
# CPMAddPackage("gh:sqlite/sqlite#version-3.49.0")
# CPMAddPackage("gh:curl/curl#curl-8_12_1")

find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)

add_library(NoobWarrior.Core STATIC)
target_sources(NoobWarrior.Core PRIVATE
    Source/NoobWarrior.cpp
    Source/Archive.cpp
    Source/ArchiveManager.cpp
    Source/Auth.cpp
    Source/Config.cpp
    Source/Log.cpp
    Source/Shell.cpp
    Source/Injector.cpp
    Source/RccServiceManager.cpp
    Source/AssetRequest.cpp
    Source/HttpServer/HttpServer.cpp
    Source/HttpServer/Handler.cpp
    Source/HttpServer/WebHandler.cpp
    Source/Roblox/DataModel/RobloxFile.cpp
    Source/Roblox/DataModel/XmlFormat/XmlRobloxFile.cpp
    Source/Roblox/DataModel/Tree/Instance.cpp
)
target_include_directories(NoobWarrior.Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_link_libraries(NoobWarrior.Core PUBLIC sqlite3 curl nlohmann_json civetweb-c-library inja)
target_compile_features(NoobWarrior.Core PUBLIC cxx_std_20)