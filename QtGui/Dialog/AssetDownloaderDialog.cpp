// === noobWarrior ===
// File: AssetDownloaderDialog.cpp
// Started by: Hattozo
// Started on: 3/13/2025
// Description: A Qt GUI dialog that invokes the noobWarrior API to download a Roblox asset
#include "AssetDownloaderDialog.h"

#include <NoobWarrior/AssetRequest.h>
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

    mQueueModel = new QStandardItemModel(this);
    mQueueModel->setColumnCount(5);
    mQueueModel->setHorizontalHeaderLabels({"Id", "Icon", "Name", "Description", "Author"});
    auto *queueView = new QTreeView();
    queueView->setModel(mQueueModel);

    mDownloadButton = new QPushButton();
    mDownloadButton->setText("Download");

    downloadQueueLayout->addLayout(idAddLayout);
    downloadQueueLayout->addWidget(queueView);
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

    resize(mMainLayout->sizeHint() * 1.25 + QSize(256, 64));
    // mQueueList->resizeColumnsToContents();

    InitControls();
}

AssetDownloaderDialog::~AssetDownloaderDialog() { delete mOutStream; }

void AssetDownloaderDialog::InitControls() {
    connect(mQueueAdd, &QPushButton::clicked, [&]() {
        if (mIdInput->text().isEmpty())
            return;
        bool okay;
        int64_t idConvert = mIdInput->text().toLongLong(&okay, 10);
        if (okay)
            AddEntry(idConvert);
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

void AssetDownloaderDialog::AddEntry(int64_t id) {
    Roblox::AssetDetails details {};
    int ret = GetAssetDetails(id, &details);
    switch (ret) {
    case -1:
        details.Name = "Failed to get metadata";
        details.Description = "You are being rate-limited. Try logging in.";
        break;
    }
    QList<QStandardItem*> entry;
    auto *idColumn = new QStandardItem(QString::number(id));
    auto *iconColumn = new QStandardItem("Icon");
    // iconColumn->setData(QVariant(QPixmap::fromImage(image)), Qt::DecorationRole)
    auto *nameColumn = new QStandardItem(QString::fromStdString(details.Name));
    auto *descColumn = new QStandardItem(QString::fromStdString(details.Description));
    entry.append(idColumn);
    entry.append(iconColumn);
    entry.append(nameColumn);
    entry.append(descColumn);
    mQueueEntries.push_back(entry);
    mQueueModel->appendRow(entry);
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