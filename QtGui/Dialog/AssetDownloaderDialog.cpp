// === noobWarrior ===
// File: AssetDownloaderDialog.cpp
// Started by: Hattozo
// Started on: 3/13/2025
// Description: A Qt GUI dialog that invokes the noobWarrior API to download a Roblox asset
#include "AssetDownloaderDialog.h"

#include <NoobWarrior/AssetDownloader.h>
#include <NoobWarrior/NoobWarrior.h>

#include <QGroupBox>

#define INIT_OPT(var, str) var = new QCheckBox(this); var->setText(str); optionsLayout->addWidget(var);

using namespace NoobWarrior;

AssetDownloaderDialog::AssetDownloaderDialog(QWidget *parent) {
    setWindowTitle("Download Asset(s)");
    mMainLayout = new QHBoxLayout(this);
    mLeftSideLayout = new QVBoxLayout(this);
    mRightSideLayout = new QVBoxLayout(this);

    // Add Asset ID layout
    auto *idAddLayout = new QHBoxLayout(this);
    mIdInput = new QLineEdit(this);
    mIdInput->setPlaceholderText("Asset ID");
    mQueueAdd = new QPushButton(this);
    mQueueAdd->setText("Add");
    idAddLayout->addWidget(mIdInput);
    idAddLayout->addWidget(mQueueAdd);

    // Download Queue box
    auto *downloadQueueLayout = new QVBoxLayout(this);
    mQueueList = new QTableWidget(this);
    mQueueList->setColumnCount(5);
    mQueueList->setHorizontalHeaderLabels({"Icon", "Id", "Name", "Description", "Author"});
    mQueueList->setSortingEnabled(true);
    mQueueList->resizeColumnsToContents();
    mQueueList->setRowCount(0);
    mQueueList->verticalHeader()->setVisible(false);
    mDownloadButton = new QPushButton(this);
    mDownloadButton->setText("Download");

    downloadQueueLayout->addLayout(idAddLayout);
    downloadQueueLayout->addWidget(mQueueList);
    downloadQueueLayout->addWidget(mDownloadButton);

    auto *downloadQueueBox = new QGroupBox("Download Queue", this);
    downloadQueueBox->setLayout(downloadQueueLayout);

    // Output box
    auto *outputLayout = new QVBoxLayout(this);
    mOutputWidget = new QPlainTextEdit(this);
    mOutputWidget->setReadOnly(true);
    mOutputWidget->setFont(QFont("Fira Mono"));
    mDownloadProgress = new QProgressBar(this);
    outputLayout->addWidget(mOutputWidget);
    outputLayout->addWidget(mDownloadProgress);
    auto *outputBox = new QGroupBox("Output", this);
    outputBox->setLayout(outputLayout);

    mOutStream = new VisualOutStream(mOutputWidget);

    // Options box
    auto *optionsLayout = new QGridLayout(this);
    INIT_OPT(mBox_DownloadAssetsInModel, "Download Asset Ids Inside Of Model/Place")
    INIT_OPT(mBox_PreserveAuthors, "Preserve Authors")
    INIT_OPT(mBox_PreserveDateCreated, "Preserve Date Created")
    INIT_OPT(mBox_PreserveDateModified, "Preserve Date Modified")
    auto *optionsBox = new QGroupBox("Options", this);
    optionsBox->setLayout(optionsLayout);

    setLayout(mMainLayout);
    mMainLayout->addLayout(mLeftSideLayout);
    mMainLayout->addLayout(mRightSideLayout);

    mLeftSideLayout->addWidget(downloadQueueBox);
    mRightSideLayout->addWidget(optionsBox);
    mRightSideLayout->addWidget(outputBox);

    InitControls();
}

AssetDownloaderDialog::~AssetDownloaderDialog() { delete mOutStream; }

void AssetDownloaderDialog::InitControls() {
    connect(mDownloadButton, &QPushButton::clicked, [&]() {
        DownloadAssetArgs args;
        args.Flags |= mBox_DownloadAssetsInModel->checkState() ? DA_FIND_ASSET_IDS_IN_MODEL : 0;
        args.OutStream = mOutStream;
        DownloadAssets(args);
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