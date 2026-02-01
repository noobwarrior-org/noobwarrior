set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(SOURCES
    Application.cpp
    Launcher.cpp
    Style/DefaultStyle.cpp
    Settings/SettingsDialog.cpp
    Settings/SettingsPage.cpp
    Settings/GeneralPage.cpp
    Settings/DatabasePage.cpp
    Settings/PluginPage.cpp
    Settings/InstallationPage.cpp
    Settings/AccountPage.cpp
    Settings/HttpServerPage.cpp
    Dialog/AssetDownloaderDialog.cpp
    Dialog/AboutDialog.cpp
    Dialog/AuthBaseDialog.cpp
    Dialog/AuthTokenDialog.cpp
    Dialog/DatabaseDialog.cpp
    Dialog/PluginDialog.cpp
    ServerHost/HostServerDialog.cpp
    MasterServer/MasterServerWindow.cpp
    MasterServer/MasterServerSidebar.cpp
    MasterServer/ServerListWidget.cpp
    Sdk/Sdk.cpp
    Sdk/WelcomeWidget.cpp
    Sdk/OverviewWidget.cpp
    Sdk/FileManagerWidget.cpp
    Sdk/ThumbnailPreviewWidget.cpp
    Sdk/Browser/ItemBrowserWidget.cpp
    Sdk/Browser/ItemBrowserPage.cpp
    Sdk/Browser/AssetPage.cpp
    Sdk/Item/AssetDialog.cpp
    Sdk/Preview/ItemPreviewWidget.cpp
    Sdk/BackgroundTask/BackgroundTask.cpp
    Sdk/BackgroundTask/BackgroundTaskStatusBarWidget.cpp
    Sdk/BackgroundTask/BackgroundTaskPopupWidget.cpp
    Sdk/Backup/BackupDialog.cpp
    Sdk/Backup/BackupTask.cpp
    Sdk/Backup/BackupTreeView.cpp
    Sdk/Backup/ItemDescriptorModel.cpp
    Ui/Launcher.ui
    Resources/resources.qrc
)

if (WIN32)
    set(SOURCES ${SOURCES} Resources/resources.rc)
endif()

add_executable(NoobWarrior.QtGui)
target_sources(NoobWarrior.QtGui PRIVATE ${SOURCES})
set_target_properties(NoobWarrior.QtGui PROPERTIES
    OUTPUT_NAME "noobwarrior"
#    WIN32_EXECUTABLE ON
)
target_link_libraries(NoobWarrior.QtGui PUBLIC NoobWarrior.Core NoobWarrior.QtShared Qt6::Core Qt6::Widgets Qt6::Multimedia)
target_include_directories(NoobWarrior.QtGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(NoobWarrior.QtGui PUBLIC cxx_std_23)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(NoobWarrior.QtGui PUBLIC -static-libgcc -static-libstdc++)
endif()

add_custom_target(CopyAppDistDir
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/AppDistribution ${CMAKE_CURRENT_BINARY_DIR}/
)
add_dependencies(NoobWarrior.QtGui CopyAppDistDir)