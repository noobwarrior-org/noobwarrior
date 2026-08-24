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
// File: WorkshopPage.cpp
// Started by: Hattozo
// Started on: 8/23/2026
// Description: Browse and download one master server's workshop
#include <cpr/cpr.h>

#include "WorkshopPage.h"
#include "MasterHttp.h"
#include "MasterServerStore.h"
#include "../Application.h"
#include "LoadingDialog.h"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <fstream>
#include <memory>
#include <thread>

#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSplitter>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

using namespace NoobWarrior;

static constexpr int kRoleSubmissionId = Qt::UserRole + 1;
static constexpr int kThumbnailSize = 128;
// Room under the icon for a wrapped, two-line name.
static const QSize kCellSize(kThumbnailSize + 40, kThumbnailSize + 56);

// Every card gets the same icon footprint: the artwork centred in a kThumbnailSize box, scaled
// down only if it overflows. Icon-mode cells otherwise size themselves to whatever each icon
// happens to be, so a 16x16 placeholder collapses the cell and clips the name off the card.
static QIcon CardIcon(const QPixmap &source) {
    QPixmap canvas(kThumbnailSize, kThumbnailSize);
    canvas.fill(Qt::transparent);
    if (!source.isNull()) {
        QPixmap scaled = (source.width() > kThumbnailSize || source.height() > kThumbnailSize)
            ? source.scaled(kThumbnailSize, kThumbnailSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)
            : source;
        QPainter painter(&canvas);
        painter.drawPixmap((kThumbnailSize - scaled.width()) / 2,
                           (kThumbnailSize - scaled.height()) / 2, scaled);
    }
    return QIcon(canvas);
}

// Shown until (or instead of) a thumbnail. These are the same 96px artworks the SDK uses.
static QIcon PlaceholderIcon(const QString &type) {
    return CardIcon(QPixmap(type == "plugin" ? ":/images/plugin_96x96.png"
                                             : ":/images/empty_database_96x96.png"));
}

WorkshopPage::WorkshopPage(QWidget *parent) : QWidget(parent) {
    InitWidgets();
}

QLabel *WorkshopPage::AddRow(QFormLayout *form, QWidget *parent, const QString &label) {
    auto *value = new QLabel(parent);
    value->setWordWrap(true);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    form->addRow(label, value);
    return value;
}

void WorkshopPage::InitWidgets() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(BuildTopBar());

    auto *splitter = new QSplitter(Qt::Horizontal, this);

    mList = new QListWidget(splitter);
    mList->setViewMode(QListView::IconMode);
    mList->setIconSize(QSize(kThumbnailSize, kThumbnailSize));
    mList->setGridSize(kCellSize);
    mList->setResizeMode(QListView::Adjust);
    mList->setMovement(QListView::Static);
    mList->setWordWrap(true);
    mList->setSpacing(6);
    connect(mList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current, QListWidgetItem *) {
                if (current == nullptr) {
                    ClearDetails();
                    return;
                }
                ShowDetails(current->data(kRoleSubmissionId).toLongLong());
            });
    splitter->addWidget(mList);

    mDetailStack = new QStackedWidget(splitter);

    mEmptyDetails = new QWidget(mDetailStack);
    auto *emptyLayout = new QVBoxLayout(mEmptyDetails);
    auto *emptyLabel = new QLabel("Select a submission to see its details.", mEmptyDetails);
    emptyLabel->setWordWrap(true);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyLabel);
    mDetailStack->addWidget(mEmptyDetails);

    mDetailStack->addWidget(BuildDetails());
    splitter->addWidget(mDetailStack);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);
}

