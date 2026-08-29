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
// File: ExecuteSqlDialog.cpp
// Started by: Hattozo
// Started on: 2/13/2026
// Description: lets you execute sql code into the emulator database
#include "ExecuteSqlDialog.h"

#include <NoobWarrior/EmuDb/EmuDb.h>

#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>

#include <sqlite3.h>

using namespace NoobWarrior;

static QString ColumnText(sqlite3_stmt *stmt, int column) {
    switch (sqlite3_column_type(stmt, column)) {
    case SQLITE_NULL:
        return "NULL";
    case SQLITE_BLOB:
        return QString("<blob, %1 bytes>").arg(sqlite3_column_bytes(stmt, column));
    default: {
        const unsigned char *text = sqlite3_column_text(stmt, column);
        return text != nullptr ? QString::fromUtf8(reinterpret_cast<const char*>(text)) : QString();
    }
    }
}

ExecuteSqlDialog::ExecuteSqlDialog(EmuDb *db, QWidget *parent) : QDialog(parent),
    mDatabase(db),
    mEditor(nullptr),
    mResults(nullptr),
    mStatusLabel(nullptr),
    mExecuteButton(nullptr)
{
    setWindowTitle("Execute SQL");
    resize(760, 560);

    InitWidgets();
    Refresh();
}

void ExecuteSqlDialog::InitWidgets() {
    auto *layout = new QVBoxLayout(this);

    auto *splitter = new QSplitter(Qt::Vertical, this);

    mEditor = new QPlainTextEdit(splitter);
    mEditor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    mEditor->setPlaceholderText("SELECT * FROM Asset LIMIT 50;");
    mEditor->setTabChangesFocus(false);
    splitter->addWidget(mEditor);

    mResults = new QTableWidget(splitter);
    mResults->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mResults->setSelectionBehavior(QAbstractItemView::SelectItems);
    mResults->setAlternatingRowColors(true);
    mResults->horizontalHeader()->setStretchLastSection(true);
    mResults->verticalHeader()->setVisible(false);
    splitter->addWidget(mResults);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    mStatusLabel = new QLabel(this);
    mStatusLabel->setWordWrap(true);
    mStatusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(mStatusLabel);

    auto *buttonLayout = new QHBoxLayout();

    auto *note = new QLabel("Changes join the project's unsaved edits; save the project to commit them.", this);
    note->setForegroundRole(QPalette::PlaceholderText);
    note->setWordWrap(true);
    buttonLayout->addWidget(note, 1);

    mExecuteButton = new QPushButton(QIcon(":/images/silk/lightning.png"), "Execute", this);
    mExecuteButton->setShortcut(QKeySequence(Qt::Key_F5));
    mExecuteButton->setDefault(true);
    connect(mExecuteButton, &QPushButton::clicked, this, &ExecuteSqlDialog::Execute);
    buttonLayout->addWidget(mExecuteButton);

    auto *ctrlReturn = new QAction(this);
    ctrlReturn->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
    connect(ctrlReturn, &QAction::triggered, this, &ExecuteSqlDialog::Execute);
    addAction(ctrlReturn);

    auto *clearButton = new QPushButton("Clear", this);
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        mEditor->clear();
        ClearResults();
        ShowStatus(QString(), false);
    });
    buttonLayout->addWidget(clearButton);

    auto *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);

    layout->addLayout(buttonLayout);
}

void ExecuteSqlDialog::Refresh() {
    if (mDatabase == nullptr) {
        setWindowTitle("Execute SQL");
        if (mExecuteButton != nullptr)
            mExecuteButton->setEnabled(false);
        return;
    }

    setWindowTitle("Execute SQL for " + QString::fromStdString(mDatabase->GetFileName()));
    if (mExecuteButton != nullptr)
        mExecuteButton->setEnabled(!mDatabase->Fail());
}

void ExecuteSqlDialog::ClearResults() {
    mResults->clear();
    mResults->setRowCount(0);
    mResults->setColumnCount(0);
}

void ExecuteSqlDialog::ShowStatus(const QString &message, bool isError) {
    mStatusLabel->setText(message);
    mStatusLabel->setStyleSheet(isError ? "color: #c0392b;" : QString());
}

