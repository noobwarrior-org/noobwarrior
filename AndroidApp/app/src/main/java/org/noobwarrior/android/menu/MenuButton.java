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
// File: Button.java
// Started by: Hattozo
// Started on: 2/1/2026
// Description:
package org.noobwarrior.android.menu;

import android.content.Context;
import android.view.View;

import androidx.annotation.StringRes;

import com.google.android.material.button.MaterialButton;

public class MenuButton {
    private Context mContext;
    private String mText;
    private String mDescription;
    private View.OnClickListener mListener;

    public MenuButton(Context context) {
        mContext = context;
        mText = "Button";
        mDescription = "";
    }

    public String getText() {
        return mText;
    }

    public String getDescription() {
        return mDescription;
    }

    public View.OnClickListener getOnClickListener() {
        return mListener;
    }

    public MenuButton setText(@StringRes int id) {
        mText = mContext.getResources().getString(id);
        return this;
    }

    public MenuButton setText(String text) {
        mText = text;
        return this;
    }

    public MenuButton setDescription(@StringRes int id) {
        mDescription = mContext.getResources().getString(id);
        return this;
    }

    public MenuButton setDescription(String description) {
        mDescription = description;
        return this;
    }

    public MenuButton onClicked(View.OnClickListener listener) {
        mListener = listener;
        return this;
    }
}
