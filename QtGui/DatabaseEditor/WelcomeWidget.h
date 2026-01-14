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
// File: WelcomeWidget.h
// Started by: Hattozo
// Started on: 11/27/2025
// Description: Starting page for Database Editor
#pragma once
#include <QWidget>
#include <QGridLayout>
#include <QLabel>

namespace NoobWarrior {
class WelcomeWidget : public QWidget {
    Q_OBJECT
public:
    WelcomeWidget(QWidget *parent = nullptr);
    void InitWidgets();
private:
    QGridLayout* mLayout;
    QLabel* mHeader;

    QFrame* mStartFrame;
    QVBoxLayout* mStartLayout;
    QLabel* mStartLabel;

    QFrame* mRecentFrame;
    QVBoxLayout* mRecentLayout;
    QLabel* mRecentLabel;

    QFrame* mDatabasesFrame;
    QVBoxLayout* mDatabasesLayout;
    QLabel* mDatabasesLabel;
};
}