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
// File: GeneralPage.h
// Started by: Hattozo
// Started on: 7/24/2025
// Description:
#include "GeneralPage.h"

#include "../Application.h"
#include "../Style/DarculaTheme.h"
#include "../Style/FluentTheme.h"

#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>

#include <format>

using namespace NoobWarrior;

namespace {
constexpr int kThemeBaseRole = Qt::UserRole + 1;
}

GeneralPage::GeneralPage(QWidget *parent) : SettingsPage(parent) {
    Init();
    InitWidgets();
}

void GeneralPage::InitWidgets() {
    auto uiBox = new QGroupBox("User Interface");
    auto uiLayout = new QFormLayout(uiBox);
    uiBox->setLayout(uiLayout);

    mTheme = new QComboBox;
    mTheme->addItem("Fluent", FluentTheme::kBaseName);
    mTheme->setItemData(0, FluentTheme::kBaseName, kThemeBaseRole);
    mTheme->addItem("Darcula", DarculaTheme::kBaseName);
    mTheme->setItemData(1, DarculaTheme::kBaseName, kThemeBaseRole);
    for (const DeclaredStyle &style : gApp->GetCore()->GetPluginManager()->GetDeclaredStyles()) {
        if (style.Base != FluentTheme::kBaseName && style.Base != DarculaTheme::kBaseName)
            continue;
        Plugin *owner = gApp->GetCore()->GetPluginManager()->GetPluginFromIdentifier(style.OwnerIdentifier);
        const std::string ownerTitle = owner != nullptr ? owner->GetProperties().Title : style.OwnerIdentifier;
        mTheme->addItem(QString::fromStdString(std::format("{} ({})", style.Title, ownerTitle)),
                        QString::fromStdString(style.GetQualifiedId()));
        mTheme->setItemData(mTheme->count() - 1, QString::fromStdString(style.Base), kThemeBaseRole);
    }

    uiLayout->addRow(new QLabel("Theme"), mTheme);

    mColorScheme = new QComboBox;
    mColorScheme->addItem("Dark", "dark");
    mColorScheme->addItem("Light", "light");
    mColorScheme->addItem("Match System", "system");

    uiLayout->addRow(new QLabel("Color Scheme"), mColorScheme);

    Layout->addWidget(uiBox);
    Layout->addStretch();
}

const QString GeneralPage::GetTitle() {
    return "General";
}

const QString GeneralPage::GetDescription() {
    return "Configure the general state of the application.";
}

const QIcon GeneralPage::GetIcon() {
    return QIcon(":/images/silk/cog.png");
}

void GeneralPage::Deserialize(Registry* reg) {
    const QString theme = QString::fromStdString(reg->GetKeyValue<std::string>("gui.theme").value_or("fluent"));
    int index = mTheme->findData(theme);
    if (index < 0) {
        mTheme->addItem(theme + " (unavailable)", theme);
        index = mTheme->count() - 1;
    }
    mTheme->setCurrentIndex(index >= 0 ? index : 0);

    std::optional<std::string> colorScheme = reg->GetKeyValue<std::string>("gui.color_scheme");
    index = mColorScheme->findData(QString::fromStdString(colorScheme.value_or("dark")));
    mColorScheme->setCurrentIndex(index >= 0 ? index : 0);
}

QString GeneralPage::GetThemeBase(const std::string &themeId) {
    const int index = mTheme->findData(QString::fromStdString(themeId));
    const QVariant base = index >= 0 ? mTheme->itemData(index, kThemeBaseRole) : QVariant();
    return base.isValid() ? base.toString() : FluentTheme::kBaseName;
}

void GeneralPage::Serialize(Registry* reg) {
    const std::string theme = mTheme->currentData().toString().toStdString();
    const std::string colorScheme = mColorScheme->currentData().toString().toStdString();
    const std::string previousTheme = reg->GetKeyValue<std::string>("gui.theme").value_or("fluent");
    const bool themeChanged = previousTheme != theme;
    const bool colorSchemeChanged = reg->GetKeyValue<std::string>("gui.color_scheme") != colorScheme;
    if (!themeChanged && !colorSchemeChanged)
        return;

    reg->SetKeyValue<std::string>("gui.theme", theme);
    reg->SetKeyValue<std::string>("gui.color_scheme", colorScheme);
    const bool baseStyleChanged = GetThemeBase(previousTheme) != GetThemeBase(theme);
    if (themeChanged && baseStyleChanged)
        QMessageBox::information(this, "Theme Changed", "Restart noobWarrior to switch to the new theme.");
    else
        QTimer::singleShot(0, gApp, &Application::ApplyStyle);
}
