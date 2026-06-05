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
// File: JniBridge.cpp
// Started by: Hattozo
// Started on: 6/4/2026
// Description: JNI surface exposing Core to the Android app. The Application owns the Core
//   singleton for the whole process lifetime; EmuService starts/stops the HTTP server against it.
//   This file is only compiled into NoobWarrior.Core on Android (see Core/CMakeLists.txt).
#include <jni.h>
#include <android/log.h>

#include <NoobWarrior/NoobWarrior.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

using namespace NoobWarrior;

namespace {
    std::mutex             gCoreMutex;
    std::unique_ptr<Core>  gCore;

    std::mutex             gServerMutex;
    std::thread            gEventLoopThread;
    std::atomic<bool>      gEventLoopRunning { false };

    std::string JStringToStd(JNIEnv* env, jstring s) {
        if (!s) return {};
        const char* c = env->GetStringUTFChars(s, nullptr);
        std::string out(c);
        env->ReleaseStringUTFChars(s, c);
        return out;
    }
}

extern "C" {
JNIEXPORT jboolean JNICALL
Java_org_noobwarrior_NoobWarrior_nativeInit(JNIEnv* env, jobject, jstring jDataDir) {
    std::lock_guard<std::mutex> lock(gCoreMutex);
    if (gCore) return JNI_TRUE;

    Init init;
    init.Portable               = true;
    init.UserDataDir            = JStringToStd(env, jDataDir);
    init.InstallDataDir         = init.UserDataDir;
    init.AutoStartServerEmulator = false; // EmuService starts the HTTP server
    init.EnableKeychain         = false;  // OsKeychainGeneric is a no-op; revisit when Keystore lands
    init.AutocreateCert         = true;
    init.LoadPlugins            = false;  // pending plugin asset packaging

    try {
        gCore = std::make_unique<Core>(std::move(init));
    } catch (const std::exception& e) {
        __android_log_print(ANDROID_LOG_ERROR, "noobwarrior", "Core init failed: %s", e.what());
        gCore.reset();
        return JNI_FALSE;
    }
    __android_log_print(ANDROID_LOG_INFO, "noobwarrior", "Core initialized at %s", init.UserDataDir.c_str());
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_org_noobwarrior_NoobWarrior_nativeShutdown(JNIEnv*, jobject) {
    {
        std::lock_guard<std::mutex> lock(gServerMutex);
        if (gEventLoopRunning.load()) {
            __android_log_print(ANDROID_LOG_WARN, "noobwarrior",
                "nativeShutdown called while server still running; stopping it first");
        }
    }
    std::lock_guard<std::mutex> lock(gCoreMutex);
    gCore.reset();
}

JNIEXPORT jstring JNICALL
Java_org_noobwarrior_NoobWarrior_nativePing(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lock(gCoreMutex);
    const char* msg = gCore ? "noobwarrior: Core alive" : "noobwarrior: Core not initialized";
    return env->NewStringUTF(msg);
}

JNIEXPORT jboolean JNICALL
Java_org_noobwarrior_NoobWarrior_nativeStartServer(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> serverLock(gServerMutex);
    if (gEventLoopRunning.load()) return JNI_TRUE;

    Core* core;
    {
        std::lock_guard<std::mutex> coreLock(gCoreMutex);
        if (!gCore) {
            __android_log_print(ANDROID_LOG_ERROR, "noobwarrior", "StartServer: Core not initialized");
            return JNI_FALSE;
        }
        core = gCore.get();
    }

    // StartServerEmulator returns the result of StartSecure, which returns 1 on success.
    // Negative values are error codes; 0 means already running; 1 means started OK.
    int startResult = core->StartServerEmulator();
    if (startResult < 0) {
        __android_log_print(ANDROID_LOG_ERROR, "noobwarrior", "StartServerEmulator failed (code %d)", startResult);
        return JNI_FALSE;
    }

    gEventLoopRunning.store(true);
    gEventLoopThread = std::thread([core] {
        __android_log_print(ANDROID_LOG_INFO, "noobwarrior", "Event loop thread started");
        while (gEventLoopRunning.load()) {
            core->ProcessEvents(true);
        }
        __android_log_print(ANDROID_LOG_INFO, "noobwarrior", "Event loop thread exiting");
    });
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_org_noobwarrior_NoobWarrior_nativeStopServer(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> serverLock(gServerMutex);
    if (!gEventLoopRunning.load()) return;

    {
        std::lock_guard<std::mutex> coreLock(gCoreMutex);
        if (gCore) gCore->StopServerEmulator();
    }
    gEventLoopRunning.store(false);
    // Kick ProcessEvents out of its blocking wait so the loop notices the flag flip.
    {
        std::lock_guard<std::mutex> coreLock(gCoreMutex);
        if (gCore) gCore->RunOnEventLoop([] {});
    }
    if (gEventLoopThread.joinable()) gEventLoopThread.join();
}

JNIEXPORT jboolean JNICALL
Java_org_noobwarrior_NoobWarrior_nativeIsServerRunning(JNIEnv*, jobject) {
    return gEventLoopRunning.load() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_org_noobwarrior_NoobWarrior_nativeHttpPort(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> coreLock(gCoreMutex);
    if (!gCore) return 0;
    return gCore->GetRegistry()->GetKeyValue<int>("emu.http_port").value_or(8080);
}
} // extern "C"
