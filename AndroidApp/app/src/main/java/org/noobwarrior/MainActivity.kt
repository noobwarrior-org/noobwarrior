package org.noobwarrior

import android.content.Intent
import android.os.Bundle
import android.view.SoundEffectConstants
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
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
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.painter.Painter
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalView
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import org.noobwarrior.ui.theme.AndroidAppTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            AndroidAppTheme {
                MainUi()
            }
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
    val iconContainerColor: Color,
    val iconContentColor: Color,
    val onClick: () -> Unit = {}
)

@Composable
fun CategoryButton(
    action: CategoryAction,
    position: ItemPosition,
    modifier: Modifier = Modifier
) {
    val view = LocalView.current

    val largeCorner = 28.dp
    val smallCorner = 4.dp
    val shape = when (position) {
        ItemPosition.Top -> RoundedCornerShape(
            topStart = largeCorner, topEnd = largeCorner,
            bottomStart = smallCorner, bottomEnd = smallCorner
        )
        ItemPosition.Bottom -> RoundedCornerShape(
            topStart = smallCorner, topEnd = smallCorner,
            bottomStart = largeCorner, bottomEnd = largeCorner
        )
        ItemPosition.Single -> RoundedCornerShape(largeCorner)
        ItemPosition.Middle -> RoundedCornerShape(smallCorner)
    }

    Surface(
        onClick = {
            view.playSoundEffect(SoundEffectConstants.CLICK)
            action.onClick()
        },
        shape = shape,
        color = MaterialTheme.colorScheme.surfaceContainerHigh,
        contentColor = MaterialTheme.colorScheme.onSurface,
        modifier = modifier.fillMaxWidth()
    ) {
        ListItem(
            headlineContent = { Text(action.name, style = MaterialTheme.typography.titleMedium) },
            supportingContent = if (action.description.isNotEmpty()) {
                {
                    Text(
                        action.description,
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            } else null,
            leadingContent = {
                Box(
                    modifier = Modifier
                        .size(40.dp)
                        .clip(CircleShape)
                        .background(action.iconContainerColor),
                    contentAlignment = Alignment.Center
                ) {
                    Icon(
                        painter = action.icon,
                        contentDescription = action.name,
                        tint = action.iconContentColor,
                        modifier = Modifier.size(22.dp)
                    )
                }
            },
            colors = ListItemDefaults.colors(containerColor = Color.Transparent)
        )
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
            modifier = Modifier.padding(start = 16.dp, bottom = 8.dp),
            style = MaterialTheme.typography.titleSmall
        )
        Column(verticalArrangement = Arrangement.spacedBy(2.dp)) {
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

@OptIn(ExperimentalMaterial3Api::class)
@Preview(showBackground = true)
@Composable
fun MainUi() {
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()
    val cs = MaterialTheme.colorScheme
    val context = LocalContext.current

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        topBar = {
            LargeTopAppBar(
                title = {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Image(
                            painter = painterResource(id = R.drawable.icon),
                            modifier = Modifier
                                .size(40.dp)
                                .padding(end = 12.dp),
                            contentDescription = "logo"
                        )
                        Text("noobWarrior")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = cs.surface,
                    scrolledContainerColor = cs.surfaceContainer,
                    titleContentColor = cs.onSurface
                ),
                scrollBehavior = scrollBehavior
            )
        },
        containerColor = cs.surface
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .padding(innerPadding)
                .padding(horizontal = 16.dp)
                .verticalScroll(rememberScrollState())
        ) {
            Category(
                name = "Play",
                actions = listOf(
                    CategoryAction(
                        name = "Online",
                        description = "The main function of the app",
                        icon = painterResource(id = R.drawable.globe_24px),
                        iconContainerColor = cs.primaryContainer,
                        iconContentColor = cs.onPrimaryContainer
                    ),
                    CategoryAction(
                        name = "Start Game Server",
                        description = "Host a server with your buddies; experimental",
                        icon = painterResource(id = R.drawable.storage_24px),
                        iconContainerColor = cs.secondaryContainer,
                        iconContentColor = cs.onSecondaryContainer
                    )
                )
            )

            Spacer(modifier = Modifier.height(24.dp))

            Category(
                name = "Developer Tools",
                actions = listOf(
                    CategoryAction(
                        name = "Launch SDK",
                        description = "Create new content",
                        icon = painterResource(id = R.drawable.sdk_24px),
                        iconContainerColor = cs.tertiaryContainer,
                        iconContentColor = cs.onTertiaryContainer
                    )
                )
            )

            Spacer(modifier = Modifier.height(24.dp))

            Category(
                name = "App",
                actions = listOf(
                    CategoryAction(
                        name = "Databases",
                        description = "Configure databases here",
                        icon = painterResource(id = R.drawable.database_24px),
                        iconContainerColor = cs.secondaryContainer,
                        iconContentColor = cs.onSecondaryContainer
                    ),
                    CategoryAction(
                        name = "Plugins",
                        description = "Add new functionality",
                        icon = painterResource(id = R.drawable.extension_24px),
                        iconContainerColor = cs.tertiaryContainer,
                        iconContentColor = cs.onTertiaryContainer
                    ),
                    CategoryAction(
                        name = "Settings",
                        description = "Configure how noobWarrior works",
                        icon = painterResource(id = R.drawable.settings_24px),
                        iconContainerColor = cs.surfaceContainerHighest,
                        iconContentColor = cs.onSurfaceVariant,
                        onClick = {
                            val intent = Intent(context, SettingsActivity::class.java)
                            context.startActivity(intent)
                        }
                    ),
                    CategoryAction(
                        name = "About",
                        description = "Information about this app",
                        icon = painterResource(id = R.drawable.info_24px),
                        iconContainerColor = cs.primaryContainer,
                        iconContentColor = cs.onPrimaryContainer
                    )
                )
            )

            Spacer(modifier = Modifier.height(16.dp))
        }
    }
}