QWidget *WorkshopPage::BuildTopBar() {
    auto *bar = new QWidget(this);
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(4, 4, 4, 4);

    mFilter = new QComboBox(bar);
    mFilter->addItem("All", "all");
    mFilter->addItem("Databases", "database");
    mFilter->addItem("Plugins", "plugin");
    mFilter->addItem("My Submissions", "mine");
    connect(mFilter, &QComboBox::currentIndexChanged, this, [this](int) { FetchList(); });
    layout->addWidget(mFilter);

    mSearch = new QLineEdit(bar);
    mSearch->setPlaceholderText("Search...");
    mSearch->setClearButtonEnabled(true);
    connect(mSearch, &QLineEdit::returnPressed, this, &WorkshopPage::FetchList);
    layout->addWidget(mSearch, 1);

    auto *searchButton = new QPushButton(QIcon(":/images/silk/magnifier.png"), "Search", bar);
    connect(searchButton, &QPushButton::clicked, this, &WorkshopPage::FetchList);
    layout->addWidget(searchButton);

    auto *refreshButton = new QPushButton(QIcon(":/images/silk/arrow_refresh.png"), "Refresh", bar);
    connect(refreshButton, &QPushButton::clicked, this, &WorkshopPage::FetchList);
    layout->addWidget(refreshButton);

    // Uploading runs as a chunked start/stream/end session driven by the master's own JavaScript;
    // hand that page to the browser rather than duplicating the protocol here.
    mUploadButton = new QPushButton(QIcon(":/images/silk/package_add.png"), "Upload...", bar);
    connect(mUploadButton, &QPushButton::clicked, this, [this]() {
        if (mMasterUrl.isEmpty())
            return;
        QDesktopServices::openUrl(QUrl(MasterHttp::ResolveUrl(mMasterUrl, "/workshop?type=upload")));
    });
    layout->addWidget(mUploadButton);

    return bar;
}

QWidget *WorkshopPage::BuildDetails() {
    auto *scroll = new QScrollArea(mDetailStack);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    mDetails = new QWidget(scroll);
    auto *layout = new QVBoxLayout(mDetails);

    mThumbnail = new QLabel(mDetails);
    mThumbnail->setAlignment(Qt::AlignCenter);
    mThumbnail->setMinimumHeight(kThumbnailSize);
    layout->addWidget(mThumbnail);

    mTitle = new QLabel(mDetails);
    QFont titleFont = mTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 3);
    mTitle->setFont(titleFont);
    mTitle->setWordWrap(true);
    layout->addWidget(mTitle);

    auto *form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    mTypeValue = AddRow(form, mDetails, "Type");
    mUploaderValue = AddRow(form, mDetails, "Uploaded by");
    mSizeValue = AddRow(form, mDetails, "File size");
    mPostedValue = AddRow(form, mDetails, "Posted");
    layout->addLayout(form);

    auto *aboutLabel = new QLabel("About", mDetails);
    QFont sectionFont = aboutLabel->font();
    sectionFont.setBold(true);
    aboutLabel->setFont(sectionFont);
    layout->addWidget(aboutLabel);

    mDescription = new QLabel(mDetails);
    mDescription->setWordWrap(true);
    mDescription->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(mDescription);

    auto *buttons = new QHBoxLayout();
    mDownloadButton = new QPushButton(QIcon(":/images/silk/package_go.png"), "Download", mDetails);
    connect(mDownloadButton, &QPushButton::clicked, this, &WorkshopPage::Download);
    buttons->addWidget(mDownloadButton);

    mDeleteButton = new QPushButton(QIcon(":/images/silk/package_delete.png"), "Delete", mDetails);
    mDeleteButton->setVisible(false); // only shown once the item is known to be ours
    connect(mDeleteButton, &QPushButton::clicked, this, &WorkshopPage::DeleteSelected);
    buttons->addWidget(mDeleteButton);

    auto *openButton = new QPushButton("Open in Browser", mDetails);
    connect(openButton, &QPushButton::clicked, this, [this]() {
        if (mMasterUrl.isEmpty() || mCurrentId == 0)
            return;
        QDesktopServices::openUrl(QUrl(MasterHttp::ResolveUrl(
            mMasterUrl, QString("/workshop?type=details&id=%1").arg(mCurrentId))));
    });
    buttons->addWidget(openButton);
    buttons->addStretch();
    layout->addLayout(buttons);

    auto *commentsLabel = new QLabel("Comments", mDetails);
    commentsLabel->setFont(sectionFont);
    layout->addWidget(commentsLabel);

    mComments = new QListWidget(mDetails);
    mComments->setAlternatingRowColors(true);
    mComments->setWordWrap(true);
    mComments->setMinimumHeight(120);
    layout->addWidget(mComments, 1);

    mCommentBody = new QTextEdit(mDetails);
    mCommentBody->setPlaceholderText("Leave a comment...");
    mCommentBody->setMaximumHeight(70);
    layout->addWidget(mCommentBody);

    mCommentButton = new QPushButton("Post Comment", mDetails);
    connect(mCommentButton, &QPushButton::clicked, this, &WorkshopPage::PostComment);
    layout->addWidget(mCommentButton, 0, Qt::AlignLeft);

    mCommentHint = new QLabel("Sign in on the Profile page to comment.", mDetails);
    mCommentHint->setWordWrap(true);
    layout->addWidget(mCommentHint);

    scroll->setWidget(mDetails);
    return scroll;
}

