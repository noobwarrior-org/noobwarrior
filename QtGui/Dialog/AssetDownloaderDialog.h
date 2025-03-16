// === noobWarrior ===
// File: AssetDownloaderDialog.h
// Started by: Hattozo
// Started on: 3/13/2025
// Description:
#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QCheckBox>

#include <ostream>
#include <vector>

namespace NoobWarrior {
class AssetDownloaderDialog : public QDialog {
    Q_OBJECT
public:
    AssetDownloaderDialog(QWidget *parent = nullptr);
    ~AssetDownloaderDialog();
private:
    std::vector<QList<QStandardItem*>> mQueueEntries;

    class VisualStreamBuffer : public std::streambuf {
    public:
        VisualStreamBuffer(QPlainTextEdit*);
    private:
        int_type overflow(int_type c) override;
        int sync() override;

        std::string mBuffer;
        QPlainTextEdit *mWidget;
    };
    class VisualOutStream : public std::ostream {
    public:
        VisualOutStream(QPlainTextEdit*);
        ~VisualOutStream();
    private:
        VisualStreamBuffer *mBuffer;
    };
    VisualOutStream *mOutStream;

    QHBoxLayout *mMainLayout;
    QVBoxLayout *mLeftSideLayout;
    QVBoxLayout *mRightSideLayout;

    QLineEdit *mIdInput;
    QPushButton *mQueueAdd;
    QStandardItemModel *mQueueModel;
    QPushButton *mDownloadButton;

    QPlainTextEdit *mOutputWidget;
    QProgressBar *mDownloadProgress;

    QHBoxLayout *mOpt_DirLayout;
    QLabel *mOpt_DirLabel;
    QLineEdit *mOpt_DirInput;
    QPushButton *mOpt_DirBrowse;
    
    QCheckBox *mBox_DownloadAssetsInModel;
    QCheckBox *mBox_PreserveAuthors;
    QCheckBox *mBox_PreserveDateCreated;
    QCheckBox *mBox_PreserveDateModified;

    void InitControls();
    void AddEntry(int64_t id);
};
}