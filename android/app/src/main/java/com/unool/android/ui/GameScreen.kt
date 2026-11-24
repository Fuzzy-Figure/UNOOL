package com.unool.android.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.unool.android.model.CardCatalog
import com.unool.android.model.CardColor
import com.unool.android.model.CardName
import com.unool.android.protocol.CardData
import com.unool.android.protocol.ChoicePrompt
import com.unool.android.protocol.GameStateData
import com.unool.android.protocol.PlayerStateData
import com.unool.android.viewmodel.AppViewModel
import com.unool.android.viewmodel.UiState
import kotlinx.coroutines.delay

@Composable
fun GameScreen(ui: UiState, viewModel: AppViewModel) {
    val game = ui.game
    val state = game.state
    val choice = game.choice
    val localPlayer = state?.players?.firstOrNull { it.id == ui.playerId }
    val canSelect = state?.operatingPlayerId == ui.playerId &&
        (choice == null || choice.options.isEmpty())

    var now by remember { mutableStateOf(System.currentTimeMillis()) }
    LaunchedEffect(choice) {
        while (choice != null) {
            now = System.currentTimeMillis()
            delay(100)
        }
    }
    val remainingSeconds = choice?.timeoutMs?.let { timeout ->
        (timeout - (now - game.choiceStartedAt)).coerceAtLeast(0) / 1000
    }

    Box(modifier = Modifier.fillMaxSize().padding(8.dp)) {
        // 玩家区：与 Windows 一致，一号位在左上、二号位在左下
        state?.players?.forEachIndexed { index, player ->
            PlayerArea(
                player = player,
                index = index,
                localId = ui.playerId,
                selectedIndex = if (player.id == ui.playerId) game.selectedIndex else -1,
                canSelect = canSelect && player.id == ui.playerId,
                onSelect = { viewModel.selectCard(it) },
                modifier = Modifier.align(if (index == 0) Alignment.TopStart else Alignment.BottomStart)
            )
        }

        // 中央：弃牌堆
        DiscardArea(state, Modifier.align(Alignment.Center))

        // 顶部中央：当前颜色与牌名（避免压到左侧角色立绘）
        Row(modifier = Modifier.align(Alignment.TopCenter).padding(top = 4.dp)) {
            Text("当前颜色：${CardColor.label(state?.currentColor ?: 0)}　", fontSize = 12.sp)
            Text("当前牌名：${CardName.label(state?.currentName ?: 0)}", fontSize = 12.sp)
        }

        // 右上：局数 / 胜负 / 结果
        Column(modifier = Modifier.align(Alignment.TopEnd).padding(end = 140.dp)) {
            localPlayer?.let {
                Text("第 ${state?.gameCount ?: 0} 局  胜${it.wins} 负${it.losses}", fontSize = 12.sp)
            }
            if (game.resultText.isNotBlank()) {
                Text(game.resultText, fontSize = 12.sp, color = Color(0xFFB26A00), fontWeight = FontWeight.Bold)
            }
        }

        // 右侧选择提示（Windows 的 displayTextInRight，改成可点）
        Box(
            modifier = Modifier
                .align(Alignment.CenterEnd)
                .padding(end = 108.dp)
                .width(240.dp)
                .background(Color(0xD6FFFFFF))
                .padding(6.dp)
                .verticalScroll(rememberScrollState())
        ) {
            ChoicePanel(choice, remainingSeconds, viewModel)
        }

        // 最右侧竖排操作：↑ 出牌，↓ 取消；下方两个小按钮保留整理/炸弹
        Column(
            modifier = Modifier.align(Alignment.CenterEnd),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center
        ) {
            Button(
                onClick = viewModel::confirmPlay,
                enabled = canSelect,
                modifier = Modifier.size(width = 64.dp, height = 56.dp)
            ) { Text("↑", fontSize = 28.sp) }

            Spacer(Modifier.height(8.dp))

            Button(
                onClick = viewModel::cancelSelection,
                enabled = canSelect,
                modifier = Modifier.size(width = 64.dp, height = 56.dp)
            ) { Text("↓", fontSize = 28.sp) }

            Spacer(Modifier.height(12.dp))

            Button(
                onClick = viewModel::sortHand,
                enabled = canSelect,
                modifier = Modifier.size(width = 64.dp, height = 40.dp)
            ) { Text("整", fontSize = 13.sp) }

            Spacer(Modifier.height(4.dp))

            Button(
                onClick = viewModel::useBomb,
                enabled = canSelect,
                modifier = Modifier.size(width = 64.dp, height = 40.dp)
            ) { Text("炸", fontSize = 13.sp) }
        }
    }
}

