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
// Description: the main activity
package org.noobwarrior.android;

import android.content.Intent;
import android.os.Bundle;

import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import org.noobwarrior.android.menu.MenuButton;
import org.noobwarrior.android.menu.MenuActivity;
import org.noobwarrior.android.menu.MenuCategory;

import java.util.ArrayList;

public class LauncherActivity extends MenuActivity {
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

    private MenuCategory playCategory;
    private MenuCategory devCategory;
    private MenuCategory appCategory;

    private MenuButton onlineButton;
    private MenuButton hostServerButton;

    private MenuButton sdkButton;

    private MenuButton databasesButton;
    private MenuButton pluginsButton;
    private MenuButton settingsButton;
    private MenuButton aboutButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        this.onlineButton = new MenuButton(this)
                .setText(R.string.main_play_online);
        this.hostServerButton = new MenuButton(this)
                .setText(R.string.main_play_hostserver);

        this.sdkButton = new MenuButton(this)
                .setText(R.string.main_dev_sdk)
                .onClicked((v) -> {
                    Intent intent = new Intent(this, SdkActivity.class);
                    startActivity(intent);
                });

        this.playCategory = new MenuCategory(this)
                .setTitle(R.string.category_play)
                .addButton(this.onlineButton)
                .addButton(this.hostServerButton);

        this.devCategory = new MenuCategory(this)
                .setTitle(R.string.category_dev)
                .addButton(this.sdkButton);

        ArrayList<MenuCategory> categories = new ArrayList<>();
        categories.add(playCategory);
        categories.add(devCategory);
        InitMenu(R.string.app_name, categories);

        if (mMainLayout != null) {
            ViewCompat.setOnApplyWindowInsetsListener(mMainLayout, (v, insets) -> {
                Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
                v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
                return insets;
            });
        }
    }
}