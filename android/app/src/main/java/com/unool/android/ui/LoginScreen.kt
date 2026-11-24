package com.unool.android.ui

import androidx.compose.foundation.background
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
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import com.unool.android.viewmodel.AppViewModel
import com.unool.android.viewmodel.UiState

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

@Composable
fun LobbyScreen(ui: UiState, viewModel: AppViewModel) {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Card(modifier = Modifier.fillMaxWidth()) {
            Column(modifier = Modifier.padding(14.dp)) {
                Text(
                    text = "账号：${ui.username}    积分：${ui.points}    胜：${ui.wins}    负：${ui.losses}",
                    fontSize = 16.sp,
                    fontWeight = FontWeight.Medium
                )
            }
        }

        Spacer(Modifier.height(12.dp))
        if (ui.matchNotice.isNotBlank()) {
            Text(
                text = ui.matchNotice,
                color = Color(0xFFB26A00),
                fontSize = 14.sp,
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Color(0xFFFFF4D6))
                    .padding(10.dp)
            )
            Spacer(Modifier.height(10.dp))
        }

        Row {
            Button(onClick = viewModel::checkin) { Text("每日签到") }
            Spacer(Modifier.width(10.dp))
            if (ui.matching) {
                Button(onClick = viewModel::cancelMatchmaking) { Text("退出匹配") }
            } else {
                Button(onClick = viewModel::readyToPlay) { Text("开始游戏") }
            }
            Spacer(Modifier.width(10.dp))
            Button(onClick = viewModel::openShop) { Text("商城") }
        }

        Spacer(Modifier.height(12.dp))
        if (ui.status.isNotBlank()) {
            Text(text = ui.status, fontSize = 14.sp, color = MaterialTheme.colorScheme.primary)
        }
        Spacer(Modifier.height(6.dp))
        Text("点击「开始游戏」进入匹配，双方都准备后开始对战", fontSize = 12.sp, color = Color.Gray)
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
