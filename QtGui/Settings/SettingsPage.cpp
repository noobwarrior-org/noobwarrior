// === noobWarrior ===
// File: SettingsPage.cpp
// Started by: Hattozo
// Started on: 7/2/2025
// Description:
#include "SettingsPage.h"

#include <QLabel>

using namespace NoobWarrior;

SettingsPage::SettingsPage(QWidget *parent) : QWidget(parent),
    Layout(new QVBoxLayout(this))
{}

void SettingsPage::Init() {
    auto title = new QLabel(GetTitle());
    QFont font = title->font();
    font.setBold(true);
    font.setPointSize(14);
    title->setFont(font);

    auto desc = new QLabel(GetDescription());
    desc->setWordWrap(true);

    Layout->addWidget(title);
    Layout->addWidget(desc);
    Layout->addStretch();
}
