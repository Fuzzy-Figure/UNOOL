package com.unool.android.server

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.IBinder
import android.os.PowerManager
import androidx.core.app.NotificationCompat

/** 前台服务：让原生服务端在切后台/锁屏后继续监听 8888。 */
class ServerService : Service() {

    companion object {
        private const val CHANNEL_ID = "unool_server"
        private const val ACTION_START = "com.unool.android.server.START"
        private const val ACTION_STOP = "com.unool.android.server.STOP"
        private const val EXTRA_PORT = "port"

        fun start(context: Context, port: Int) {
            val intent = Intent(context, ServerService::class.java).apply {
                action = ACTION_START
                putExtra(EXTRA_PORT, port)
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                context.startForegroundService(intent)
            } else {
                context.startService(intent)
            }
        }

        fun stop(context: Context) {
            val intent = Intent(context, ServerService::class.java).apply { action = ACTION_STOP }
            context.startService(intent)
        }
    }

    private var worker: Thread? = null
    private var wakeLock: PowerManager.WakeLock? = null

    override fun onCreate() {
        super.onCreate()
        createChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_STOP) {
            stopNative()
            stopSelf()
            return START_NOT_STICKY
        }

        startForeground(1, buildNotification())
        acquireWakeLock()

        if (worker == null) {
            val port = intent?.getIntExtra(EXTRA_PORT, 8888) ?: 8888
            worker = Thread({
                ServerState.setRunning(true)
                val ok = ServerBridge.start(applicationContext, port)
                if (!ok) {
                    ServerState.appendLog("服务端启动失败：端口 $port 可能被占用", true)
                    FrpcController.stop()
                }
                ServerState.setRunning(false)
                stopSelf()
            }, "unool-native-server").also { it.start() }
        }

        val frpcConfig = FrpcConfig.load(this)
        if (frpcConfig.autoStart) {
            if (!FrpcController.isAvailable(this)) {
                ServerState.appendLog("[SakuraFrp] 当前 APK 未包含 frpc 可执行文件", true)
            }
            else {
                FrpcController.start(this, frpcConfig.token, frpcConfig.tunnelIds)
            }
        }
        else {
            ServerState.appendLog("[SakuraFrp] 未开启自动启动，当前仅局域网可连接")
        }
        return START_NOT_STICKY
    }

    private fun stopNative() {
        FrpcController.stop()
        ServerBridge.nativeStop()
        worker?.join(2000)
        worker = null
        wakeLock?.let { if (it.isHeld) it.release() }
        wakeLock = null
        ServerState.setRunning(false)
    }

    private fun acquireWakeLock() {
        if (wakeLock != null) return
        val pm = getSystemService(Context.POWER_SERVICE) as PowerManager
        wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "unool:server").apply {
            setReferenceCounted(false)
            acquire()
        }
    }

    private fun createChannel() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return
        val manager = getSystemService(NotificationManager::class.java) ?: return
        if (manager.getNotificationChannel(CHANNEL_ID) == null) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                "UNOOL 服务端",
                NotificationManager.IMPORTANCE_LOW
            )
            channel.description = "局域网游戏服务端运行状态"
            manager.createNotificationChannel(channel)
        }
    }

    private fun buildNotification(): android.app.Notification {
        val flags = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        } else {
            PendingIntent.FLAG_UPDATE_CURRENT
        }
        val contentIntent = PendingIntent.getActivity(
            this,
            0,
            Intent(this, ServerActivity::class.java),
            flags
        )
        val stopIntent = PendingIntent.getService(
            this,
            1,
            Intent(this, ServerService::class.java).apply { action = ACTION_STOP },
            flags
        )
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.ic_dialog_info)
            .setContentTitle("UNOOL 服务端运行中")
            .setContentText("局域网客户端可连接本机 8888 端口")
            .setContentIntent(contentIntent)
            .addAction(android.R.drawable.ic_media_pause, "停止", stopIntent)
            .setOngoing(true)
            .build()
    }

    override fun onDestroy() {
        stopNative()
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null
}
