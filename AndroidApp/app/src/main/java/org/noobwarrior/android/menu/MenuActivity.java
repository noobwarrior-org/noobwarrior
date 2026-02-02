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
// File: ButtonGroupsActivity.java
// Started by: Hattozo
// Started on: 2/1/2026
// Description:
package org.noobwarrior.android.menu;

import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.StringRes;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.coordinatorlayout.widget.CoordinatorLayout;
import androidx.core.content.ContextCompat;
import androidx.core.widget.NestedScrollView;

import com.google.android.material.appbar.AppBarLayout;
import com.google.android.material.appbar.CollapsingToolbarLayout;
import com.google.android.material.appbar.MaterialToolbar;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.button.MaterialButtonToggleGroup;

import org.noobwarrior.android.NoobActivity;
import org.noobwarrior.android.R;

import java.util.ArrayList;

public abstract class MenuActivity extends NoobActivity {
    protected CoordinatorLayout mMainLayout;
    private String mTitle;

    protected void InitMenu(@StringRes int title, ArrayList<MenuCategory> categories) {
        mTitle = getString(title);
        
        mMainLayout = new CoordinatorLayout(this);
        mMainLayout.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        mMainLayout.setBackgroundColor(ContextCompat.getColor(this, android.R.color.system_neutral2_50));

        AppBarLayout appBarLayout = new AppBarLayout(this);
        appBarLayout.setBackground(new ColorDrawable(Color.TRANSPARENT));
        appBarLayout.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        appBarLayout.setFitsSystemWindows(true);

        CollapsingToolbarLayout collapsingToolbarLayout = new CollapsingToolbarLayout(this);
        collapsingToolbarLayout.setBackground(new ColorDrawable(Color.TRANSPARENT));
        collapsingToolbarLayout.setTitleCollapseMode(CollapsingToolbarLayout.TITLE_COLLAPSE_MODE_SCALE);
        AppBarLayout.LayoutParams collapserLp = new AppBarLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                (int) (256 * getResources().getDisplayMetrics().density));

        collapserLp.setScrollFlags(AppBarLayout.LayoutParams.SCROLL_FLAG_SCROLL 
                | AppBarLayout.LayoutParams.SCROLL_FLAG_EXIT_UNTIL_COLLAPSED);
        
        collapsingToolbarLayout.setLayoutParams(collapserLp);
        collapsingToolbarLayout.setTitle(mTitle);
        collapsingToolbarLayout.setExpandedTitleTextSize(96);
        collapsingToolbarLayout.setExpandedTitleMarginStart((int) (32 * getResources().getDisplayMetrics().density));
        collapsingToolbarLayout.setFitsSystemWindows(true);

        ImageView iconImg = new ImageView(this);
        iconImg.setImageDrawable(AppCompatResources.getDrawable(this, R.drawable.icon));
        iconImg.setScaleX(0.4F);
        iconImg.setScaleY(0.4F);

        MaterialToolbar toolbar = new MaterialToolbar(this);
        CollapsingToolbarLayout.LayoutParams toolbarLp = new CollapsingToolbarLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                (int) (64 * getResources().getDisplayMetrics().density)); // Standard toolbar height

        toolbarLp.setCollapseMode(CollapsingToolbarLayout.LayoutParams.COLLAPSE_MODE_PIN);
        toolbar.setLayoutParams(toolbarLp);
        toolbar.setBackground(new ColorDrawable(Color.TRANSPARENT));
        setSupportActionBar(toolbar);

        collapsingToolbarLayout.addView(iconImg);
        collapsingToolbarLayout.addView(toolbar);
        appBarLayout.addView(collapsingToolbarLayout);

        NestedScrollView nestedScrollView = new NestedScrollView(this);
        CoordinatorLayout.LayoutParams lp = new CoordinatorLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT);
        lp.setBehavior(new AppBarLayout.ScrollingViewBehavior());
        nestedScrollView.setLayoutParams(lp);

        LinearLayout contentLayout = new LinearLayout(this);
        contentLayout.setOrientation(LinearLayout.VERTICAL);
        contentLayout.setPadding(32, 32, 32, 32);

        for (MenuCategory category : categories) {
            TextView categoryView = new TextView(this);
            categoryView.setText(category.getTitle());
            categoryView.setPadding(16, 32, 0, 16);
            categoryView.setTextAppearance(com.google.android.material.R.style.TextAppearance_Material3_TitleSmall);
            categoryView.setTextColor(ContextCompat.getColor(this, android.R.color.system_accent1_400));
            contentLayout.addView(categoryView);

            // Create the container for connected buttons
            MaterialButtonToggleGroup group = new MaterialButtonToggleGroup(this);
            group.setOrientation(MaterialButtonToggleGroup.VERTICAL);
            group.setLayoutParams(new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));
            
            // This makes the buttons touch each other
            group.setSelectionRequired(false);
            group.setSingleSelection(true);

            for (MenuButton item : category.getButtons()) {
                MaterialButton button = new MaterialButton(this, null, com.google.android.material.R.attr.materialButtonTonalStyle);
                button.setText(item.getText());
                button.setTextAlignment(View.TEXT_ALIGNMENT_TEXT_START);
                button.setPadding(48, 48, 48, 48);

                button.setInsetTop(0);
                button.setInsetBottom(0);

                if (item.getOnClickListener() != null) {
                    button.setOnClickListener(item.getOnClickListener());
                }

                group.addView(button);
            }
            contentLayout.addView(group);
        }
        
        nestedScrollView.addView(contentLayout);
        
        mMainLayout.addView(appBarLayout);
        mMainLayout.addView(nestedScrollView);

        setContentView(mMainLayout);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }
}
