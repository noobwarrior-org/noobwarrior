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
    logo->setTextFormat(Qt::TextFormat::RichText);
    logo->setStyleSheet("* { line-height: 0px; }");
    logo->setText("<center><h1><img src=\":/images/icon64.png\">noobWarrior</h1><font color=\"gray\"><h3>v" NOOBWARRIOR_VERSION "</h3></font></center>");
    logo->setMaximumHeight(128);

    auto *text = new QTextEdit(this);
    text->setReadOnly(true);
    text->insertPlainText("Authors\n" NOOBWARRIOR_AUTHORS "\nContributors\n" NOOBWARRIOR_CONTRIBUTORS "\nAttributions\n" NOOBWARRIOR_ATTRIBUTIONS_BRIEF);
    // do all of this retarded shit just to center the text
    text->selectAll(); text->setAlignment(Qt::AlignCenter); auto retard = text->textCursor(); retard.clearSelection(); text->setTextCursor(retard);
    text->moveCursor(QTextCursor::Start); // scroll the text widget to the top

    grid->addWidget(logo);
    grid->addWidget(text);
}