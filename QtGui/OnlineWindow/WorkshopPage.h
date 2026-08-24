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
// File: WorkshopPage.h
// Started by: Hattozo
// Started on: 8/23/2026
// Description: Browse and download one master server's workshop
#pragma once
#include <QWidget>

#include <cstdint>

class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;
class QTextEdit;

namespace NoobWarrior {
// The native counterpart to the master server's /workshop pages: a grid of submissions on the left,
// the selected one's details and comments on the right. Uploading is deliberately left to the web
// page, which drives a chunked upload session this window has no reason to reimplement.
class WorkshopPage : public QWidget {
    Q_OBJECT
public:
    explicit WorkshopPage(QWidget *parent = nullptr);

    void SetMaster(const QString &masterUrl);
    // Re-reads the sign-in state (which decides whether commenting is offered) and refetches.
    void Reload();

signals:
    void StatusChanged(const QString &status);

private:
    void InitWidgets();
    QWidget *BuildTopBar();
    QWidget *BuildDetails();
    QLabel *AddRow(QFormLayout *form, QWidget *parent, const QString &label);

    void FetchList();
    void FetchThumbnail(int64_t submissionId);
    void ShowDetails(int64_t submissionId);
    void ClearDetails();

    void Download();
    void DeleteSelected();
    void PostComment();

    QListWidgetItem *FindItem(int64_t submissionId) const;

    QString mMasterUrl;
    int64_t mCurrentId { 0 };
    bool mCurrentIsMine { false };
    QString mCurrentName;
    bool mLoading { false };

    QComboBox *mFilter { nullptr };
    QLineEdit *mSearch { nullptr };
    QPushButton *mUploadButton { nullptr };

    QListWidget *mList { nullptr };

    QStackedWidget *mDetailStack { nullptr };
    QWidget *mEmptyDetails { nullptr };
    QWidget *mDetails { nullptr };
    QLabel *mThumbnail { nullptr };
    QLabel *mTitle { nullptr };
    QLabel *mTypeValue { nullptr };
    QLabel *mUploaderValue { nullptr };
    QLabel *mSizeValue { nullptr };
    QLabel *mPostedValue { nullptr };
    QLabel *mDescription { nullptr };
    QPushButton *mDownloadButton { nullptr };
    QPushButton *mDeleteButton { nullptr };
    QListWidget *mComments { nullptr };
    QTextEdit *mCommentBody { nullptr };
    QPushButton *mCommentButton { nullptr };
    QLabel *mCommentHint { nullptr };
};
}
