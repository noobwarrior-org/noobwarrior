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
// File: PlaceInfoCardWidget.cpp
// Started by: Hattozo
// Started on: 5/9/2026
// Description:
#include "PlaceInfoCardWidget.h"

#include <QImage>
#include <QScrollBar>

using namespace NoobWarrior;

PlaceInfoCardWidget::PlaceInfoCardWidget(QWidget *parent) : QFrame(parent) {
    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    InitWidgets();
}

void PlaceInfoCardWidget::Refresh(EmuDb* db, int64_t id) {
    mNoPlaceLabel->setVisible(db == nullptr);
    mNoPlaceDescriptionLabel->setVisible(db == nullptr);

    mTitleLabel->setVisible(db != nullptr);
    mCreatorLabel->setVisible(db != nullptr);
    mDescriptionLabel->setVisible(db != nullptr);

    if (db == nullptr) {
        SetCarouselVisible(false);
        return;
    }

    Statement placeInfoStmt = db->PrepareStatement("SELECT Name, Description, UserId, GroupId FROM Asset WHERE Id = ?;");
    placeInfoStmt.Bind(1, id);
    if (placeInfoStmt.Step() != SQLITE_ROW) {
        mTitleLabel->setText("No place found");
        mDescriptionLabel->setText("This universe does not contain a place.");
        mCreatorLabel->setText("By No One!");
        SetCarouselVisible(false);
        return;
    }
    std::string name = placeInfoStmt.GetStringFromColumnIndex(0);
    std::string desc = placeInfoStmt.GetStringFromColumnIndex(1);
    mTitleLabel->setText(QString::fromStdString(name));
    mDescriptionLabel->setText(QString::fromStdString(desc));

    // Resolve the creator (the place's owning user or group) to a name, if it's in the database.
    int64_t userId  = placeInfoStmt.IsColumnIndexNull(2) ? 0 : placeInfoStmt.GetInt64FromColumnIndex(2);
    int64_t groupId = placeInfoStmt.IsColumnIndexNull(3) ? 0 : placeInfoStmt.GetInt64FromColumnIndex(3);
    std::optional<std::string> creatorName;
    if (userId != 0)
        creatorName = db->GetItemName(ItemType::User, userId);
    else if (groupId != 0)
        creatorName = db->GetItemName(ItemType::Group, groupId);
    mCreatorLabel->setText(creatorName.has_value() && !creatorName->empty()
        ? QString("By %1").arg(QString::fromStdString(*creatorName))
        : "By No One!");

    LoadCarousel(db, id);

    // Scrolled back to the top for the newly selected place.
    mScrollArea->verticalScrollBar()->setValue(0);
}

void PlaceInfoCardWidget::LoadCarousel(EmuDb* db, int64_t placeId) {
    mCarouselTimer->stop();
    mFadeAnimation->stop();
    mFadingOut = false;
    mCarouselPixmaps.clear();
    mCarouselIndex = 0;

    auto toPixmap = [](const std::vector<unsigned char>& bytes) -> QPixmap {
        QImage img;
        if (!img.loadFromData(bytes))
            return QPixmap();
        return QPixmap::fromImage(img).scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    };

    // The game's thumbnails (the wide carousel images), in the order they were stored.
    Statement stmt = db->PrepareStatement("SELECT Thumbnail FROM AssetPlaceThumbnail WHERE Id = ?;");
    if (!stmt.Fail()) {
        stmt.Bind(1, placeId);
        while (stmt.Step() == SQLITE_ROW) {
            QPixmap pm = toPixmap(db->RetrieveImageData(ItemType::Asset, stmt.GetInt64FromColumnIndex(0)));
            if (!pm.isNull())
                mCarouselPixmaps.push_back(pm);
        }
    }

    // Fall back to the place's own image (its game icon) when it has no carousel thumbnails.
    if (mCarouselPixmaps.empty()) {
        QPixmap pm = toPixmap(db->RetrieveImageData(ItemType::Asset, placeId));
        if (!pm.isNull())
            mCarouselPixmaps.push_back(pm);
    }

    SetCarouselVisible(!mCarouselPixmaps.empty());
    if (mCarouselPixmaps.empty())
        return;

    ShowThumbnail(0, false);

    // Auto-advance (with a fade) only makes sense when there's more than one frame to show.
    bool multiple = mCarouselPixmaps.size() > 1;
    mPrevButton->setVisible(multiple);
    mNextButton->setVisible(multiple);
    mCarouselIndicator->setVisible(multiple);
    if (multiple)
        mCarouselTimer->start();
}

void PlaceInfoCardWidget::ShowThumbnail(int index, bool animate) {
    if (mCarouselPixmaps.empty())
        return;

    const int n = static_cast<int>(mCarouselPixmaps.size());
    index = ((index % n) + n) % n; // wrap around in both directions

    if (!animate) {
        mCarouselIndex = index;
        mThumbnailLabel->setPixmap(mCarouselPixmaps[index]);
        mThumbnailOpacity->setOpacity(1.0);
        UpdateCarouselControls();
        return;
    }

    // Fade the current frame out; OnFadeFinished swaps in the new one and fades it back in.
    mPendingIndex = index;
    mFadingOut = true;
    mFadeAnimation->stop();
    mFadeAnimation->setStartValue(mThumbnailOpacity->opacity());
    mFadeAnimation->setEndValue(0.0);
    mFadeAnimation->start();
}

