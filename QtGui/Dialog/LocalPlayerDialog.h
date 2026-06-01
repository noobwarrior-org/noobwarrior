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
// File: LocalPlayerDialog.h
// Started by: Hattozo
// Started on: 4/23/2026
// Description:
#pragma once
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTabWidget>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QColor>
#include <QMap>

namespace NoobWarrior {
struct AvatarBodyPart {
    QString key;
    QString label;
    QPushButton* button;
    QString colorName;
    QColor color;
};

struct AvatarItemCategory {
    QString key;
    QString label;
    QListWidget* list;
};

class LocalPlayerDialog : public QDialog {
    Q_OBJECT
public:
    LocalPlayerDialog(QWidget *parent = nullptr);
    ~LocalPlayerDialog();
protected:
    void InitWidgets();

    QWidget* BuildAvatarBody();
    QWidget* BuildItemEditor();
    void AddBodyPart(QGridLayout* grid, const QString& key, const QString& label,
                     int row, int col, int w, int h, const QString& defaultColorName);
    void PickBodyColor(AvatarBodyPart& part);
    void ApplyBodyColor(const AvatarBodyPart& part);

    QLineEdit* MakeAssetField(const QString& regKey);
    QDoubleSpinBox* MakeScaleField(const QString& regKey);

    void AddItemToCategory(AvatarItemCategory& category);
    void RemoveSelectedItem(AvatarItemCategory& category);

    void LoadFromRegistry();
    void SaveToRegistry();
private:
    QVBoxLayout* mLayout;
    QHBoxLayout* mMainLayout;
    QFormLayout* mFormLayout;

    QLineEdit* mIdInput;
    QLineEdit* mNameInput;
    QLineEdit* mDisplayNameInput;

    QMap<QString, AvatarBodyPart> mBodyParts;
    QList<AvatarItemCategory> mItemCategories;
    QMap<QString, QLineEdit*> mAssetFields;
    QMap<QString, QDoubleSpinBox*> mScaleFields;

    QDialogButtonBox* mButtonBox;
};
}