@Composable
private fun PlayerArea(
    player: PlayerStateData,
    index: Int,
    localId: Long,
    selectedIndex: Int,
    canSelect: Boolean,
    onSelect: (Int) -> Unit,
    modifier: Modifier = Modifier
) {
    Row(modifier = modifier, verticalAlignment = Alignment.Top) {
        Column {
            Row {
                player.characterImagePaths.take(2).forEach { path ->
                    AssetImage(
                        path = path,
                        modifier = Modifier.size(width = 104.dp, height = 134.dp).padding(end = 2.dp),
                        maxDimension = 256
                    )
                }
            }
            Text(
                "${if (index == 0) "一号位" else "二号位"}${if (player.id == localId) "（我）" else ""}",
                fontSize = 11.sp,
                fontWeight = FontWeight.Medium
            )
            val shield = if (player.shield > 0) "(+${player.shield})" else ""
            Text("体力：${player.hp}$shield/${player.maxHp}", fontSize = 11.sp)
            Text(
                "手牌数：${player.hand.cards.size}" +
                    if (player.id == localId) "  价值：${player.hand.cards.sumOf { CardCatalog.value(it) }}" else "",
                fontSize = 11.sp
            )
        }

        Spacer(Modifier.width(4.dp))

        val isLocal = player.id == localId
        Row(modifier = Modifier.horizontalScroll(rememberScrollState())) {
            player.hand.cards.forEachIndexed { i, card ->
                // Windows 的 displayHand：后续卡牌向左压住前一张约 1/3 宽度
                Box(modifier = Modifier.offset(x = if (i == 0) 0.dp else (-20).dp)) {
                    if (isLocal) {
                        HandCard(
                            card = card,
                            selected = i == selectedIndex,
                            enabled = canSelect,
                            onClick = { onSelect(i) }
                        )
                    } else {
                        BackCard()
                    }
                }
            }
            Spacer(Modifier.width(2.dp))
        }
    }
}

@Composable
private fun HandCard(card: CardData, selected: Boolean, enabled: Boolean, onClick: () -> Unit) {
    val borderColor = if (selected) MaterialTheme.colorScheme.primary else Color(0xFFDDDDDD)
    Box(
        modifier = Modifier
            .size(width = 62.dp, height = 93.dp)
            .border(if (selected) 3.dp else 1.dp, borderColor)
            .background(if (selected) Color(0xFFE3EEFF) else Color.White)
            .clickable(enabled = enabled, onClick = onClick)
    ) {
        AssetImage(
            path = CardCatalog.imagePath(card),
            modifier = Modifier.matchParentSize(),
            maxDimension = 256
        )
    }
}

@Composable
private fun BackCard() {
    Box(
        modifier = Modifier
            .size(width = 62.dp, height = 93.dp)
            .border(1.dp, Color(0xFFDDDDDD))
            .background(Color.White)
    ) {
        AssetImage(
            path = "images/cards/back.jpg",
            modifier = Modifier.matchParentSize(),
            maxDimension = 128
        )
    }
}

@Composable
private fun DiscardArea(state: GameStateData?, modifier: Modifier = Modifier) {
    val discards = state?.discards.orEmpty()
    val current = discards.lastOrNull()
    val history = discards.takeLast(4).dropLast(1)

    Column(modifier = modifier, horizontalAlignment = Alignment.CenterHorizontally) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            history.forEach { card ->
                AssetImage(
                    path = CardCatalog.imagePath(card),
                    modifier = Modifier.size(width = 38.dp, height = 57.dp).padding(end = 2.dp),
                    maxDimension = 128
                )
            }
            if (current != null) {
                AssetImage(
                    path = CardCatalog.imagePath(current),
                    modifier = Modifier.size(width = 66.dp, height = 99.dp),
                    maxDimension = 256
                )
            }
        }
    }
}

@Composable
private fun ChoicePanel(choice: ChoicePrompt?, remainingSeconds: Long?, viewModel: AppViewModel) {
    if (choice == null) {
        Text("当前没有待选项", color = Color.Gray, fontSize = 12.sp)
        return
    }

    Column {
        Text(choice.title, fontSize = 14.sp, fontWeight = FontWeight.Bold)
        Spacer(Modifier.height(2.dp))
        if (choice.totalPages > 1) {
            Text(
                "第 ${choice.currentPage + 1}/${choice.totalPages} 页  ← → 翻页",
                fontSize = 11.sp,
                color = Color.Gray
            )
        }
        remainingSeconds?.let { Text("剩余时间：$it 秒", fontSize = 11.sp, color = Color(0xFFB26A00)) }
        if (choice.errorMsg.isNotBlank()) {
            Text(choice.errorMsg, fontSize = 11.sp, color = Color(0xFFB3261E))
        }
        Spacer(Modifier.height(4.dp))

        choice.options.forEachIndexed { index, option ->
            val meta = choice.optionMeta.getOrNull(index)
            val label = buildString {
                append("[${index + 1}] $option")
                if (meta != null && meta.characterName.isNotBlank()) {
                    append("  ${meta.characterName}（${meta.level}） 体力：${meta.hp}")
                }
            }
            Text(
                text = label,
                fontSize = 12.sp,
                color = MaterialTheme.colorScheme.primary,
                modifier = Modifier
                    .width(220.dp)
                    .clickable(enabled = index < 9) { viewModel.chooseOption(index + 1) }
                    .padding(vertical = 2.dp)
            )
        }

        if (!choice.forced && choice.options.isNotEmpty()) {
            Text(
                "[0] 不选择",
                fontSize = 12.sp,
                color = MaterialTheme.colorScheme.primary,
                modifier = Modifier.clickable { viewModel.cancelChoice() }
            )
        }
    }
}