void PlaceInfoCardWidget::OnFadeFinished() {
    if (!mFadingOut)
        return; // the fade-in just finished; nothing to do

    mFadingOut = false;
    mCarouselIndex = mPendingIndex;
    mThumbnailLabel->setPixmap(mCarouselPixmaps[mCarouselIndex]);
    UpdateCarouselControls();

    mFadeAnimation->stop();
    mFadeAnimation->setStartValue(0.0);
    mFadeAnimation->setEndValue(1.0);
    mFadeAnimation->start();
}

void PlaceInfoCardWidget::NextThumbnail() {
    if (mCarouselPixmaps.size() <= 1)
        return;
    if (mCarouselTimer->isActive())
        mCarouselTimer->start(); // restart the countdown after a manual step
    ShowThumbnail(mCarouselIndex + 1, true);
}

void PlaceInfoCardWidget::PrevThumbnail() {
    if (mCarouselPixmaps.size() <= 1)
        return;
    if (mCarouselTimer->isActive())
        mCarouselTimer->start();
    ShowThumbnail(mCarouselIndex - 1, true);
}

void PlaceInfoCardWidget::UpdateCarouselControls() {
    const int n = static_cast<int>(mCarouselPixmaps.size());
    mCarouselIndicator->setText(n > 0 ? QString("%1 / %2").arg(mCarouselIndex + 1).arg(n) : "");
}

void PlaceInfoCardWidget::SetCarouselVisible(bool visible) {
    if (!visible) {
        mCarouselTimer->stop();
        mFadeAnimation->stop();
        mFadingOut = false;
        mThumbnailLabel->clear();
    }
    mThumbnailLabel->setVisible(visible);
    mPrevButton->setVisible(visible);
    mNextButton->setVisible(visible);
    mCarouselIndicator->setVisible(visible);
}

void PlaceInfoCardWidget::InitWidgets() {
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    mScrollArea = new QScrollArea(this);
    mScrollArea->setWidgetResizable(true);
    mScrollArea->setFrameShape(QFrame::NoFrame);
    mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    mContentWidget = new QWidget();
    mLayout = new QVBoxLayout(mContentWidget);

    mNoPlaceLabel = new QLabel("No place selected");
    mNoPlaceLabel->setWordWrap(true);

    mNoPlaceDescriptionLabel = new QLabel("Select a place that you want to host by clicking on one of the places located within the sidebar beside me. Its information will show up here.");
    mNoPlaceDescriptionLabel->setWordWrap(true);

    // --- thumbnail carousel ---
    mThumbnailLabel = new QLabel();
    mThumbnailLabel->setAlignment(Qt::AlignCenter);
    mThumbnailLabel->setMinimumHeight(144);

    mThumbnailOpacity = new QGraphicsOpacityEffect(mThumbnailLabel);
    mThumbnailOpacity->setOpacity(1.0);
    mThumbnailLabel->setGraphicsEffect(mThumbnailOpacity);

    mFadeAnimation = new QPropertyAnimation(mThumbnailOpacity, "opacity", this);
    mFadeAnimation->setDuration(220);
    connect(mFadeAnimation, &QPropertyAnimation::finished, this, &PlaceInfoCardWidget::OnFadeFinished);

    mCarouselNavLayout = new QHBoxLayout();
    mPrevButton = new QPushButton(QString::fromUtf8("‹")); // ‹
    mNextButton = new QPushButton(QString::fromUtf8("›")); // ›
    mPrevButton->setFixedWidth(28);
    mNextButton->setFixedWidth(28);
    mCarouselIndicator = new QLabel();
    mCarouselIndicator->setAlignment(Qt::AlignCenter);

    connect(mPrevButton, &QPushButton::clicked, this, [this]() { PrevThumbnail(); });
    connect(mNextButton, &QPushButton::clicked, this, [this]() { NextThumbnail(); });

    mCarouselNavLayout->addStretch();
    mCarouselNavLayout->addWidget(mPrevButton);
    mCarouselNavLayout->addWidget(mCarouselIndicator);
    mCarouselNavLayout->addWidget(mNextButton);
    mCarouselNavLayout->addStretch();

    mCarouselTimer = new QTimer(this);
    mCarouselTimer->setInterval(4000);
    connect(mCarouselTimer, &QTimer::timeout, this, [this]() { NextThumbnail(); });

    mTitleLabel = new QLabel("Place");
    mTitleLabel->setWordWrap(true);

    QFont titleFont = mTitleLabel->font();
    titleFont.setPointSize(16);
    mTitleLabel->setFont(titleFont);
    mNoPlaceLabel->setFont(titleFont);

    mCreatorLabel = new QLabel("By No One!");
    mCreatorLabel->setWordWrap(true);

    QFont creatorFont = mCreatorLabel->font();
    creatorFont.setPointSize(12);
    mCreatorLabel->setFont(creatorFont);

    mDescriptionLabel = new QLabel("Description");
    mDescriptionLabel->setWordWrap(true);

    QFont descFont = mTitleLabel->font();
    descFont.setPointSize(10);
    mDescriptionLabel->setFont(descFont);
    mNoPlaceDescriptionLabel->setFont(descFont);

    mLayout->addWidget(mNoPlaceLabel);
    mLayout->addWidget(mNoPlaceDescriptionLabel);

    mLayout->addWidget(mThumbnailLabel);
    mLayout->addLayout(mCarouselNavLayout);
    mLayout->addWidget(mTitleLabel);
    mLayout->addWidget(mCreatorLabel);
    mLayout->addWidget(mDescriptionLabel);
    mLayout->addStretch();

    mScrollArea->setWidget(mContentWidget);
    outerLayout->addWidget(mScrollArea);

    Refresh(nullptr, 0);
}