void WorkshopPage::SetMaster(const QString &masterUrl) {
    // Reload unconditionally so a sign-in elsewhere is reflected; only drop the search when the
    // master itself changed.
    if (mMasterUrl != masterUrl) {
        mMasterUrl = masterUrl;
        mSearch->clear();
    }
    Reload();
}

void WorkshopPage::Reload() {
    // "My Submissions" needs a session; fall back to All when the user is signed out so the page
    // isn't stuck showing a 401.
    bool signedIn = !mMasterUrl.isEmpty() && MasterHttp::IsSignedIn(mMasterUrl);
    mUploadButton->setEnabled(signedIn);
    if (!signedIn && mFilter->currentData().toString() == "mine")
        mFilter->setCurrentIndex(0);

    FetchList();
}

QListWidgetItem *WorkshopPage::FindItem(int64_t submissionId) const {
    for (int i = 0; i < mList->count(); i++) {
        QListWidgetItem *item = mList->item(i);
        if (item->data(kRoleSubmissionId).toLongLong() == submissionId)
            return item;
    }
    return nullptr;
}

void WorkshopPage::FetchList() {
    mList->clear();
    ClearDetails();

    if (mMasterUrl.isEmpty()) {
        emit StatusChanged(QString());
        return;
    }
    if (mLoading)
        return;

    QString path = QString("/v1/workshop/list?type=%1").arg(mFilter->currentData().toString());
    QString search = mSearch->text().trimmed();
    if (!search.isEmpty())
        path += "&q=" + QString::fromUtf8(QUrl::toPercentEncoding(search));

    mLoading = true;
    emit StatusChanged("Loading workshop...");

    MasterHttp::Get(this, mMasterUrl, path, [this](const MasterResponse &response) {
        mLoading = false;

        if (!response.Ok) {
            emit StatusChanged("Could not load the workshop: " + QString::fromStdString(response.Error));
            return;
        }

        nlohmann::json list = nlohmann::json::parse(response.Body, nullptr, false);
        if (list.is_discarded() || !list.is_array()) {
            emit StatusChanged("The master server sent a workshop listing we could not read.");
            return;
        }

        for (const auto &entry : list) {
            auto id = entry.value("Id", static_cast<int64_t>(0));
            QString name = QString::fromStdString(entry.value("Name", std::string{}));
            QString uploader = QString::fromStdString(entry.value("Uploader", std::string{}));
            QString sizeText = QString::fromStdString(entry.value("SizeText", std::string{}));
            QString type = QString::fromStdString(entry.value("Type", std::string{}));

            auto *item = new QListWidgetItem(name, mList);
            item->setData(kRoleSubmissionId, static_cast<qlonglong>(id));
            item->setToolTip(QString("%1\nBy %2 - %3").arg(name, uploader, sizeText));
            item->setIcon(PlaceholderIcon(type));
            item->setSizeHint(kCellSize);
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);

            if (entry.value("HasThumbnail", false))
                FetchThumbnail(id);
        }

        emit StatusChanged(mList->count() == 1
            ? QString("1 submission.")
            : QString("%1 submissions.").arg(mList->count()));
    });
}

