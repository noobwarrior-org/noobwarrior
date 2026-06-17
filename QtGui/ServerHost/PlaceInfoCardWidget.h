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
// File: PlaceInfoCardWidget.h
// Started by: Hattozo
// Started on: 5/9/2026
// Description:
#pragma once
#include <QFrame>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QPixmap>

#include <cstdint>
#include <vector>

#include <NoobWarrior/EmuDb/EmuDb.h>

namespace NoobWarrior {
class PlaceInfoCardWidget : public QFrame {
    Q_OBJECT
public:
    PlaceInfoCardWidget(QWidget *parent = nullptr);
    void Refresh(EmuDb* db, int64_t id);
protected:
    void InitWidgets();
private:
    // Loads the place's thumbnail carousel (its AssetPlaceThumbnail images, or the game icon as a
    // fallback) and shows the first frame.
    void LoadCarousel(EmuDb* db, int64_t placeId);
    void ShowThumbnail(int index, bool animate);
    void NextThumbnail();
    void PrevThumbnail();
    void OnFadeFinished();
    void UpdateCarouselControls();
    void SetCarouselVisible(bool visible);

    // The whole card scrolls, so a place with a huge description doesn't blow up the panel.
    QScrollArea* mScrollArea;
    QWidget* mContentWidget;
    QVBoxLayout* mLayout;

    QLabel* mNoPlaceLabel;
    QLabel* mNoPlaceDescriptionLabel;

    QLabel* mThumbnailLabel;
    QGraphicsOpacityEffect* mThumbnailOpacity;
    QPropertyAnimation* mFadeAnimation;

    QHBoxLayout* mCarouselNavLayout;
    QPushButton* mPrevButton;
    QPushButton* mNextButton;
    QLabel* mCarouselIndicator;
    QTimer* mCarouselTimer;

    std::vector<QPixmap> mCarouselPixmaps;
    int mCarouselIndex { 0 };
    int mPendingIndex { 0 };
    bool mFadingOut { false };

    QLabel* mTitleLabel;
    QLabel* mCreatorLabel;
    QLabel* mDescriptionLabel;
};
}
