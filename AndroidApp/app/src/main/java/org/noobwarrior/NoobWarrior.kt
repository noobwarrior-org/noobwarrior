package org.noobwarrior

object NoobWarrior {
    external fun nativeInit(dataDir: String): Boolean
    external fun nativeShutdown()
    external fun nativePing(): String

    external fun nativeStartServer(): Boolean
    external fun nativeStopServer()
    external fun nativeIsServerRunning(): Boolean
    external fun nativeHttpPort(): Int
}
