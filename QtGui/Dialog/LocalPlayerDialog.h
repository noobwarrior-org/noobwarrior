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
#include <QComboBox>
#include <QPushButton>
#include <QTabWidget>
#include <QListWidget>
#include <QColor>
#include <QMap>

namespace NoobWarrior {
struct AvatarBodyPart {
    QString key;          // registry sub-key, e.g. "head"
    QString label;        // human readable name shown in the color picker title
    QPushButton* button;  // swatch button the user clicks to recolor
    QColor color;         // currently selected colour
};

struct AvatarItemCategory {
    QString key;          // registry sub-key, e.g. "hats"
    QString label;        // tab title
    QListWidget* list;    // equipped asset ids
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
                     int row, int col, int w, int h, const QColor& defaultColor);
    void PickBodyColor(AvatarBodyPart& part);
    // Repaints a swatch button to reflect part.color.
    void ApplyBodyColor(const AvatarBodyPart& part);

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
    QComboBox* mAvatarTypeInput;

    QMap<QString, AvatarBodyPart> mBodyParts;
    QList<AvatarItemCategory> mItemCategories;

    QDialogButtonBox* mButtonBox;
};
}
