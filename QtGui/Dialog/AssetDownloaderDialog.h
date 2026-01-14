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
class AssetDownloader : public QDialog {
    Q_OBJECT
public:
    AssetDownloader(QWidget *parent = nullptr);
    ~AssetDownloader();
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