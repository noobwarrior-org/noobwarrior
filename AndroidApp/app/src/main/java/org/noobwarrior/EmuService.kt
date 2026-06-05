package org.noobwarrior

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.core.app.ServiceCompat

class EmuService : Service() {
    companion object {
        private const val TAG = "noobwarrior"
        private const val CHANNEL_ID = "noobwarrior.emu"
        private const val NOTIFICATION_ID = 1
        private const val ACTION_STOP = "org.noobwarrior.EmuService.STOP"

        fun start(ctx: Context) {
            val intent = Intent(ctx, EmuService::class.java)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                ctx.startForegroundService(intent)
            } else {
                ctx.startService(intent)
            }
        }

        fun stop(ctx: Context) {
            ctx.stopService(Intent(ctx, EmuService::class.java))
        }
    }

    override fun onCreate() {
        super.onCreate()
        ensureNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_STOP) {
            stopSelf()
            return START_NOT_STICKY
        }

        ServiceCompat.startForeground(
            this,
            NOTIFICATION_ID,
            buildNotification(port = 0),
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE)
                ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE
            else 0
        )

        val ok = NoobWarrior.nativeStartServer()
        if (!ok) {
            Log.e(TAG, "nativeStartServer failed; stopping service")
            stopSelf()
            return START_NOT_STICKY
        }

        // Refresh the notification with the actual port once the server is up.
        val port = NoobWarrior.nativeHttpPort()
        notificationManager().notify(NOTIFICATION_ID, buildNotification(port))

        return START_STICKY
    }

    override fun onDestroy() {
        NoobWarrior.nativeStopServer()
        super.onDestroy()
    }

    override fun onBind(intent: Intent): IBinder? = null

    private fun ensureNotificationChannel() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return
        val nm = notificationManager()
        if (nm.getNotificationChannel(CHANNEL_ID) != null) return
        nm.createNotificationChannel(
            NotificationChannel(
                CHANNEL_ID,
                "Emulator",
                NotificationManager.IMPORTANCE_LOW
            ).apply {
                description = "Shown while the noobWarrior HTTP server is running."
            }
        )
    }

    private fun buildNotification(port: Int): android.app.Notification {
        val stopIntent = PendingIntent.getService(
            this, 0,
            Intent(this, EmuService::class.java).setAction(ACTION_STOP),
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )
        val text = if (port > 0) "Server emulator listening on localhost:$port" else "Starting server emulator..."
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(R.drawable.globe_24px)
            .setContentTitle("noobWarrior")
            .setContentText(text)
            .setOngoing(true)
            .addAction(0, "Stop", stopIntent)
            .build()
    }

    private fun notificationManager(): NotificationManager =
        getSystemService(NOTIFICATION_SERVICE) as NotificationManager
}
