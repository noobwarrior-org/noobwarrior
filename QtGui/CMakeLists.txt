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
    Sdk/FileManagerWidget.cpp
    Sdk/ThumbnailPreviewWidget.cpp
    Sdk/ExecuteSqlDialog.cpp
    Sdk/CreatorInfoWidget.cpp
    Sdk/EmuDbListWidget.cpp
    Sdk/PluginListWidget.cpp
    Sdk/Item/Browser/ItemBrowserWidget.cpp
    Sdk/Item/Browser/AssetPage.cpp
    Sdk/Item/Browser/BrowserItem.cpp
    Sdk/Item/ItemOpenSaveDialog.cpp
    Sdk/Item/ItemListWidget.cpp
    Sdk/Item/ItemDialog.cpp
    Sdk/Backup/BackupDialog.cpp
    Sdk/Backup/BackupTask.cpp
    Sdk/Backup/BackupTreeView.cpp
    Sdk/Backup/ItemDescriptorModel.cpp
    Sdk/Project/Project.cpp
    Sdk/Project/EmuDb/EmuDbProject.cpp
    Sdk/Project/EmuDb/Widget/OverviewWidget.cpp
    Sdk/Project/EmuDb/Widget/ItemPreviewWidget.cpp
    Sdk/Project/PluginProject.cpp
    Sdk/Project/Wizard/ProjectWizard.cpp
    Sdk/Project/Wizard/ProjectWizardIntroPage.cpp
    Sdk/Project/Wizard/Template/TemplatePage.cpp
    Sdk/Project/Wizard/Template/EmuDbEmpty.cpp
    Sdk/Project/Wizard/Template/EmuDbGameBackup.cpp
    Sdk/Project/Wizard/Template/PluginEmpty.cpp
    Sdk/BackgroundTask/BackgroundTask.cpp
    Sdk/BackgroundTask/BackgroundTaskStatusBarWidget.cpp
    Sdk/BackgroundTask/BackgroundTaskPopupWidget.cpp
    Ui/Launcher.ui
    Resources/resources.qrc
)

if (WIN32)
    set(SOURCES ${SOURCES} Resources/resources.rc)
endif()

# TODO: fix this
# CPMAddPackage(
#     URI "gh:KDE/extra-cmake-modules#b372037" # required by ktexteditor
#     OPTIONS "BUILD_DOC OFF" "BUILD_TESTING OFF"
# )

# CPMAddPackage(
#     URI "gh:KDE/ktexteditor#9f7def2"
# )

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