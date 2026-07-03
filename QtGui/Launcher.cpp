/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: Launcher.cpp
// Started by: Hattozo
// Started on: 12/15/2024
// Description: This is the startup window that appears when the user launches the application.
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Macros.h>

#include "Launcher.h"
#include "Application.h"
#include "Dialog/AboutDialog.h"
#include "Dialog/AssetDownloaderDialog.h"
#include "Dialog/DatabaseDialog.h"
#include "Dialog/PluginDialog.h"
#include "Dialog/SelectStudioVersionDialog.h"
#include "ServerHost/HostServerDialog.h"

#include <memory>

#include <QLabel>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>
#include <QDate>
#include <QDesktopServices>

#define ADD_BUTTONS(arr) for (int i = 0; i < NOOBWARRIOR_ARRAY_SIZE(arr); i++) { \
    auto *button = new QPushButton(this); \
    button->setText((char*)arr[i][0]); \
    button->setIcon(QIcon((const char*)arr[i][2])); \
    button->setStyleSheet("text-align: left;"); \
    QObject::connect(button, &QPushButton::clicked, [&, i]() { if (arr[i][1] != nullptr) ((void(*)(Launcher&))arr[i][1])(*this); else QMessageBox::critical(this, "Error", "This function is currently not supported!"); }); \
    frameGrid->addWidget(button); \
}

#define HANDLE_QDIALOG(ptr, qdialog) if (!ptr) { \
    ptr = new qdialog(nullptr); \
    ptr->setAttribute(Qt::WA_DeleteOnClose); /* Ensure deletion on close */ \
    QObject::connect(ptr, &QWidget::destroyed, [&]() { \
        ptr = nullptr; \
    }); \
    ptr->show(); \
} else { \
    ptr->raise(); /* Bring to front if already open */ \
    ptr->activateWindow(); /* Make active */ \
}

using namespace NoobWarrior;

// static void ShowStartGame(Launcher &launcher) { gApp->LaunchClient({ .NoobWarriorVersion = 1, .Type = ClientType::Server, .Hash = "07b64feec0bd47c1", .Version = "0.463.0.417004" }); }
static void ShowStartGame(Launcher &launcher) { HANDLE_QDIALOG(launcher.mHostServerDialog, HostServerDialog) }
static void ShowJoinServer(Launcher &launcher) {
    if (!gApp->GetCore()->GetRegistry()->GetKeyValue<bool>("gui.acknowledged_online_disclaimer").value_or(false)) {
        QString msg =
            "Caution: Online play is unrated\n\n"
            "This message will show only once.\n\n"
            "All noobWarrior servers are decentralized and are potentially ran by strangers that you don't know. "
            "The creators of " NOOBWARRIOR_BRAND " are not responsible for whatever you "
            "may see on these servers. We are not a law enforcement entity and we have no agency to "
            "moderate any servers that you join. We advise you to take caution.\n\n"
            "If you agree to these statements, click Yes to open this window. Otherwise, click No.";
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Disclaimer");
        msgBox.setText(msg);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        QPointer<QAbstractButton> yes = msgBox.button(QMessageBox::Yes);
        if (yes) {
            yes->setEnabled(false);
            yes->setText("Yes (15)");
        }

        QTimer *timer = new QTimer(&msgBox);
        auto counter = std::make_shared<int>(15);

        launcher.connect(timer, &QTimer::timeout, [yes, timer, counter]() {
            --*counter;
            if (yes)
                yes->setText(QString("Yes (%1)").arg(QString::number(*counter)));

            if (*counter == 0) {
                timer->stop();

                if (yes) {
                    yes->setText("Yes");
                    yes->setEnabled(true);
                }
            }
        });

        timer->start(1000);
        int res = msgBox.exec();
        if (res == QMessageBox::Yes) {
            gApp->GetCore()->GetRegistry()->SetKeyValue<bool>("gui.acknowledged_online_disclaimer", true);
        } else {
            return;
        }
    }
    HANDLE_QDIALOG(launcher.mOnlineWindow, OnlineWindow)
}
static void ShowAboutDialog(Launcher &launcher) { HANDLE_QDIALOG(launcher.mAboutDialog, AboutDialog) }
static void ShowSettings(Launcher &launcher) { HANDLE_QDIALOG(launcher.mSettings, SettingsDialog) }
static void LaunchDatabaseEditor(Launcher& launcher) { HANDLE_QDIALOG(launcher.mSdk, Sdk) }
static void ShowDatabaseMenu(Launcher &launcher) { HANDLE_QDIALOG(launcher.mDatabaseDialog, DatabaseDialog) }
static void ShowPluginMenu(Launcher &launcher) { HANDLE_QDIALOG(launcher.mPluginDialog, PluginDialog) }
static void ShowLocalPlayer(Launcher &launcher) { HANDLE_QDIALOG(launcher.mPlayerDialog, PlayerDialog) }
static void ShowDownloadAssetDialog(Launcher &launcher) { HANDLE_QDIALOG(launcher.mAssetDownload, AssetDownloader) }
static void LaunchOfflineStudio(Launcher &launcher) {
    SelectStudioVersionDialog dialog;
    dialog.exec();
    /*
    gApp->LaunchEngine({
        .Engine = {
            .Architecture = EngineArchitecture::x86_64,
            .Type = EngineType::Roblox,
            .Side = EngineSide::Studio,
            // .Hash = "ef266da340bc4058",
            // .Version = "0.463.0.417004"
            .Hash = "c2e4d104afaf449c",
            .Version = "0.574.0.5740446"
        },
        .Ip = "",
        .Port = std::nullopt,
        .PlaceId = std::nullopt
    });*/
}

