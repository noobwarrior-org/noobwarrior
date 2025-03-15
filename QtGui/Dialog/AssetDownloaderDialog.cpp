// === noobWarrior ===
// File: AssetDownloaderDialog.cpp
// Started by: Hattozo
// Started on: 3/13/2025
// Description: A Qt GUI dialog that invokes the noobWarrior API to download a Roblox asset
#include "AssetDownloaderDialog.h"

#include <NoobWarrior/AssetDownloader.h>
#include <NoobWarrior/NoobWarrior.h>

#include <QGroupBox>
#include <QFileDialog>

#define INIT_OPT(var, str) var = new QCheckBox(this); var->setText(str); optionsLayout->addWidget(var);

using namespace NoobWarrior;

AssetDownloaderDialog::AssetDownloaderDialog(QWidget *parent) {
    setWindowTitle("Download Asset(s)");
    mMainLayout = new QHBoxLayout(this);
    mLeftSideLayout = new QVBoxLayout();
    mRightSideLayout = new QVBoxLayout();

    // Add Asset ID layout
    auto *idAddLayout = new QHBoxLayout();
    mIdInput = new QLineEdit();
    mIdInput->setPlaceholderText("Asset ID");
    mQueueAdd = new QPushButton();
    mQueueAdd->setText("Add");
    idAddLayout->addWidget(mIdInput);
    idAddLayout->addWidget(mQueueAdd);

    // Download Queue box
    auto *downloadQueueBox = new QGroupBox("Download Queue");
    auto *downloadQueueLayout = new QVBoxLayout(downloadQueueBox);
    mQueueList = new QTableWidget();
    mQueueList->setColumnCount(5);
    mQueueList->setHorizontalHeaderLabels({"Id", "Icon", "Name", "Description", "Author"});
    mQueueList->setSortingEnabled(true);
    mQueueList->resizeColumnsToContents();
    mQueueList->setRowCount(0);
    mQueueList->verticalHeader()->setVisible(false);
    mDownloadButton = new QPushButton();
    mDownloadButton->setText("Download");

    downloadQueueLayout->addLayout(idAddLayout);
    downloadQueueLayout->addWidget(mQueueList);
    downloadQueueLayout->addWidget(mDownloadButton);

    // Options box
    auto *optionsBox = new QGroupBox("Options");
    auto *optionsLayout = new QGridLayout(optionsBox);

    mOpt_DirLayout = new QHBoxLayout();
    mOpt_DirLabel = new QLabel("Directory");
    mOpt_DirInput = new QLineEdit("./downloads");
    mOpt_DirBrowse = new QPushButton("Browse");
    mOpt_DirLayout->addWidget(mOpt_DirLabel);
    mOpt_DirLayout->addWidget(mOpt_DirInput);
    mOpt_DirLayout->addWidget(mOpt_DirBrowse);

    optionsLayout->addLayout(mOpt_DirLayout, 0, 0);

    INIT_OPT(mBox_DownloadAssetsInModel, "Download Asset Ids Inside Of Model/Place")
    INIT_OPT(mBox_PreserveAuthors, "Preserve Authors")
    INIT_OPT(mBox_PreserveDateCreated, "Preserve Date Created")
    INIT_OPT(mBox_PreserveDateModified, "Preserve Date Modified")

    // Output box
    auto *outputBox = new QGroupBox("Output");
    auto *outputLayout = new QVBoxLayout(outputBox);
    mOutputWidget = new QPlainTextEdit();
    mOutputWidget->setReadOnly(true);
    mOutputWidget->setFont(QFont("Fira Mono", 12));
    mDownloadProgress = new QProgressBar();
    outputLayout->addWidget(mOutputWidget);
    outputLayout->addWidget(mDownloadProgress);

    mOutStream = new VisualOutStream(mOutputWidget);

    // init
    mMainLayout->addLayout(mLeftSideLayout);
    mMainLayout->addLayout(mRightSideLayout);

    mLeftSideLayout->addWidget(downloadQueueBox);
    mRightSideLayout->addWidget(optionsBox);
    mRightSideLayout->addWidget(outputBox);

    resize(mMainLayout->sizeHint() * 1.25 + QSize(128, 0));
    mQueueList->resizeColumnsToContents();

    InitControls();
}

AssetDownloaderDialog::~AssetDownloaderDialog() { delete mOutStream; }

void AssetDownloaderDialog::InitControls() {
    connect(mQueueAdd, &QPushButton::clicked, [&]() {
        auto *item = new QTableWidgetItem(mIdInput->text());
        mQueueList->insertRow(mQueueList->rowCount());
        mQueueList->setItem(mQueueList->rowCount() - 1, 0, item);
    });

    connect(mDownloadButton, &QPushButton::clicked, [&]() {
        DownloadAssetArgs args {};
        args.Flags |= mBox_DownloadAssetsInModel->checkState() ? DA_FIND_ASSET_IDS_IN_MODEL : 0;
        args.OutStream = mOutStream;
        args.OutDir = mOpt_DirInput->text().toStdString();
        DownloadAssets(args);
    });

    connect(mOpt_DirBrowse, &QPushButton::clicked, [&]() {
        QString pathStr = QFileDialog::getExistingDirectory(this, "Select Directory", "", QFileDialog::ShowDirsOnly);
        if (!pathStr.isEmpty())
            mOpt_DirInput->setText(pathStr);
    });
}

AssetDownloaderDialog::VisualStreamBuffer::VisualStreamBuffer(QPlainTextEdit *widget) :
    mWidget(widget)
{};

auto AssetDownloaderDialog::VisualStreamBuffer::overflow(int_type c) -> int_type {
    if (c != EOF) {
        mBuffer += static_cast<char>(c);
        sync();
    }
    return c;
}

int AssetDownloaderDialog::VisualStreamBuffer::sync() {
    mWidget->setPlainText(mWidget->toPlainText() + QString::fromStdString(mBuffer));
    mBuffer.clear();
    return 0;
}

AssetDownloaderDialog::VisualOutStream::VisualOutStream(QPlainTextEdit *widget) :
    std::ostream(new VisualStreamBuffer(widget)),
    mBuffer(dynamic_cast<VisualStreamBuffer*>(rdbuf()))
{}

AssetDownloaderDialog::VisualOutStream::~VisualOutStream() { NOOBWARRIOR_FREE_PTR(mBuffer) }