void ExecuteSqlDialog::Execute() {
    if (mDatabase == nullptr || mDatabase->Fail())
        return;

    const std::string sql = mEditor->toPlainText().toStdString();
    if (sql.find_first_not_of(" \t\r\n") == std::string::npos) {
        ClearResults();
        ShowStatus("Nothing to run.", false);
        return;
    }

    sqlite3 *handle = mDatabase->Get();

    // Statement only ever compiles the first statement of a string (it passes no tail pointer), and
    // ExecStatement runs them all but throws the rows away. Neither is enough for an editor where
    // the user may paste a script and still expect to see what the last SELECT returned, so the
    // prepare/step loop is spelled out here.
    const char *cursor = sql.c_str();
    const char *end = cursor + sql.size();

    int statementCount = 0;
    int64_t changesBefore = sqlite3_total_changes64(handle);

    QStringList headers;
    QList<QStringList> rows;
    bool truncated = false;

    QElapsedTimer timer;
    timer.start();

    while (cursor < end) {
        sqlite3_stmt *stmt = nullptr;
        const char *tail = nullptr;
        int rc = sqlite3_prepare_v2(handle, cursor, static_cast<int>(end - cursor), &stmt, &tail);

        if (rc != SQLITE_OK) {
            ClearResults();
            ShowStatus(QString("Statement %1 failed to compile: %2")
                           .arg(statementCount + 1)
                           .arg(QString::fromUtf8(sqlite3_errmsg(handle))), true);
            return;
        }

        // Trailing whitespace or a comment compiles to no statement at all.
        if (stmt == nullptr) {
            if (tail == nullptr || tail == cursor)
                break;
            cursor = tail;
            continue;
        }

        // Each statement replaces the previous one's rows, so what is left at the end is the last
        // result set the script produced -- the one the user was looking for.
        headers.clear();
        rows.clear();
        truncated = false;

        const int columnCount = sqlite3_column_count(stmt);
        for (int i = 0; i < columnCount; i++)
            headers << QString::fromUtf8(sqlite3_column_name(stmt, i));

        int stepRc = SQLITE_OK;
        while ((stepRc = sqlite3_step(stmt)) == SQLITE_ROW) {
            if (rows.size() >= kMaxDisplayedRows) {
                truncated = true;
                break;
            }
            QStringList row;
            for (int i = 0; i < columnCount; i++)
                row << ColumnText(stmt, i);
            rows << row;
        }

        if (stepRc != SQLITE_DONE && stepRc != SQLITE_ROW) {
            QString error = QString::fromUtf8(sqlite3_errmsg(handle));
            sqlite3_finalize(stmt);
            ClearResults();
            ShowStatus(QString("Statement %1 failed: %2").arg(statementCount + 1).arg(error), true);
            return;
        }

        sqlite3_finalize(stmt);
        statementCount++;
        cursor = tail;
    }

    const qint64 elapsed = timer.elapsed();
    const int64_t changed = sqlite3_total_changes64(handle) - changesBefore;

    ClearResults();
    mResults->setColumnCount(static_cast<int>(headers.size()));
    mResults->setHorizontalHeaderLabels(headers);
    mResults->setRowCount(static_cast<int>(rows.size()));
    for (int r = 0; r < rows.size(); r++) {
        for (int c = 0; c < rows[r].size(); c++)
            mResults->setItem(r, c, new QTableWidgetItem(rows[r][c]));
    }
    mResults->resizeColumnsToContents();

    QString status = QString("%1 statement%2 in %3 ms")
        .arg(statementCount)
        .arg(statementCount == 1 ? "" : "s")
        .arg(elapsed);
    if (!headers.isEmpty()) {
        status += QString(" · %1 row%2").arg(rows.size()).arg(rows.size() == 1 ? "" : "s");
        if (truncated)
            status += QString(" (stopped at %1)").arg(kMaxDisplayedRows);
    }
    if (changed > 0) {
        status += QString(" · %1 row%2 changed").arg(changed).arg(changed == 1 ? "" : "s");
        mDatabase->MarkDirty();
    }
    ShowStatus(status, false);
}