static const char* sCategoryNames[] = {
    "Play",
    "Developer Tools",
    "Application"
};

static const void* sPlay[][3] = {
    {"Online", (void*)&ShowJoinServer, ":/images/silk/world.png"},
    {"Start Game Server", (void*)&ShowStartGame, ":/images/silk/controller.png"},
};

static const void* sTools[][3] = {
    {"Launch SDK", (void*)&LaunchDatabaseEditor, ":/images/sdk.png"},
    {"Launch Studio", (void*)&LaunchOfflineStudio, ":/images/silk/application_side_tree.png"}
    // {"Download Asset(s)", (void*)&ShowDownloadAssetDialog, ":/images/silk/page_save.png"},
    // {"Model/Place Explorer", nullptr, ":/images/silk/bricks.png"},
    // {"Scan Roblox Clients", nullptr, ":/images/silk/drive_magnify.png"},
    // {"Scan Roblox Cache", nullptr, ":/images/silk/folder_magnify.png"}
};

static const void* sApplication[][3] = {
    // WIP, uncomment these when they are completed for later
    // {"Shell", nullptr, ":/images/silk/application_xp_terminal.png"},
    // {"Lua Shell", nullptr, ":/images/lua16.png"},
    {"Databases", (void*)&ShowDatabaseMenu, ":/images/silk/database.png"},
    {"Plugins", (void*)&ShowPluginMenu, ":/images/silk/plugin.png"},
    {"Player", (void*)&ShowLocalPlayer, ":/images/silk/user.png"},
    {"Settings", (void*)&ShowSettings, ":/images/silk/cog.png"},
    {"About", (void*)&ShowAboutDialog, ":/images/silk/help.png"}
};

