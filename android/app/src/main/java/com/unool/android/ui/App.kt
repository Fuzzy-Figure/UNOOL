package com.unool.android.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.runtime.collectAsState
import com.unool.android.net.ConnectionState
import com.unool.android.viewmodel.AppViewModel
import com.unool.android.viewmodel.Screen
import com.unool.android.viewmodel.UiState

@Composable
fun UnoolApp(viewModel: AppViewModel) {
    val ui by viewModel.ui.collectAsState()
    val connected = ui.connection is ConnectionState.Connected
    val connecting = ui.connection is ConnectionState.Connecting

    Box(
        modifier = Modifier
            .fillMaxSize()
            .statusBarsPadding()
            .imePadding()
            .background(MaterialTheme.colorScheme.background)
    ) {
        when (ui.screen) {
            Screen.Login -> LoginScreen(ui, viewModel)
            Screen.Lobby -> LobbyScreen(ui, viewModel)
            Screen.Rooms -> RoomsScreen(ui, viewModel)
            Screen.Room -> RoomScreen(ui, viewModel)
            Screen.Shop -> ShopScreen(ui, viewModel)
            Screen.Game -> GameScreen(ui, viewModel)
        }

        // 小尺寸连接按钮悬浮右上角，避开导航键区域
        Row(modifier = Modifier.align(Alignment.TopEnd).padding(end = 72.dp, top = 2.dp)) {
            Button(
                onClick = viewModel::reconnect,
                enabled = !connecting,
                modifier = Modifier.width(64.dp).height(32.dp),
                contentPadding = PaddingValues(horizontal = 4.dp, vertical = 0.dp)
            ) { Text(if (connecting) "连接中" else "重连", fontSize = 11.sp) }
            if (connected) {
                Spacer(Modifier.width(4.dp))
                Button(
                    onClick = viewModel::disconnect,
                    modifier = Modifier.width(64.dp).height(32.dp),
                    contentPadding = PaddingValues(horizontal = 4.dp, vertical = 0.dp)
                ) { Text("断开", fontSize = 11.sp) }
            }
        }
    }
}

@Composable
private fun ConnectionBar(ui: UiState, viewModel: AppViewModel) {
    val connected = ui.connection is ConnectionState.Connected
    val connecting = ui.connection is ConnectionState.Connecting
    val status = when (val state = ui.connection) {
        ConnectionState.Disconnected -> "未连接"
        ConnectionState.Connecting -> "连接中..."
        ConnectionState.Connected -> "已连接（玩家 ${ui.playerId}）"
        is ConnectionState.Failed -> "连接失败：${state.reason}"
    }
    val statusColor = when {
        connected -> Color(0xFF1B7F3A)
        connecting -> Color(0xFFB26A00)
        else -> Color(0xFFB3261E)
    }

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(MaterialTheme.colorScheme.surface)
            .padding(horizontal = 10.dp, vertical = 5.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = "UNOOL 安卓端",
            fontSize = 15.sp,
            fontWeight = FontWeight.Bold,
            color = MaterialTheme.colorScheme.primary
        )
        Spacer(Modifier.width(10.dp))
        Text(text = status, fontSize = 12.sp, color = statusColor)
        Spacer(Modifier.weight(1f))

        Button(onClick = viewModel::reconnect, enabled = !connecting) {
            Text(if (connecting) "连接中" else "重连")
        }
        if (connected) {
            Spacer(Modifier.width(6.dp))
            Button(onClick = viewModel::disconnect) {
                Text("断开")
            }
        }
        // 避开右侧系统导航键区域
        Spacer(Modifier.width(72.dp))
    }
}
