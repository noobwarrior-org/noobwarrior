package org.noobwarrior

import android.app.Application
import android.util.Log

class NoobWarriorApp : Application() {
    companion object {
        private const val TAG = "noobwarrior"

        init {
            System.loadLibrary("noobwarrior")
        }
    }

    override fun onCreate() {
        super.onCreate()
        val ok = NoobWarrior.nativeInit(filesDir.absolutePath)
        Log.i(TAG, "Core init ${if (ok) "ok" else "FAILED"} (dataDir=${filesDir.absolutePath})")
        if (ok) EmuService.start(this)
    }
}