Launcher::Launcher(QWidget *parent) : QDialog(parent),
    mAboutDialog(nullptr),
    mSettings(nullptr),
    mSdk(nullptr),
    mAssetDownload(nullptr),
    mHostServerDialog(nullptr),
    mOnlineWindow(nullptr),
    mDatabaseDialog(nullptr),
    mPluginDialog(nullptr),
    mPlayerDialog(nullptr)
{
    // ui->setupUi(this);
    setWindowTitle("noobWarrior");

    Layout = new QVBoxLayout(this);
    Layout->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(Layout);

    if (QDate::currentDate() == QDate(QDate::currentDate().year(), 4, 1)) {
        // APRIL FOOLS!!!!!!!
        auto *label = new QLabel();
        label->setStyleSheet("QLabel { background-color: red; color: white; }");
        label->setTextFormat(Qt::RichText);
        label->setOpenExternalLinks(false);
        label->setTextInteractionFlags(Qt::TextBrowserInteraction);
        label->setText("<center><a href=\"https://www.youtube.com/watch?v=9sJUDx7iEJw\" style=\"color: white;\"><h3>Complete your age check!!!!!</h3></a></center>");
        Layout->addWidget(label);
        connect(label, &QLabel::linkActivated, [label]() {
            QDesktopServices::openUrl(QUrl("https://www.youtube.com/watch?v=9sJUDx7iEJw"));
            label->deleteLater();
        });
    }

    QImage logoImg(":/images/icon1024.png");
    QPixmap logoPix = QPixmap::fromImage(logoImg).scaled(64 * static_cast<int>(devicePixelRatio()), 64 * static_cast<int>(devicePixelRatio()), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    logoPix.setDevicePixelRatio(devicePixelRatio());

    auto *logoLayout = new QHBoxLayout();
    logoLayout->setAlignment(Qt::AlignBottom);
    Layout->addLayout(logoLayout);

    auto *logoLabel = new QLabel();
    logoLabel->setPixmap(logoPix);
    logoLayout->addWidget(logoLabel);

    auto *titleLabel = new QLabel();
    titleLabel->setText("noobWarrior");
    titleLabel->setStyleSheet("QLabel { color: white; }");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);

    QFont font = titleLabel->font();
#if !defined(Q_OS_MACOS)
    font.setPointSize(20);
#else
    font.setPointSize(30); // fonts are smaller on Macs for whatever reason so we have to adjust for it
#endif
    titleLabel->setFont(font);
    logoLayout->addWidget(titleLabel);

    auto *frame = new QFrame(this);
    // QPalette framePalette = frame->palette();
    // framePalette.setColor(QPalette::Window, framePalette.color(QPalette::Window).darker(175));
    // frame->setPalette(framePalette);
    frame->setAutoFillBackground(true); // QFrames usually have invisible backgrounds, turn it on in this case.
    frame->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);

    Layout->addWidget(frame);

    auto *frameGrid = new QGridLayout(frame);
    frameGrid->setSpacing(8);
    frame->setLayout(frameGrid);
    for (int i = 0; i < NOOBWARRIOR_ARRAY_SIZE(sCategoryNames); i++) {
        auto *label = new QLabel(this);
        label->setText(sCategoryNames[i]);
        label->setMaximumHeight(24);
        frameGrid->addWidget(label);

        switch (i) {
        case 0: ADD_BUTTONS(sPlay) break;
        case 1: ADD_BUTTONS(sTools) break;
        case 2: ADD_BUTTONS(sApplication) break;
        }
    }

    /*
    auto *button = new QPushButton(this);
    button->setText("whatever");
    button->setIcon(QIcon(":/images/silk/vcard.png"));
    button->setStyleSheet("text-align: left;");
    QObject::connect(button, &QPushButton::clicked, []() {
        
    });
    frameGrid->addWidget(button);
    */

    auto *versionLabel = new QLabel(QString("noobWarrior v%1").arg(NOOBWARRIOR_VERSION));
    Layout->addWidget(versionLabel);

    AuthenticationStatusLabel = new QLabel("Not logged into Roblox");
    Layout->addWidget(AuthenticationStatusLabel);

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

Launcher::~Launcher() {}

void Launcher::paintEvent(QPaintEvent *event) {
    QDialog::paintEvent(event);

    RbxKeychain *keychain = gApp->GetCore()->GetRbxKeychain();
    AuthenticationStatusLabel->setText(keychain->IsLoggedIn() ? QString("Roblox Account: %1").arg(QString::fromStdString(keychain->GetActiveAccount()->Name)) : "Not logged into Roblox");

    // ServerEmulatorStatusLabel->setText(QString("Server Emulator: %1").arg(gApp->GetCore()->IsServerEmulatorRunning() ? "Running" : "Stopped"));
}