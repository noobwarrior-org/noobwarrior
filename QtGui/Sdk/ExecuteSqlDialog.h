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
// File: ExecuteSqlDialog.h
// Started by: Hattozo
// Started on: 2/13/2026
// Description:
#pragma once
#include <QDialog>
#include <QWidget>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;

namespace NoobWarrior {
class EmuDb;
class ExecuteSqlDialog : public QDialog {
    Q_OBJECT
public:
    static constexpr int kMaxDisplayedRows = 1000;

    ExecuteSqlDialog(EmuDb *db, QWidget *parent = nullptr);

    void Refresh();
private:
    void InitWidgets();
    void Execute();

    void ShowStatus(const QString &message, bool isError);
    void ClearResults();

    EmuDb *mDatabase;

    QPlainTextEdit *mEditor;
    QTableWidget *mResults;
    QLabel *mStatusLabel;
    QPushButton *mExecuteButton;
};
}
