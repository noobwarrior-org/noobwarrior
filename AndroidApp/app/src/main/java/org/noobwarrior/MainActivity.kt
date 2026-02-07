package org.noobwarrior

import android.graphics.drawable.Drawable
import android.os.Build
import android.os.Bundle
import android.view.SoundEffectConstants
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.LargeTopAppBar
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.dynamicDarkColorScheme
import androidx.compose.material3.dynamicLightColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalView
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import org.noobwarrior.ui.theme.Typography

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            MainUi()
        }
    }
}

@Composable
fun Greeting(name: String, modifier: Modifier = Modifier) {
    Text(
        text = "Hello $name!",
        modifier = modifier
    )
}

@Composable
fun Action(name: String) {
    Text(text = "Action $name")
}

@OptIn(ExperimentalMaterial3ExpressiveApi::class)
@Composable
fun Category(name: String, items: List<String>) {
    val view = LocalView.current;
    Text(
        text = name,
        color = MaterialTheme.colorScheme.primary,
        modifier = Modifier.padding(start = 6.dp, bottom = 4.dp),
        style = Typography.labelMedium
    )
    Column {
        items.forEachIndexed { index, label ->
            val shape = when (index) {
                0 -> RoundedCornerShape(topStart = 16.dp, topEnd = 16.dp, bottomStart = 4.dp, bottomEnd = 4.dp) // Top button
                items.lastIndex -> RoundedCornerShape(topStart = 4.dp, topEnd = 4.dp, bottomStart = 16.dp, bottomEnd = 16.dp) // Bottom button
                else -> RoundedCornerShape(4.dp) // Middle buttons
            }

            var modifier = Modifier
                            .fillMaxWidth()
                            .height(72.dp)

            modifier = when (index) {
                0 -> modifier.padding(bottom = 1.dp)
                items.lastIndex -> modifier.padding(top = 2.dp)
                else -> modifier.padding(top = 1.dp)
            }

            Button(
                onClick = { view.playSoundEffect(SoundEffectConstants.CLICK) },
                modifier = modifier,
                shape = shape,
                contentPadding = PaddingValues(16.dp)
            ) {
                Box(
                    modifier = Modifier
                        .fillMaxHeight()
                        .aspectRatio(1F)
                        .clip(CircleShape) // Clip the background to a circle shape
                        .background(Color.Red), // Apply a background color
                        contentAlignment = Alignment.Center
                ) {
                    Image(
                        painter = painterResource(id = R.drawable.globe_24px),
                        contentDescription = stringResource(R.string.main_play_online)
                    )
                }

                Spacer(modifier = Modifier.width(12.dp))
                Column {
                    Text(text = label, modifier = Modifier.fillMaxWidth(), textAlign = TextAlign.Left, style = Typography.bodyMedium)
                    Text(text = "hi", modifier = Modifier.fillMaxWidth(), textAlign = TextAlign.Left, style = Typography.bodySmall, color = MaterialTheme.colorScheme.inverseOnSurface)
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3ExpressiveApi::class, ExperimentalMaterial3Api::class)
@Preview(showBackground = true)
@Composable
fun MainUi() {
    val dynamicColor = Build.VERSION.SDK_INT >= Build.VERSION_CODES.S
    val darkTheme = isSystemInDarkTheme()
    val colors = when {
        dynamicColor && darkTheme -> dynamicDarkColorScheme(LocalContext.current)
        dynamicColor && !darkTheme -> dynamicLightColorScheme(LocalContext.current)
        darkTheme -> darkColorScheme()
        else -> lightColorScheme()
    }
    MaterialTheme(typography = Typography, colorScheme = colors) {
        Scaffold(
            topBar = {
                LargeTopAppBar(
                    title = {
                        Column {
                            Image(
                                painter = painterResource(id = R.drawable.icon),
                                modifier = Modifier
                                    .size(96.dp)
                                    .padding(bottom = 8.dp),
                                contentDescription = "logo"
                            )
                            Text("noobWarrior")
                        }
                    }
                    //scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()
                )
            },
            content = { innerPadding ->
                Column(modifier = Modifier
                    .padding(innerPadding)
                    .padding(16.dp)) {
                    Category("Play", listOf("Online", "Start Game Server", "Play"))
                }
            }
        )
    }
}