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
    Settings/AccountsPage.cpp
    Settings/ServerEmulatorPage.cpp
    Dialog/AssetDownloaderDialog.cpp
    Dialog/AboutDialog.cpp
    Dialog/AuthBaseDialog.cpp
    Dialog/AuthTokenDialog.cpp
    Dialog/DatabaseDialog.cpp
    Dialog/PlayerDialog.cpp
    Dialog/AvatarBackend.cpp
    Dialog/PluginDialog.cpp
    Dialog/SelectStudioVersionDialog.cpp
    ServerHost/HostServerDialog.cpp
    ServerHost/PlaceInfoCardWidget.cpp
    OnlineWindow/OnlineWindow.cpp
    OnlineWindow/OnlineSidebar.cpp
    OnlineWindow/ServerInformationSidebar.cpp
    OnlineWindow/ServerListWidget.cpp
    OnlineWindow/ProfilePage.cpp
    OnlineWindow/WorkshopPage.cpp
    OnlineWindow/MasterServerStore.cpp
    OnlineWindow/MasterHttp.cpp
    OnlineWindow/AddMasterServerDialog.cpp
    OnlineWindow/DirectConnectDialog.cpp
    OnlineWindow/ServerLoginDialog.cpp
    OnlineWindow/MasterLoginDialog.cpp
    Sdk/Sdk.cpp
    Sdk/WelcomeWidget.cpp
    Sdk/FileManagerWidget.cpp
    Sdk/ThumbnailPreviewWidget.cpp
    Sdk/ExecuteSqlDialog.cpp
    Sdk/CreatorInfoWidget.cpp
    Sdk/EngineTreeWidget.cpp
    Sdk/PluginTreeWidget.cpp
    Sdk/EmuDbListWidget.cpp
    Sdk/EmuDbComboBox.cpp
    Sdk/PluginListWidget.cpp
    Sdk/CodeEditorWidget.cpp
    Sdk/Item/Browser/ItemBrowserWidget.cpp
    Sdk/Item/Browser/ItemBrowserPage.cpp
    Sdk/Item/ItemOpenSaveDialog.cpp
    Sdk/Item/ItemWidget.cpp
    Sdk/Item/ItemListWidget.cpp
    Sdk/Item/ItemDialog.cpp
    Sdk/Item/ItemDialog_Impl_Asset.cpp
    Sdk/Item/ItemDialog_Impl_Badge.cpp
    Sdk/Item/ItemDialog_Impl_Bundle.cpp
    Sdk/Item/ItemDialog_Impl_DevProduct.cpp
    Sdk/Item/ItemDialog_Impl_Group.cpp
    Sdk/Item/ItemDialog_Impl_Outfit.cpp
    Sdk/Item/ItemDialog_Impl_Pass.cpp
    Sdk/Item/ItemDialog_Impl_Set.cpp
    Sdk/Item/ItemDialog_Impl_Universe.cpp
    Sdk/Item/ItemDialog_Impl_User.cpp
    Sdk/Item/UniverseTreeWidget.cpp
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
    Sdk/Project/Wizard/Template/EmuDbRobloxBackup.cpp
    Sdk/Studio/ExplorerWidget.cpp
    Sdk/Studio/PropertiesWidget.cpp
    Sdk/Studio/SourceEditorContainer.cpp
    Sdk/Studio/RobloxFilePreviewController.cpp
    Sdk/Studio/RobloxFilePreviewWindow.cpp
    Sdk/Project/Wizard/Template/PluginEmpty.cpp
    Sdk/Project/Wizard/Template/WipPage.cpp
    Sdk/Notification/Notification.cpp
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
target_link_libraries(NoobWarrior.QtGui PUBLIC NoobWarrior.Core NoobWarrior.QtShared Qt6::Core Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets)
target_include_directories(NoobWarrior.QtGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(NoobWarrior.QtGui PUBLIC cxx_std_23)
target_compile_definitions(NoobWarrior.QtGui PRIVATE POC_BUILD)

if (NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    target_link_options(NoobWarrior.QtGui PRIVATE -static-libgcc -static-libstdc++)
endif()

add_custom_target(CopyAppDistDir
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/AppDistribution ${CMAKE_CURRENT_BINARY_DIR}/
)
add_dependencies(NoobWarrior.QtGui CopyAppDistDir)