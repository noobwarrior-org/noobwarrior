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
#include <QVBoxLayout>
#include <QLabel>

#include <cstdint>

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
    QVBoxLayout* mLayout;
    QLabel* mNoPlaceLabel;
    QLabel* mNoPlaceDescriptionLabel;

    QLabel* mThumbnailLabel;
    QLabel* mTitleLabel;
    QLabel* mCreatorLabel;
    QLabel* mDescriptionLabel;
};
}