cmake_minimum_required(VERSION 3.26)
project(NoobWarrior.Core)

find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)

add_library(NoobWarrior.Core STATIC)
target_sources(NoobWarrior.Core PRIVATE
    Source/Archive.cpp
    Source/Tools/DownloadAsset.cpp
    Source/Tools/SavePlaceAssets.cpp
    Source/Tools/PreserveGame.cpp
)
target_include_directories(NoobWarrior.Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_include_directories(NoobWarrior.Core PRIVATE sqlite3 curl)
target_link_libraries(NoobWarrior.Core PRIVATE sqlite3 curl)
target_compile_features(NoobWarrior.Core PUBLIC cxx_std_20)