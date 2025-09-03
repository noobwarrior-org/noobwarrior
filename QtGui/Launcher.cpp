// === noobWarrior ===
// File: Launcher.cpp
// Started by: Hattozo
// Started on: 12/15/2024
// Description: This is the startup window that appears when the user launches the application.
#include <NoobWarrior/NoobWarrior.h>

#include "Launcher.h"
#include "Application.h"
#include "Dialog/AboutDialog.h"
#include "Dialog/AssetDownloaderDialog.h"

#include <QLabel>
#include <QMessageBox>

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

static void ShowStartGame(Launcher &launcher) { gApp->LaunchClient({ .NoobWarriorVersion = 1, .Type = ClientType::Server, .Hash = "07b64feec0bd47c1", .Version = "0.463.0.417004" }); }
static void ShowAboutDialog(Launcher &launcher) { HANDLE_QDIALOG(launcher.mAboutDialog, AboutDialog) }
static void ShowSettings(Launcher &launcher) { HANDLE_QDIALOG(launcher.mSettings, SettingsDialog) }
static void LaunchDatabaseEditor(Launcher& launcher) { HANDLE_QDIALOG(launcher.mDatabaseEditor, DatabaseEditor) }
static void ShowDownloadAssetDialog(Launcher &launcher) { HANDLE_QDIALOG(launcher.mAssetDownload, AssetDownloader) }
static void LaunchOfflineStudio(Launcher &launcher) { }

static const char* sCategoryNames[] = {
    "Roblox",
    "Tools",
    "Application"
};

static const void* sRoblox[][3] = {
    {"Start Game", (void*)&ShowStartGame, ":/images/silk/controller.png"},
    {"Join Server", nullptr, ":/images/silk/server_go.png"},
    {"Launch Studio", nullptr, ":/images/silk/application_side_tree.png"}
};

static const void* sTools[][3] = {
    {"Database Editor", (void*)&LaunchDatabaseEditor, ":/images/silk/database_gear.png"},
    {"Download Asset(s)", (void*)&ShowDownloadAssetDialog, ":/images/silk/page_save.png"},
    {"Model/Place Explorer", nullptr, ":/images/silk/bricks.png"},
    {"Scan Roblox Clients", nullptr, ":/images/silk/drive_magnify.png"},
    // {"Import Game As Database", nullptr, ":/images/silk/folder_go.png"}, // find this in the Database Utility instead
    {"Scan Roblox Cache", nullptr, ":/images/silk/folder_magnify.png"}
};

static const void* sApplication[][3] = {
    {"Shell", nullptr, ":/images/silk/application_xp_terminal.png"},
    {"Lua Shell", nullptr, ":/images/lua16.png"},
    {"Settings", (void*)&ShowSettings, ":/images/silk/cog.png"},
    {"About", (void*)&ShowAboutDialog, ":/images/silk/help.png"}
};

Launcher::Launcher(QWidget *parent) : QDialog(parent),
    mAboutDialog(nullptr),
    mSettings(nullptr),
    mDatabaseEditor(nullptr),
    mAssetDownload(nullptr)
{
    // ui->setupUi(this);
    setWindowTitle("noobWarrior");

    Layout = new QVBoxLayout(this);
    setLayout(Layout);

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
    QFont font = titleLabel->font();
    font.setPointSize(20);
    titleLabel->setFont(font);
    logoLayout->addWidget(titleLabel);

    auto *frame = new QFrame(this);
    // QPalette framePalette = frame->palette();
    // framePalette.setColor(QPalette::Window, framePalette.color(QPalette::Window).darker(175));
    // frame->setPalette(framePalette);
    frame->setAutoFillBackground(true); // QFrames usually have invisible backgrounds, turn it on in this case.
    frame->setFrameStyle(QFrame::Panel | QFrame::Sunken);

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
        case 0: ADD_BUTTONS(sRoblox) break;
        case 1: ADD_BUTTONS(sTools) break;
        case 2: ADD_BUTTONS(sApplication) break;
        }
    }

    AuthenticationStatusLabel = new QLabel("Not authenticated with Roblox");
    Layout->addWidget(AuthenticationStatusLabel);

    ServerEmulatorStatusLabel = new QLabel("Server Emulator: Stopped");
    Layout->addWidget(ServerEmulatorStatusLabel);

    auto *robloxServersLabel = new QLabel("0 Running Roblox Servers");
    Layout->addWidget(robloxServersLabel);

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

Launcher::~Launcher() {}

void Launcher::paintEvent(QPaintEvent *event) {
    QDialog::paintEvent(event);

    Auth *auth = gApp->GetCore()->GetAuth();
    AuthenticationStatusLabel->setText(auth->IsLoggedIn() ? QString("Logged in as %1").arg(auth->GetActiveAccount()->Name) : "Not authenticated with Roblox");

    ServerEmulatorStatusLabel->setText(QString("Server Emulator: %1").arg(gApp->GetCore()->IsServerEmulatorRunning() ? "Running" : "Stopped"));
}