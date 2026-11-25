package com.unool.android.server

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.Checkbox
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import androidx.core.content.ContextCompat
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
            ServerTheme {
                ServerScreen(this@ServerActivity)
            }
        }
    }
}

private data class ConfigDraft(
    var mode: String = "normal",
    var maxClients: String = "32",
    var banCount: String = "1",
    var normalCandidateCount: String = "5",
    var normalInitHandCount: String = "8",
    var doubleCandidateCount: String = "5",
    var doubleInitHandCount: String = "10",
    var assign: List<String> = listOf("", ""),
    var shieldedCharacters: String = "",
    var enabledGroups: List<String> = ServerConfig.allGroups,
    var extraShieldedGroups: List<String> = emptyList()
) {
    companion object {
        fun from(config: ServerConfig): ConfigDraft {
            val expected = if (config.mode == "double") 4 else 2
            val slots = buildList {
                addAll(config.assign)
                while (size < expected) add("")
            }.take(expected)
            return ConfigDraft(
                mode = config.mode,
                maxClients = config.maxClients.toString(),
                banCount = config.banCount.toString(),
                normalCandidateCount = config.normalCandidateCount.toString(),
                normalInitHandCount = config.normalInitHandCount.toString(),
                doubleCandidateCount = config.doubleCandidateCount.toString(),
                doubleInitHandCount = config.doubleInitHandCount.toString(),
                assign = slots,
                shieldedCharacters = config.shieldedCharacters.joinToString(","),
                enabledGroups = ServerConfig.allGroups.filterNot { it in config.shieldedGroups },
                extraShieldedGroups = config.shieldedGroups.filter { it !in ServerConfig.allGroups }
            )
        }
    }

    fun withMode(value: String): ConfigDraft {
        val expected = if (value == "double") 4 else 2
        val slots = buildList {
            addAll(assign)
            while (size < expected) add("")
        }.take(expected)
        return copy(mode = value, assign = slots)
    }

    fun toConfig(): ServerConfig? {
        fun parse(value: String, min: Int, max: Int): Int? =
            value.trim().toIntOrNull()?.takeIf { it in min..max }

        fun parseList(value: String): List<String> =
            value.split(',').map { it.trim() }.filter { it.isNotEmpty() }

        val modeValue = mode.trim().lowercase()
        if (modeValue != "normal" && modeValue != "double") return null

        val maxClients = parse(maxClients, 2, 256) ?: return null
        val banCount = parse(banCount, 0, 10) ?: return null
        val normalCandidates = parse(normalCandidateCount, 1, 123) ?: return null
        val normalHand = parse(normalInitHandCount, 1, 30) ?: return null
        val doubleCandidates = parse(doubleCandidateCount, 1, 123) ?: return null
        val doubleHand = parse(doubleInitHandCount, 1, 30) ?: return null

        val assignList = assign.map { it.trim() }.filter { it.isNotEmpty() }
        val expectedAssignCount = if (modeValue == "normal") 2 else 4
        if (assignList.isNotEmpty() && assignList.size != expectedAssignCount) return null

        val shieldedGroups = ServerConfig.allGroups.filterNot { it in enabledGroups } + extraShieldedGroups
        return ServerConfig(
            mode = modeValue,
            maxClients = maxClients,
            banCount = banCount,
            normalCandidateCount = normalCandidates,
            normalInitHandCount = normalHand,
            doubleCandidateCount = doubleCandidates,
            doubleInitHandCount = doubleHand,
            assign = assignList,
            shieldedCharacters = parseList(shieldedCharacters),
            shieldedGroups = shieldedGroups
        )
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

    var showConfigDialog by rememberSaveable { mutableStateOf(false) }
    var showFrpcDialog by rememberSaveable { mutableStateOf(false) }
    var showUserDataDialog by rememberSaveable { mutableStateOf(false) }
    var userDataText by remember { mutableStateOf("") }
    var modeExpanded by rememberSaveable { mutableStateOf(false) }
    val logListState = rememberLazyListState()
    var configDraft by remember { mutableStateOf(ConfigDraft.from(ServerConfig.load(context))) }

    LaunchedEffect(logs.size) {
        if (logs.isNotEmpty()) logListState.animateScrollToItem(logs.lastIndex)
    }

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
        Button(
            onClick = {
                if (running) ServerService.stop(context) else ServerService.start(context, 8888)
            },
            colors = ButtonDefaults.buttonColors(
                containerColor = if (running) Color(0xFFB3261E) else Color(0xFF1B7F3A),
                contentColor = Color.White
            ),
            modifier = Modifier.fillMaxWidth().height(52.dp)
        ) {
            Text(
                text = if (running) "关闭服务器" else "启动服务器",
                fontSize = 16.sp,
                fontWeight = FontWeight.Bold
            )
        }
        Spacer(Modifier.height(8.dp))
        Row {
            Button(
                onClick = {
                    configDraft = ConfigDraft.from(ServerConfig.load(context))
                    showConfigDialog = true
                },
                modifier = Modifier.weight(1f)
            ) { Text("编辑服务器配置", fontSize = 12.sp) }
            Spacer(Modifier.width(8.dp))
            Button(
                onClick = { showFrpcDialog = true },
                modifier = Modifier.weight(1f)
            ) { Text("SakuraFrp 配置", fontSize = 12.sp) }
        }
        Spacer(Modifier.height(8.dp))
        Button(
            onClick = {
                userDataText = UserDataFile.loadText(context)
                showUserDataDialog = true
            },
            modifier = Modifier.fillMaxWidth()
        ) { Text("编辑用户数据", fontSize = 13.sp) }

        Spacer(Modifier.height(12.dp))
        Text("运行日志（最近 200 行，最新在底部）", fontSize = 14.sp, fontWeight = FontWeight.Bold)
        LazyColumn(
            state = logListState,
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
                    lineHeight = 14.sp,
                    fontFamily = FontFamily.Monospace,
                    color = if (line.error) Color(0xFFB3261E) else MaterialTheme.colorScheme.onSurface
                )
                Spacer(Modifier.height(4.dp))
            }
        }

        if (showFrpcDialog) {
            Dialog(onDismissRequest = { showFrpcDialog = false }) {
                Card(modifier = Modifier.fillMaxWidth().padding(12.dp)) {
                    Column(
                        modifier = Modifier
                            .padding(16.dp)
                            .verticalScroll(rememberScrollState())
                    ) {
                        Text("SakuraFrp 配置", fontSize = 18.sp, fontWeight = FontWeight.Bold)
                        Spacer(Modifier.height(4.dp))
                        Text("隧道本地端口请填 8888。", fontSize = 12.sp, color = Color.Gray)
                        Spacer(Modifier.height(10.dp))
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
                        Spacer(Modifier.height(12.dp))
                        Row {
                            Button(
                                onClick = {
                                    FrpcConfig.save(context, FrpcConfig(token, tunnelIds, autoStart))
                                    ServerState.appendLog("SakuraFrp 配置已保存")
                                    showFrpcDialog = false
                                }
                            ) { Text("保存") }
                            Spacer(Modifier.padding(horizontal = 8.dp))
                            Button(onClick = { showFrpcDialog = false }) { Text("取消") }
                        }
                    }
                }
            }
        }

        if (showUserDataDialog) {
            Dialog(onDismissRequest = { showUserDataDialog = false }) {
                Card(modifier = Modifier.fillMaxWidth().padding(12.dp)) {
                    Column(
                        modifier = Modifier
                            .padding(16.dp)
                            .verticalScroll(rememberScrollState())
                    ) {
                        Text("编辑 userDatas.json", fontSize = 18.sp, fontWeight = FontWeight.Bold)
                        Spacer(Modifier.height(4.dp))
                        Text(
                            text = if (running) "服务器运行中不能保存；请先关闭服务器。" else "保存前自动备份为 userDatas.json.bak，保存后重启服务器生效。",
                            fontSize = 12.sp,
                            color = if (running) Color(0xFFB3261E) else Color.Gray
                        )
                        Spacer(Modifier.height(10.dp))
                        OutlinedTextField(
                            value = userDataText,
                            onValueChange = { userDataText = it },
                            label = { Text("账号存档 JSON") },
                            maxLines = 14,
                            textStyle = TextStyle(fontFamily = FontFamily.Monospace, fontSize = 11.sp),
                            modifier = Modifier.fillMaxWidth().height(280.dp)
                        )
                        Spacer(Modifier.height(12.dp))
                        Row {
                            Button(
                                enabled = !running,
                                onClick = {
                                    val error = UserDataFile.saveText(context, userDataText)
                                    if (error == null) {
                                        ServerState.appendLog("userDatas.json 已保存，重启服务器生效")
                                        showUserDataDialog = false
                                    }
                                    else {
                                        ServerState.appendLog(error, true)
                                    }
                                }
                            ) { Text("保存") }
                            Spacer(Modifier.padding(horizontal = 6.dp))
                            Button(onClick = { userDataText = UserDataFile.loadText(context) }) {
                                Text("重新载入", fontSize = 12.sp)
                            }
                            Spacer(Modifier.padding(horizontal = 6.dp))
                            Button(onClick = { showUserDataDialog = false }) {
                                Text("取消", fontSize = 12.sp)
                            }
                        }
                    }
                }
            }
        }

        if (showConfigDialog) {
            Dialog(onDismissRequest = { showConfigDialog = false }) {
                Card(modifier = Modifier.fillMaxWidth().padding(12.dp)) {
                    Column(
                        modifier = Modifier
                            .padding(16.dp)
                            .verticalScroll(rememberScrollState())
                    ) {
                        Text("编辑服务器配置", fontSize = 18.sp, fontWeight = FontWeight.Bold)
                        Spacer(Modifier.height(4.dp))
                        Text("保存后写入本地 server_config.json，重启服务端生效。", fontSize = 12.sp, color = Color.Gray)
                        Spacer(Modifier.height(10.dp))

                        Text("模式", fontSize = 13.sp)
                        Box(modifier = Modifier.fillMaxWidth()) {
                            OutlinedButton(
                                onClick = { modeExpanded = true },
                                modifier = Modifier.fillMaxWidth()
                            ) {
                                Text(
                                    text = if (configDraft.mode == "double") "双将模式" else "普通模式",
                                    modifier = Modifier.weight(1f),
                                    textAlign = TextAlign.Start
                                )
                                Text("▾")
                            }
                            DropdownMenu(
                                expanded = modeExpanded,
                                onDismissRequest = { modeExpanded = false },
                                modifier = Modifier.fillMaxWidth()
                            ) {
                                DropdownMenuItem(
                                    text = { Text("普通模式") },
                                    onClick = {
                                        configDraft = configDraft.withMode("normal")
                                        modeExpanded = false
                                    }
                                )
                                DropdownMenuItem(
                                    text = { Text("双将模式") },
                                    onClick = {
                                        configDraft = configDraft.withMode("double")
                                        modeExpanded = false
                                    }
                                )
                            }
                        }
                        Spacer(Modifier.height(6.dp))

                        val setAssignAt = { index: Int, value: String ->
                            configDraft = configDraft.copy(
                                assign = configDraft.assign.toMutableList().also { it[index] = value }
                            )
                        }
                        configDraft.assign.forEachIndexed { index, value ->
                            OutlinedTextField(
                                value = value,
                                onValueChange = { setAssignAt(index, it) },
                                label = { Text("指定角色 ${index + 1}") },
                                singleLine = true,
                                modifier = Modifier.fillMaxWidth()
                            )
                        }
                        Text(
                            text = "全部留空 = BP 选角；填写时必须把 ${configDraft.assign.size} 个角色全部填满",
                            fontSize = 11.sp,
                            color = Color.Gray
                        )
                        OutlinedTextField(
                            value = configDraft.shieldedCharacters,
                            onValueChange = { configDraft = configDraft.copy(shieldedCharacters = it) },
                            label = { Text("屏蔽角色（逗号分隔角色名）") },
                            singleLine = true,
                            modifier = Modifier.fillMaxWidth()
                        )

                        Spacer(Modifier.height(6.dp))
                        Text("分组启用状态（勾选=启用，不勾选=屏蔽）", fontSize = 13.sp, fontWeight = FontWeight.Bold)
                        ServerConfig.allGroups.forEach { group ->
                            Row(
                                modifier = Modifier.fillMaxWidth().padding(vertical = 2.dp),
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                Checkbox(
                                    checked = group in configDraft.enabledGroups,
                                    onCheckedChange = { checked ->
                                        configDraft = configDraft.copy(
                                            enabledGroups = if (checked) {
                                                (configDraft.enabledGroups + group).distinct()
                                            }
                                            else {
                                                configDraft.enabledGroups - group
                                            }
                                        )
                                    }
                                )
                                Text(group, fontSize = 13.sp)
                            }
                        }
                        if (configDraft.extraShieldedGroups.isNotEmpty()) {
                            Text(
                                text = "其它保留屏蔽分组：${configDraft.extraShieldedGroups.joinToString("、")}",
                                fontSize = 11.sp,
                                color = Color.Gray
                            )
                        }

                        Spacer(Modifier.height(10.dp))

                        OutlinedTextField(
                            value = configDraft.maxClients,
                            onValueChange = { configDraft = configDraft.copy(maxClients = it) },
                            label = { Text("连接槽位上限 maxClients（2-256）") },
                            singleLine = true,
                            modifier = Modifier.fillMaxWidth()
                        )
                        OutlinedTextField(
                            value = configDraft.banCount,
                            onValueChange = { configDraft = configDraft.copy(banCount = it) },
                            label = { Text("禁牌数 banCount（0-10）") },
                            singleLine = true,
                            modifier = Modifier.fillMaxWidth()
                        )
                        OutlinedTextField(
                            value = configDraft.normalCandidateCount,
                            onValueChange = { configDraft = configDraft.copy(normalCandidateCount = it) },
                            label = { Text("普通模式候选角色数（1-123）") },
                            singleLine = true,
                            modifier = Modifier.fillMaxWidth()
                        )
                        OutlinedTextField(
                            value = configDraft.normalInitHandCount,
                            onValueChange = { configDraft = configDraft.copy(normalInitHandCount = it) },
                            label = { Text("普通模式初始手牌数（1-30）") },
                            singleLine = true,
                            modifier = Modifier.fillMaxWidth()
                        )
                        OutlinedTextField(
                            value = configDraft.doubleCandidateCount,
                            onValueChange = { configDraft = configDraft.copy(doubleCandidateCount = it) },
                            label = { Text("双将模式候选角色数（1-123）") },
                            singleLine = true,
                            modifier = Modifier.fillMaxWidth()
                        )
                        OutlinedTextField(
                            value = configDraft.doubleInitHandCount,
                            onValueChange = { configDraft = configDraft.copy(doubleInitHandCount = it) },
                            label = { Text("双将模式初始手牌数（1-30）") },
                            singleLine = true,
                            modifier = Modifier.fillMaxWidth()
                        )

                        Spacer(Modifier.height(12.dp))
                        Row {
                            Button(
                                onClick = {
                                    val parsed = configDraft.toConfig()
                                    if (parsed == null) {
                                        ServerState.appendLog("服务器配置保存失败：请检查数值范围或角色数量", true)
                                    }
                                    else if (ServerConfig.save(context, parsed)) {
                                        configDraft = ConfigDraft.from(parsed)
                                        showConfigDialog = false
                                        ServerState.appendLog("服务器配置已保存，重启服务端生效")
                                    }
                                    else {
                                        ServerState.appendLog("服务器配置保存失败：无法写入本地文件", true)
                                    }
                                }
                            ) { Text("保存") }
                            Spacer(Modifier.padding(horizontal = 8.dp))
                            Button(onClick = { showConfigDialog = false }) { Text("取消") }
                        }
                    }
                }
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
