cmake_minimum_required(VERSION 3.26)
project(NoobWarrior.Core)

find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)

add_library(NoobWarrior.Core STATIC)
target_sources(NoobWarrior.Core PRIVATE
    Source/Archive.cpp
    Source/Auth.cpp
    Source/Config.cpp
    Source/Log.cpp
    Source/Shell.cpp
    Source/RccServiceInvoker.cpp
    Source/AssetRequest.cpp
    Source/HttpServer/HttpServer.cpp
    Source/RbxHttpServer/RbxHttpServer.cpp
    Source/Roblox/DataModel/RobloxFile.cpp
    Source/Roblox/DataModel/XmlFormat/XmlRobloxFile.cpp
    Source/Roblox/DataModel/Tree/Instance.cpp
)
target_include_directories(NoobWarrior.Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_include_directories(NoobWarrior.Core PRIVATE sqlite3 curl yaml-cpp::yaml-cpp nlohmann_json)
target_link_libraries(NoobWarrior.Core PRIVATE sqlite3 curl yaml-cpp::yaml-cpp nlohmann_json)
target_compile_features(NoobWarrior.Core PUBLIC cxx_std_20)