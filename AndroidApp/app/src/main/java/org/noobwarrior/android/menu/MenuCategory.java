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
// File: Category.java
// Started by: Hattozo
// Started on: 2/1/2026
// Description:
package org.noobwarrior.android.menu;

import android.content.Context;

import androidx.annotation.StringRes;

import java.util.ArrayList;

public class MenuCategory {
    private Context mContext;
    private String mTitle;
    private boolean mVisible;
    private ArrayList<MenuButton> mButtons;

    public MenuCategory(Context context) {
        mContext = context;
        mTitle = "Untitled";
        mVisible = true;
        mButtons = new ArrayList<>();
    }

    public String getTitle() {
        return mTitle;
    }

    public MenuCategory setTitle(@StringRes int id) {
        mTitle = mContext.getResources().getString(id);
        return this;
    }

    public MenuCategory setTitle(String str) {
        mTitle = str;
        return this;
    }

    public MenuCategory setVisible(boolean val) {
        mVisible = val;
        return this;
    }

    public MenuCategory addButton(MenuButton button) {
        mButtons.add(button);
        return this;
    }

    public ArrayList<MenuButton> getButtons() {
        return mButtons;
    }
}
