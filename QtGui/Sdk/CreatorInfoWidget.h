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
// File: CreatorInfoWidget.h
// Started by: Hattozo
// Started on: 2/8/2026
// Description:
#pragma once
#include <NoobWarrior/Roblox/Api/User.h>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <cstdint>

namespace NoobWarrior {
class CreatorInfoWidget : public QWidget {
    Q_OBJECT
public:
    CreatorInfoWidget(QWidget* parent = nullptr);
    void Update(int64_t id, Roblox::CreatorType type);
private:
    QHBoxLayout* mMainLayout;
    QVBoxLayout* mContentLayout;
    
    QLabel* mImageLabel;
    QLabel* mNameLabel;
    QLabel* mTypeLabel;
    QLabel* mIdLabel;
};
}