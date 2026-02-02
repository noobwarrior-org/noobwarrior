package org.noobwarrior.android;

import android.os.Bundle;

import androidx.appcompat.widget.Toolbar;
import androidx.appcompat.app.AppCompatActivity;

public class SdkActivity extends AppCompatActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.sdk_activity);

        Toolbar toolbar = findViewById(R.id.sdk_toolbar);
        setSupportActionBar(toolbar);
    }
}