void WorkshopPage::FetchThumbnail(int64_t submissionId) {
    QString path = QString("/v1/workshop/thumbnail?id=%1").arg(submissionId);
    MasterHttp::Get(this, mMasterUrl, path, [this, submissionId](const MasterResponse &response) {
        if (!response.Ok)
            return;

        QPixmap pixmap;
        if (!pixmap.loadFromData(reinterpret_cast<const uchar *>(response.Body.data()),
                                 static_cast<uint>(response.Body.size())))
            return;

        // The list may have been refiltered while this was in flight, so match on the id rather
        // than holding on to the item pointer.
        if (QListWidgetItem *item = FindItem(submissionId); item != nullptr)
            item->setIcon(CardIcon(pixmap));

        if (mCurrentId == submissionId)
            mThumbnail->setPixmap(pixmap.scaled(QSize(240, 240), Qt::KeepAspectRatio,
                                                Qt::SmoothTransformation));
    });
}

void WorkshopPage::ClearDetails() {
    mCurrentId = 0;
    mCurrentIsMine = false;
    mCurrentName.clear();
    mDetailStack->setCurrentWidget(mEmptyDetails);
}

void WorkshopPage::ShowDetails(int64_t submissionId) {
    if (submissionId == 0 || mMasterUrl.isEmpty())
        return;

    mCurrentId = submissionId;
    mCurrentIsMine = false;
    mThumbnail->clear();
    mComments->clear();
    mDeleteButton->setVisible(false);
    mDetailStack->setCurrentIndex(1);

    QString path = QString("/v1/workshop/item?id=%1").arg(submissionId);
    MasterHttp::Get(this, mMasterUrl, path, [this, submissionId](const MasterResponse &response) {
        // The user may have clicked another submission while this was loading.
        if (mCurrentId != submissionId)
            return;

        if (!response.Ok) {
            mTitle->setText("Could not load this submission");
            mDescription->setText(QString::fromStdString(response.Error));
            return;
        }

        nlohmann::json item = nlohmann::json::parse(response.Body, nullptr, false);
        if (item.is_discarded() || !item.is_object()) {
            mTitle->setText("Could not read this submission");
            return;
        }

        mCurrentName = QString::fromStdString(item.value("Name", std::string{}));
        mCurrentIsMine = item.value("IsMine", false);

        mTitle->setText(mCurrentName);
        mTypeValue->setText(item.value("Type", std::string{}) == "plugin" ? "Plugin" : "Database");
        mUploaderValue->setText(QString::fromStdString(item.value("Uploader", std::string{})));
        mSizeValue->setText(QString::fromStdString(item.value("SizeText", std::string{})));

        auto created = item.value("CreatedTimestamp", static_cast<int64_t>(0));
        mPostedValue->setText(created > 0
            ? QDateTime::fromSecsSinceEpoch(created).toString("MMM d, yyyy h:mm ap")
            : QString("Unknown"));

        QString description = QString::fromStdString(item.value("Description", std::string{})).trimmed();
        mDescription->setText(description.isEmpty() ? "No description was provided." : description);

        mDeleteButton->setVisible(mCurrentIsMine);

        bool canComment = item.value("CanComment", false);
        mCommentBody->setVisible(canComment);
        mCommentButton->setVisible(canComment);
        mCommentHint->setVisible(!canComment);

        mComments->clear();
        nlohmann::json comments = item.value("Comments", nlohmann::json{});
        if (comments.is_array()) {
            for (const auto &comment : comments) {
                QString author = QString::fromStdString(comment.value("Author", std::string{}));
                QString body = QString::fromStdString(comment.value("Body", std::string{}));
                auto when = comment.value("CreatedTimestamp", static_cast<int64_t>(0));
                QString date = when > 0
                    ? QDateTime::fromSecsSinceEpoch(when).toString("MMM d, yyyy h:mm ap")
                    : QString();
                mComments->addItem(QString("%1 - %2\n%3").arg(author, date, body));
            }
        }
        if (mComments->count() == 0)
            mComments->addItem("No comments yet.");

        if (item.value("HasThumbnail", false))
            FetchThumbnail(submissionId);
    });
}

