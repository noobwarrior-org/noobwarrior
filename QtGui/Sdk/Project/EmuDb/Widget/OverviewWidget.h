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
// File: OverviewWidget.h
// Started by: Hattozo
// Started on: 7/4/2025
// Description:
#pragma once
#include <QVBoxLayout>
#include <QGridLayout>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <QWidget>

#include <array>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QToolButton;

namespace NoobWarrior {
class OverviewWidget : public QWidget {
    Q_OBJECT
public:
    OverviewWidget(EmuDb *db, QWidget *parent = nullptr);

    void Refresh();
    void RefreshAll();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    void InitWidgets();
    void InitMetadataBox();
    void InitContentsBox();
    void InitSettingsBox();
    void InitStorageBox();
    void InitHealthBox();

    void CommitMetadata();
    void RefreshMetadata();
    void RefreshStatistics();

    void SetIconFromFile();
    void ResetIcon();
    void ShowIcon();

    // Maintenance actions. Each one warns about what it is about to do before doing it, because
    // all of them commit the project's pending edits as a side effect.
    void Compact();
    void TrimFreePages();
    void EnableIncrementalVacuum();
    void CheckIntegrity();
    void CollectOrphanedBlobs();
    void OpenSqlConsole();
    
    bool ConfirmCommitBefore(const QString &actionDescription);

    EmuDb *mDatabase;
    QVBoxLayout *ToplevelLayout;
    QGridLayout *ContentLayout;

    QLabel *mOverviewLabel = nullptr;
    QLabel *mPathLabel = nullptr;

    QLabel *mIconLabel = nullptr;
    QLineEdit *mTitleField = nullptr;
    QPlainTextEdit *mDescriptionField = nullptr;
    QLineEdit *mVersionField = nullptr;
    QLineEdit *mAuthorField = nullptr;
    QCheckBox *mMutableCheckBox = nullptr;

    //////////////////// Contents ////////////////////
    std::array<QToolButton*, ItemTypeCount> mCountButtons {};
    QLabel *mUniverseLabel = nullptr;

    //////////////////// Storage ////////////////////
    QLabel *mTotalSizeLabel = nullptr;
    QLabel *mBlobSizeLabel = nullptr;
    QLabel *mFreeSpaceLabel = nullptr;
    QLabel *mAutoVacuumLabel = nullptr;
    QPushButton *mTrimButton = nullptr;
    QPushButton *mCompactButton = nullptr;
    QPushButton *mIncrementalButton = nullptr;

    //////////////////// Health ////////////////////
    QLabel *mSchemaLabel = nullptr;
    QLabel *mAccessLabel = nullptr;
    QLabel *mMountLabel = nullptr;

    // Whether RefreshStatistics() has run at least once. Keeps the expensive half off the path
    // that fires every time the user switches back to this tab.
    bool mStatisticsLoaded = false;
};
}
