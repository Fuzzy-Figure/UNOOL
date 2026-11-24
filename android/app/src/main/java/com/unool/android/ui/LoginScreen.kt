package com.unool.android.ui

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import com.unool.android.protocol.RoomSummary
import com.unool.android.viewmodel.AppViewModel
import com.unool.android.viewmodel.UiState
import kotlinx.coroutines.delay

@Composable
fun LoginScreen(ui: UiState, viewModel: AppViewModel) {
    Column(
        modifier = Modifier.fillMaxSize().padding(horizontal = 32.dp, vertical = 8.dp).verticalScroll(rememberScrollState()),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Top
    ) {
        Text("UNOOL", fontSize = 34.sp, fontWeight = FontWeight.Bold, color = MaterialTheme.colorScheme.primary)
        Text("安卓客户端 · 连接 PC 服务端对战", fontSize = 11.sp, color = Color.Gray)
        Spacer(Modifier.height(12.dp))

        OutlinedTextField(
            value = ui.username,
            onValueChange = viewModel::setUsername,
            label = { Text("用户名") },
            singleLine = true,
            modifier = Modifier.fillMaxWidth(0.6f)
        )
        Spacer(Modifier.height(10.dp))
        OutlinedTextField(
            value = ui.password,
            onValueChange = viewModel::setPassword,
            label = { Text("密码") },
            singleLine = true,
            modifier = Modifier.fillMaxWidth(0.6f)
        )
        Spacer(Modifier.height(6.dp))
        Text(
            text = ui.message,
            color = if (ui.message.contains("失败") || ui.message.contains("存在")) Color(0xFFB3261E)
            else MaterialTheme.colorScheme.primary,
            fontSize = 14.sp,
            modifier = Modifier.heightIn(min = 24.dp)
        )
        Spacer(Modifier.height(8.dp))

        Row {
            Button(onClick = viewModel::submitLogin, modifier = Modifier.width(120.dp)) {
                Text("登录")
            }
            Spacer(Modifier.width(16.dp))
            Button(onClick = viewModel::register, modifier = Modifier.width(120.dp)) {
                Text("注册")
            }
        }

        Spacer(Modifier.height(10.dp))
        Text("启动后自动连接服务器，直接登录或注册", fontSize = 12.sp, color = Color.Gray)
    }
}

// ---------------------------------------------------------------- 主大厅：开始游戏 / 商城 / 签到

