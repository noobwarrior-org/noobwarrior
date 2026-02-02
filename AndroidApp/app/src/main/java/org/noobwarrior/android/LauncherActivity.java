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
// File: LauncherActivity.java
// Started by: Hattozo
// Started on: 2/1/2026
// Description:
package org.noobwarrior.android;

import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.os.Bundle;

import androidx.coordinatorlayout.widget.CoordinatorLayout;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

public class LauncherActivity extends NoobActivity {
    public static final String[][] PLAY = {
        {"@drawable/globe_24px", "Online"},
        {"@drawable/storage_24px", "Start Game Server"}
    };

    public static final String[][] DEV = {
        {"@drawable/sdk_24px", "Launch SDK"},
    };

    public static final String[][] APP = {
        {"@drawable/database_24px", "Databases"},
        {"@drawable/extension_24px", "Plugins"},
        {"@drawable/settings_24px", "Settings"},
        {"@drawable/info_24px", "About"},
    };

    private CoordinatorLayout mainLayout;

    Drawable draw = Drawable.createFromPath("@drawable/globe_24px");

    protected void createBullshit() {
        this.mainLayout = new CoordinatorLayout(this);
        this.mainLayout.setLayoutParams(new CoordinatorLayout.LayoutParams(
                CoordinatorLayout.LayoutParams.MATCH_PARENT,
                CoordinatorLayout.LayoutParams.MATCH_PARENT));
        this.mainLayout.setBackgroundColor(Color.parseColor("@android:color/system_neutral2_50"));


    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        createBullshit();
        setContentView(mainLayout);

        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });
    }
}