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
// File: AboutDialog.cpp
// Started by: Hattozo
// Started on: 3/15/2025
// Description: Qt dialog showing noobWarrior version, contibutors, and attributions
#include "AboutDialog.h"

#include <NoobWarrior/NoobWarrior.h>

#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>

using namespace NoobWarrior;

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("About noobWarrior");

    auto *grid = new QGridLayout(this);
    setLayout(grid);

    auto *logo = new QLabel(this);
    logo->setWordWrap(true);
    logo->setTextFormat(Qt::TextFormat::RichText);
    logo->setStyleSheet("* { line-height: 0px; }");
    logo->setText("<center><h1><img src=\":/images/icon64.png\">noobWarrior</h1><font color=\"gray\"><h3>v" NOOBWARRIOR_VERSION "</h3></font><h3>All-in-one toolkit for archiving, viewing, editing, and playing Roblox content offline</h3></center>");
    logo->setMaximumHeight(160);

    auto *text = new QTextEdit(this);
    text->setReadOnly(true);
    text->insertPlainText("Authors\n" NOOBWARRIOR_AUTHORS "\nContributors\n" NOOBWARRIOR_CONTRIBUTORS "\nAttributions\n" NOOBWARRIOR_ATTRIBUTIONS_BRIEF);
    // do all of this retarded shit just to center the text
    text->selectAll(); text->setAlignment(Qt::AlignCenter); auto retard = text->textCursor(); retard.clearSelection(); text->setTextCursor(retard);
    text->moveCursor(QTextCursor::Start); // scroll the text widget to the top

    grid->addWidget(logo);
    grid->addWidget(text);
}

AboutDialog::~AboutDialog() = default;