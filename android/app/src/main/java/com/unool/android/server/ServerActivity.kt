package com.unool.android.server

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.ContextCompat
import com.unool.android.ui.UnoolTheme
import java.net.NetworkInterface

class ServerActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
            ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS)
            != PackageManager.PERMISSION_GRANTED
        ) {
            requestPermissions(arrayOf(Manifest.permission.POST_NOTIFICATIONS), 1001)
        }
        setContent {
            UnoolTheme {
                ServerScreen(this@ServerActivity)
            }
        }
    }
}

@Composable
private fun ServerScreen(context: android.content.Context) {
    val running by ServerState.running.collectAsState()
    val tunnelRunning by FrpcController.running.collectAsState()
    val logs by ServerState.logs.collectAsState()
    val lastError by ServerState.lastError.collectAsState()
    val addresses = rememberLocalAddresses()

    val savedConfig = remember { FrpcConfig.load(context) }
    var token by rememberSaveable { mutableStateOf(savedConfig.token) }
    var tunnelIds by rememberSaveable { mutableStateOf(savedConfig.tunnelIds) }
    var autoStart by rememberSaveable { mutableStateOf(savedConfig.autoStart) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .statusBarsPadding()
            .background(MaterialTheme.colorScheme.background)
            .padding(16.dp)
    ) {
        Text(
            text = "UNOOL 局域网服务端",
            fontSize = 22.sp,
            fontWeight = FontWeight.Bold,
            color = MaterialTheme.colorScheme.primary
        )
        Spacer(Modifier.height(8.dp))
        Text("监听端口：8888（与 PC 客户端协议版本 3 一致）", fontSize = 14.sp)
        Text("本机地址：${addresses.ifEmpty { "未检测到局域网地址" }}", fontSize = 14.sp)
        Text(
            text = if (running) "状态：运行中（切后台后仍继续监听）" else "状态：已停止",
            fontSize = 14.sp,
            color = if (running) Color(0xFF1B7F3A) else Color(0xFFB3261E)
        )
        Text(
            text = "SakuraFrp：${if (tunnelRunning) "隧道已启动" else "未启动"}（自动启动：${if (autoStart) "开" else "关"}）",
            fontSize = 14.sp,
            color = if (tunnelRunning) Color(0xFF1B7F3A) else Color(0xFFB26A00)
        )
        if (lastError != null) {
            Text("最近错误：${lastError}", fontSize = 13.sp, color = Color(0xFFB3261E))
        }

        Spacer(Modifier.height(12.dp))
        Row {
            Button(
                onClick = { ServerService.start(context, 8888) },
                enabled = !running
            ) { Text("启动服务端") }
            Spacer(Modifier.padding(horizontal = 8.dp))
            Button(
                onClick = { ServerService.stop(context) },
                enabled = running
            ) { Text("停止服务端") }
        }

        Spacer(Modifier.height(12.dp))
        Text(
            text = "SakuraFrp 内网穿透（隧道本地端口请填 8888）",
            fontSize = 14.sp,
            fontWeight = FontWeight.Bold
        )
        OutlinedTextField(
            value = token,
            onValueChange = { token = it },
            label = { Text("访问密钥（不会写入日志）") },
            singleLine = true,
            visualTransformation = PasswordVisualTransformation(),
            modifier = Modifier.fillMaxWidth()
        )
        OutlinedTextField(
            value = tunnelIds,
            onValueChange = { tunnelIds = it },
            label = { Text("隧道 ID（多个同节点隧道用英文逗号分隔）") },
            singleLine = true,
            modifier = Modifier.fillMaxWidth()
        )
        Row(verticalAlignment = Alignment.CenterVertically) {
            Switch(checked = autoStart, onCheckedChange = { autoStart = it })
            Spacer(Modifier.padding(horizontal = 6.dp))
            Text("开服时自动启动隧道", fontSize = 13.sp)
        }
        Button(
            onClick = {
                FrpcConfig.save(context, FrpcConfig(token, tunnelIds, autoStart))
                ServerState.appendLog("SakuraFrp 配置已保存")
            }
        ) { Text("保存穿透配置") }

        Spacer(Modifier.height(12.dp))
        Text("运行日志（最近 200 行）", fontSize = 14.sp, fontWeight = FontWeight.Bold)
        LazyColumn(
            modifier = Modifier
                .fillMaxWidth()
                .weight(1f)
                .background(MaterialTheme.colorScheme.surface, RoundedCornerShape(8.dp))
                .padding(8.dp)
        ) {
            items(logs) { line ->
                Text(
                    text = line.text,
                    fontSize = 11.sp,
                    fontFamily = FontFamily.Monospace,
                    color = if (line.error) Color(0xFFB3261E) else MaterialTheme.colorScheme.onSurface
                )
            }
        }
    }
}

@Composable
private fun rememberLocalAddresses(): String {
    val addresses = StringBuilder()
    try {
        for (iface in NetworkInterface.getNetworkInterfaces()) {
            if (iface.isLoopback || !iface.isUp) continue
            for (addr in iface.inetAddresses) {
                if (!addr.isLoopbackAddress && addr is java.net.Inet4Address) {
                    if (addresses.isNotEmpty()) addresses.append(" / ")
                    addresses.append(addr.hostAddress)
                }
            }
        }
    } catch (_: Exception) {
        // 无局域网时保持空串，UI 会显示提示
    }
    return addresses.toString()
}
