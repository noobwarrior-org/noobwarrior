cmake_minimum_required(VERSION 3.26)
project(NoobWarrior.QtGui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

set(SOURCES
    main.cpp
    Application.cpp
    Application.h
    Launcher.cpp
    Launcher.h
    Settings.cpp
    Settings.h
    AssetDownloader.cpp
    AssetDownloader.h
    Dialog/AboutDialog.cpp
    Dialog/AboutDialog.h
    DatabaseEditor/DatabaseEditor.cpp
    DatabaseEditor/DatabaseEditor.h
    DatabaseEditor/OutlineWidget.cpp
    DatabaseEditor/OutlineWidget.h
    Ui/Launcher.ui
    Resources/resources.qrc
)

if (WIN32)
    set(SOURCES ${SOURCES} Resources/resources.rc)
endif()

qt_add_executable(NoobWarrior.QtGui ${SOURCES})
set_target_properties(NoobWarrior.QtGui PROPERTIES
    OUTPUT_NAME "noobwarrior"
#    WIN32_EXECUTABLE ON
)
target_include_directories(NoobWarrior.QtGui PRIVATE NoobWarrior.Core Qt6::Core Qt6::Widgets)
target_link_libraries(NoobWarrior.QtGui PRIVATE NoobWarrior.Core Qt6::Core Qt6::Widgets)
target_compile_features(NoobWarrior.QtGui PUBLIC cxx_std_20)