void WorkshopPage::Download() {
    if (mCurrentId == 0 || mMasterUrl.isEmpty())
        return;

    QString suggested = mCurrentName.isEmpty() ? QString("submission") : mCurrentName;
    suggested.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
    suggested += ".nwdb";

    QString target = QFileDialog::getSaveFileName(this, "Save workshop submission", suggested);
    if (target.isEmpty())
        return;

    // Submissions are whole .nwdb databases and can run to gigabytes, so stream straight to disk
    // instead of buffering the body the way the other endpoints do.
    auto *dialog = new LoadingDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->SetText("Downloading " + mCurrentName + "...");
    dialog->DisableCancel(true);
    dialog->show();

    std::string url = MasterHttp::ResolveUrl(mMasterUrl,
        QString("/v1/workshop/download?id=%1").arg(mCurrentId)).toStdString();
    std::string token = MasterHttp::SessionToken(mMasterUrl);
    std::string path = target.toStdString();

    QPointer<LoadingDialog> dialogPtr(dialog);
    QPointer<WorkshopPage> self(this);

    std::thread([url, token, path, dialogPtr, self]() {
        std::ofstream out(path, std::ios::binary);
        bool opened = out.is_open();

        cpr::Header header {};
        if (!token.empty())
            header["Cookie"] = ".LOGINSESSION=" + token;

        cpr::Response res;
        if (opened) {
            res = cpr::Download(out, cpr::Url{url}, header, cpr::VerifySsl{false},
                cpr::ProgressCallback([dialogPtr](cpr::cpr_pf_arg_t downloadTotal,
                                                  cpr::cpr_pf_arg_t downloadNow,
                                                  cpr::cpr_pf_arg_t, cpr::cpr_pf_arg_t, intptr_t) -> bool {
                    if (downloadTotal <= 0)
                        return true;
                    double fraction = static_cast<double>(downloadNow) / static_cast<double>(downloadTotal);
                    QTimer::singleShot(0, qApp, [dialogPtr, fraction]() {
                        if (dialogPtr)
                            dialogPtr->SetProgress(fraction);
                    });
                    return true;
                }));
            out.close();
        }

        QString error;
        if (!opened)
            error = "Could not open that file for writing.";
        else if (res.error.code != cpr::ErrorCode::OK)
            error = QString::fromStdString(res.error.message);
        else if (res.status_code >= 400)
            error = QString("The master server answered HTTP %1").arg(res.status_code);

        QTimer::singleShot(0, qApp, [dialogPtr, self, error, path]() {
            if (dialogPtr)
                dialogPtr->close();
            if (!self)
                return;
            if (!error.isEmpty()) {
                // A failed download leaves a truncated or error-body file behind; don't keep it.
                std::remove(path.c_str());
                QMessageBox::warning(self, "Download", "The download failed: " + error);
                emit self->StatusChanged("Download failed.");
                return;
            }
            emit self->StatusChanged("Saved to " + QString::fromStdString(path));
        });
    }).detach();
}

void WorkshopPage::DeleteSelected() {
    if (mCurrentId == 0 || mMasterUrl.isEmpty() || !mCurrentIsMine)
        return;

    if (QMessageBox::question(this, "Delete submission",
                              QString("Delete \"%1\" from the workshop? This cannot be undone.")
                                  .arg(mCurrentName),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    QString path = QString("/v1/workshop/delete?id=%1").arg(mCurrentId);
    MasterHttp::Post(this, mMasterUrl, path, {}, [this](const MasterResponse &response) {
        if (!response.Ok) {
            QMessageBox::warning(this, "Delete submission",
                                 "Could not delete it: " + QString::fromStdString(response.Error));
            return;
        }
        emit StatusChanged("Submission deleted.");
        FetchList();
    });
}

void WorkshopPage::PostComment() {
    if (mCurrentId == 0 || mMasterUrl.isEmpty())
        return;

    QString body = mCommentBody->toPlainText().trimmed();
    if (body.isEmpty())
        return;

    mCommentButton->setEnabled(false);
    int64_t submissionId = mCurrentId;

    MasterHttp::Post(this, mMasterUrl, "/v1/workshop/comment",
                     {{"submissionId", QString::number(submissionId).toStdString()},
                      {"body", body.toStdString()}},
                     [this, submissionId](const MasterResponse &response) {
        mCommentButton->setEnabled(true);
        if (!response.Ok) {
            QMessageBox::warning(this, "Post comment",
                                 "Could not post that comment: " + QString::fromStdString(response.Error));
            return;
        }
        mCommentBody->clear();
        ShowDetails(submissionId);
    });
}
