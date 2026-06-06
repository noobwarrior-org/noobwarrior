package org.noobwarrior

import android.content.Context
import android.util.Log
import java.io.File

/**
 * Extracts the bundled AppDistribution assets (plugins/ and priv-plugins/) from the APK into
 * filesDir so Core's PluginManager can find them at the expected paths.
 *
 * Extraction is skipped when the app version hasn't changed (stamped in filesDir/.appversion).
 */
object AppDistribution {
    private const val TAG = "noobwarrior"
    private val DIRS = listOf("plugins", "priv-plugins")

    fun extractIfNeeded(ctx: Context) {
        val stamp = File(ctx.filesDir, ".appversion")
        val currentVersion = ctx.packageManager
            .getPackageInfo(ctx.packageName, 0).versionCode.toString()

        if (stamp.exists() && stamp.readText().trim() == currentVersion) return

        Log.i(TAG, "Extracting AppDistribution assets (version $currentVersion)...")
        for (dir in DIRS) {
            extractDir(ctx, dir, ctx.filesDir)
        }
        stamp.writeText(currentVersion)
        Log.i(TAG, "AppDistribution extraction complete.")
    }

    private fun extractDir(ctx: Context, assetPath: String, destRoot: File) {
        val entries = ctx.assets.list(assetPath) ?: return
        if (entries.isEmpty()) {
            val dest = File(destRoot, assetPath)
            dest.parentFile?.mkdirs()
            ctx.assets.open(assetPath).use { src ->
                dest.outputStream().use { dst -> src.copyTo(dst) }
            }
            return
        }
        for (entry in entries) {
            extractDir(ctx, "$assetPath/$entry", destRoot)
        }
    }
}
