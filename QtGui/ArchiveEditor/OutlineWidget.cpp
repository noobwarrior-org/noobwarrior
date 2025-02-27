// === noobWarrior ===
// File: OutlineWidget.cpp
// Started by: Hattozo
// Started on: 2/17/2025
// Description: Dockable Qt widget that allows the user to explore the contents of an archive in an easily-digestible format
#include <NoobWarrior/NoobWarrior.h>
#include "OutlineWidget.h"

#include <QVBoxLayout>

using namespace NoobWarrior;

OutlineWidget::OutlineWidget(QWidget *parent) :
    mSearchBox(new QLineEdit(this))
{
    setWindowTitle("Outline");

    auto layout = new QVBoxLayout(this);
    layout->addWidget(mSearchBox);

    mSearchBox->setPlaceholderText("Search..."); // seeeaaaaaarch.... you know you wanna search...
    setLayout(layout);
}

OutlineWidget::~OutlineWidget() {}