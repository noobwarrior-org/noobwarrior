// === noobWarrior ===
// File: Launcher.cpp
// Started by: Hattozo
// Started on: 12/15/2024
// Description: This is the startup window that appears when the user launches the application.
#include <NoobWarrior/NoobWarrior.h>

#include "Launcher.h"
#include "Ui/ui_Launcher.h"

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

using namespace NoobWarrior;

static void LaunchArchiveUtility(Launcher& launcher) {
    if (!launcher.mArchiveEditor) {
        launcher.mArchiveEditor = new ArchiveEditor(nullptr);
        launcher.mArchiveEditor->setAttribute(Qt::WA_DeleteOnClose); // Ensure deletion on close

        QObject::connect(launcher.mArchiveEditor, &QWidget::destroyed, [&]() {
            launcher.mArchiveEditor = nullptr;
        });

        launcher.mArchiveEditor->show();
    } else {
        launcher.mArchiveEditor->raise(); // Bring to front if already open
        launcher.mArchiveEditor->activateWindow(); // Make active
    }
}

static void LaunchOfflineStudio(Launcher& launcher) {

}

static const char* sCategoryNames[] = {
    "Tools",
    "Roblox",
    "Application"
};

static const void* sTools[][3] = {
    {"Archive Utility", (void*)&LaunchArchiveUtility, ":/images/silk/database_gear.png"},
    {"Model/Place Explorer", nullptr, ":/images/silk/bricks.png"},
    {"Download Asset(s)", nullptr, ":/images/silk/brick_add.png"},
    {"Import Game As Archive", nullptr, ":/images/silk/folder_go.png"},
};

static const void* sRoblox[][3] = {
    {"Host Server", nullptr, ":/images/silk/server.png"},
    {"Join Server", nullptr, ":/images/silk/controller.png"},
    {"Play Solo", nullptr, ":/images/silk/status_offline.png"},
    {"Launch Offline Studio", nullptr, ":/images/silk/application_side_tree.png"},
    {"Manage Roblox Installations", nullptr, ":/images/silk/disk_multiple.png"},
    {"Manage Servers", nullptr, ":/images/silk/drive_network.png"}
};

static const void* sApplication[][3] = {
    {"Shell", nullptr, ":/images/silk/application_xp_terminal.png"},
    {"Settings", nullptr, ":/images/silk/cog.png"},
    {"About", nullptr, ":/images/silk/help.png"}
};

Launcher::Launcher(QWidget *parent) : QDialog(parent), ui(new Ui::Launcher), mArchiveEditor(nullptr) {
    // ui->setupUi(this);
    setWindowTitle("noobWarrior");
    auto *grid = new QGridLayout(this);
    setLayout(grid);
    auto *logo = new QLabel(this);
    logo->setTextFormat(Qt::TextFormat::RichText);
    logo->setText("<h1><img src=\":/images/icon64.png\">noobWarrior</h1>");
    logo->setMaximumHeight(64);
    grid->addWidget(logo);
    auto *frame = new QFrame(this);
    frame->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    // frame->setStyleSheet("background-color: rgb(80, 80, 80);");
    grid->addWidget(frame);
    auto *frameGrid = new QGridLayout(frame);
    frame->setLayout(frameGrid);
    for (int i = 0; i < NOOBWARRIOR_ARRAY_SIZE(sCategoryNames); i++) {
        auto *label = new QLabel(this);
        label->setText(sCategoryNames[i]);
        label->setMaximumHeight(20);
        frameGrid->addWidget(label);

        switch (i) {
        case 0: ADD_BUTTONS(sTools) break;
        case 1: ADD_BUTTONS(sRoblox) break;
        case 2: ADD_BUTTONS(sApplication) break;
        }
    }
    resize(grid->sizeHint());
}

Launcher::~Launcher() {
    delete ui;
    ui = nullptr;
}