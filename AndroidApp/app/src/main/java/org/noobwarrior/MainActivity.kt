package org.noobwarrior

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
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.BlendMode
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ColorFilter
import androidx.compose.ui.graphics.painter.Painter
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalView
import androidx.compose.ui.res.painterResource
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

/**
 * Defines the position of the item in the group to determine corner rounding.
 */
enum class ItemPosition {
    Top, Middle, Bottom, Single
}

/**
 * Data model for a category action.
 */
data class CategoryAction(
    val name: String,
    val description: String = "",
    val icon: Painter,
    val color: Color = Color.Blue,
    val onClick: () -> Unit = {}
)

@Composable
fun CategoryButton(
    action: CategoryAction,
    position: ItemPosition,
    modifier: Modifier = Modifier
) {
    val view = LocalView.current

    val shape = when (position) {
        ItemPosition.Top -> RoundedCornerShape(topStart = 16.dp, topEnd = 16.dp, bottomStart = 4.dp, bottomEnd = 4.dp)
        ItemPosition.Bottom -> RoundedCornerShape(topStart = 4.dp, topEnd = 4.dp, bottomStart = 16.dp, bottomEnd = 16.dp)
        ItemPosition.Single -> RoundedCornerShape(16.dp)
        ItemPosition.Middle -> RoundedCornerShape(4.dp)
    }

    Button(
        onClick = {
            view.playSoundEffect(SoundEffectConstants.CLICK)
            action.onClick()
        },
        colors = ButtonDefaults.buttonColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant,
            contentColor = MaterialTheme.colorScheme.onSurfaceVariant
        ),
        modifier = modifier
            .fillMaxWidth()
            .height(72.dp),
        shape = shape,
        contentPadding = PaddingValues(16.dp)
    ) {
        Box(
            modifier = Modifier
                .fillMaxHeight()
                .aspectRatio(1F)
                .clip(CircleShape)
                .background(action.color),
            contentAlignment = Alignment.Center
        ) {
            Image(
                painter = action.icon,
                contentDescription = action.name
            )
        }

        Spacer(modifier = Modifier.width(12.dp))
        Column {
            Text(
                text = action.name,
                modifier = Modifier.fillMaxWidth(),
                textAlign = TextAlign.Left,
                style = Typography.labelLarge
            )
            if (!action.description.isEmpty()) {
                Text(
                    text = action.description,
                    modifier = Modifier.fillMaxWidth(),
                    textAlign = TextAlign.Left,
                    style = Typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
    }
}

@Composable
fun Category(
    name: String,
    actions: List<CategoryAction>,
    modifier: Modifier = Modifier
) {
    Column(modifier = modifier) {
        Text(
            text = name,
            color = MaterialTheme.colorScheme.primary,
            modifier = Modifier.padding(start = 6.dp, bottom = 4.dp),
            style = Typography.labelLarge
        )
        Column(verticalArrangement = Arrangement.spacedBy(1.dp)) {
            actions.forEachIndexed { index, action ->
                val position = when {
                    actions.size == 1 -> ItemPosition.Single
                    index == 0 -> ItemPosition.Top
                    index == actions.lastIndex -> ItemPosition.Bottom
                    else -> ItemPosition.Middle
                }
                CategoryButton(action = action, position = position)
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
                )
            },
            content = { innerPadding ->
                Column(
                    modifier = Modifier
                        .padding(innerPadding)
                        .padding(16.dp)
                        .verticalScroll(rememberScrollState())
                ) {
                    Category(
                        name = "Play",
                        actions = listOf(
                            CategoryAction(
                                name = "Online",
                                description = "The main function of the app",
                                icon = painterResource(id = R.drawable.globe_24px),
                                color = Color(0xFF149EF0)
                            ),
                            CategoryAction(
                                name = "Start Game Server",
                                description = "Host a server with your buddies; experimental",
                                icon = painterResource(id = R.drawable.storage_24px),
                                color = Color(0xFF4682B4)
                            )
                        )
                    )

                    Spacer(modifier = Modifier.height(16.dp))

                    Category(
                        name = "Developer Tools",
                        actions = listOf(
                            CategoryAction(
                                name = "Launch SDK",
                                description = "Create new content",
                                icon = painterResource(id = R.drawable.sdk_24px),
                                color = Color(0xFFA67B5B)
                            )
                        )
                    )

                    Spacer(modifier = Modifier.height(16.dp))

                    Category(
                        name = "App",
                        actions = listOf(
                            CategoryAction(
                                name = "Databases",
                                description = "Configure databases here",
                                icon = painterResource(id = R.drawable.database_24px),
                                color = Color(0xFFB2BEB5)
                            ),
                            CategoryAction(
                                name = "Plugins",
                                description = "Add new functionality",
                                icon = painterResource(id = R.drawable.extension_24px),
                                color = Color(0xFF96F97B)
                            ),
                            CategoryAction(
                                name = "Settings",
                                description = "Configure how noobWarrior works",
                                icon = painterResource(id = R.drawable.settings_24px),
                                color = Color(0xFF808080)
                            ),
                            CategoryAction(
                                name = "About",
                                description = "Information about this app",
                                icon = painterResource(id = R.drawable.info_24px),
                                color = Color(0xFFADDFFF)
                            )
                        )
                    )
                }
            }
        )
    }
}