@Composable
fun LobbyScreen(ui: UiState, viewModel: AppViewModel) {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Card(modifier = Modifier.fillMaxWidth()) {
            Column(modifier = Modifier.padding(14.dp)) {
                Text(
                    text = "账号：${ui.username}    积分：${ui.points}    胜：${ui.wins}    负：${ui.losses}",
                    fontSize = 15.sp,
                    fontWeight = FontWeight.Medium
                )
            }
        }

        Spacer(Modifier.height(12.dp))
        if (ui.message.isNotBlank()) {
            Text(
                text = ui.message,
                fontSize = 13.sp,
                color = Color(0xFFB26A00),
                modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)
            )
        }

        Spacer(Modifier.weight(1f))
        Button(
            onClick = viewModel::startGame,
            modifier = Modifier.fillMaxWidth().height(56.dp)
        ) {
            Text("开始游戏", fontSize = 18.sp, fontWeight = FontWeight.Bold)
        }
        Spacer(Modifier.height(10.dp))
        Row {
            Button(onClick = viewModel::checkin, modifier = Modifier.weight(1f)) { Text("每日签到") }
            Spacer(Modifier.width(10.dp))
            Button(onClick = viewModel::openShop, modifier = Modifier.weight(1f)) { Text("商城") }
        }
        Spacer(Modifier.height(6.dp))
        Text("点击「开始游戏」进入房间界面：创建房间或加入别人的房间", fontSize = 12.sp, color = Color.Gray)
        Spacer(Modifier.weight(0.4f))
    }

    ui.checkinPopup?.let { message ->
        Dialog(onDismissRequest = viewModel::dismissCheckin) {
            Card {
                Column(modifier = Modifier.padding(24.dp)) {
                    Text("签到结果", fontSize = 18.sp, fontWeight = FontWeight.Bold)
                    Spacer(Modifier.height(12.dp))
                    Text(message, fontSize = 15.sp)
                    Spacer(Modifier.height(16.dp))
                    Button(onClick = viewModel::dismissCheckin, modifier = Modifier.align(Alignment.End)) {
                        Text("关闭")
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------- 房间界面：创建 / 列表 / 加入

@Composable
fun RoomsScreen(ui: UiState, viewModel: AppViewModel) {
    // 房间列表自动刷新：不点刷新也能看到别人新开的房间
    LaunchedEffect(Unit) {
        while (true) {
            delay(2000)
            viewModel.refreshRooms()
        }
    }

    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("房间界面", fontSize = 18.sp, fontWeight = FontWeight.Bold)
            Spacer(Modifier.weight(1f))
            Button(onClick = viewModel::backToLobby) { Text("返回大厅") }
        }

        Spacer(Modifier.height(8.dp))
        // 创建房间：房间名可留空（以「账号的房间」命名）
        Row(verticalAlignment = Alignment.CenterVertically) {
            OutlinedTextField(
                value = ui.roomNameInput,
                onValueChange = viewModel::setRoomName,
                label = { Text("房间名（可留空）") },
                singleLine = true,
                modifier = Modifier.weight(1f)
            )
            Spacer(Modifier.width(8.dp))
            Button(onClick = viewModel::createRoom) { Text("创建房间") }
        }

        Spacer(Modifier.height(8.dp))
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("房间列表（${ui.rooms.size} 个）", fontSize = 14.sp, fontWeight = FontWeight.Medium)
            Spacer(Modifier.weight(1f))
            Button(onClick = viewModel::refreshRooms) { Text("刷新") }
        }

        Column(
            modifier = Modifier
                .weight(1f)
                .fillMaxWidth()
                .verticalScroll(rememberScrollState())
        ) {
            if (ui.rooms.isEmpty()) {
                Text("当前没有房间，先创建一个吧", fontSize = 13.sp, color = Color.Gray,
                    modifier = Modifier.padding(vertical = 12.dp))
            }
            ui.rooms.forEachIndexed { index, room ->
                RoomRow(room = room, onClick = { viewModel.joinRoom(index) })
                Spacer(Modifier.height(6.dp))
            }
        }

        if (ui.message.isNotBlank()) {
            Text(
                text = ui.message,
                fontSize = 13.sp,
                color = Color(0xFFB26A00),
                modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)
            )
        }
        Text("点击房间即可加入；双方都准备后自动开局", fontSize = 12.sp, color = Color.Gray)
    }
}

@Composable
private fun RoomRow(room: RoomSummary, onClick: () -> Unit) {
    val full = room.members >= room.capacity || room.inGame
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .clickable(enabled = !full, onClick = onClick)
    ) {
        Row(
            modifier = Modifier.padding(horizontal = 14.dp, vertical = 10.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column(modifier = Modifier.weight(1f)) {
                Text(
                    text = room.name,
                    fontSize = 15.sp,
                    fontWeight = FontWeight.Medium,
                    color = if (room.inGame) Color.Gray else MaterialTheme.colorScheme.onSurface
                )
                Text("房主：${room.owner}", fontSize = 12.sp, color = Color.Gray)
            }
            Text(
                text = "${room.members}/${room.capacity} 人",
                fontSize = 12.sp,
                color = Color.Gray
            )
            Spacer(Modifier.width(10.dp))
            Text(
                text = when {
                    room.inGame -> "对局中"
                    room.members >= room.capacity -> "已满"
                    else -> "可加入"
                },
                fontSize = 12.sp,
                color = when {
                    room.inGame -> Color(0xFFB26A00)
                    room.members >= room.capacity -> Color(0xFFB3261E)
                    else -> Color(0xFF1B7F3A)
                }
            )
        }
    }
}

// ---------------------------------------------------------------- 房间内

@Composable
fun RoomScreen(ui: UiState, viewModel: AppViewModel) {
    val room = ui.room
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        if (room == null) {
            // 房间快照尚未到达（刚加入 / 刚重连）：给一句占位提示，避免空白界面
            Text("正在进入房间...", fontSize = 14.sp, color = Color.Gray)
            return@Column
        }

        Card(modifier = Modifier.fillMaxWidth()) {
            Column(modifier = Modifier.padding(14.dp)) {
                Text(room.name, fontSize = 20.sp, fontWeight = FontWeight.Bold)
                Spacer(Modifier.height(4.dp))
                Text(
                    "房主：${room.owner}    ${room.members.size}/${room.capacity} 人" +
                        if (room.inGame) "    对局中" else "    等待中",
                    fontSize = 13.sp,
                    color = Color.Gray
                )
            }
        }

        Spacer(Modifier.height(12.dp))
        Card(modifier = Modifier.fillMaxWidth()) {
            Column(modifier = Modifier.padding(14.dp)) {
                room.members.forEach { member ->
                    Row(
                        modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text(
                            text = member.name + if (member.owner) "（房主）" else "",
                            fontSize = 15.sp
                        )
                        Spacer(Modifier.weight(1f))
                        Text(
                            text = if (member.ready) "已准备" else "未准备",
                            fontSize = 13.sp,
                            color = if (member.ready) Color(0xFF1B7F3A) else Color.Gray
                        )
                    }
                }
                if (room.members.size < room.capacity) {
                    Text("等待其他玩家加入...", fontSize = 13.sp, color = Color.Gray,
                        modifier = Modifier.padding(vertical = 4.dp))
                }
            }
        }

        Spacer(Modifier.weight(1f))

        if (ui.message.isNotBlank()) {
            Text(
                text = ui.message,
                fontSize = 13.sp,
                color = Color(0xFFB26A00),
                modifier = Modifier.fillMaxWidth().padding(vertical = 6.dp)
            )
        }

        Button(
            onClick = viewModel::toggleReady,
            enabled = !room.inGame,
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(if (ui.selfReady) "取消准备" else "准备")
        }
        Spacer(Modifier.height(8.dp))
        Button(
            onClick = viewModel::leaveRoom,
            enabled = !room.inGame,
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(if (room.inGame) "对局进行中" else "离开房间")
        }
        Spacer(Modifier.height(4.dp))
        Text(
            text = if (room.members.size < room.capacity) "等待第二名玩家加入房间"
            else if (ui.selfReady) "已准备，等待对手准备后自动开局"
            else "双方都准备后自动开局",
            fontSize = 12.sp,
            color = Color.Gray
        )
    }
}
