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
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
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
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import com.unool.android.model.CardCatalog
import com.unool.android.model.CardColor
import com.unool.android.model.CardName
import com.unool.android.model.DiscardReason
import com.unool.android.protocol.CardData
import com.unool.android.protocol.ChoicePrompt
import com.unool.android.protocol.GameStateData
import com.unool.android.protocol.PlayerStateData
import com.unool.android.viewmodel.AppViewModel
import com.unool.android.viewmodel.UiState
import kotlinx.coroutines.delay

// 对战手牌高度；掉线提示图也按这个边长显示为正方形。
internal val HandCardHeight = 93.dp

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

    var infoPlayerIndex by remember { mutableStateOf<Int?>(null) }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .padding(start = 8.dp, top = 8.dp, end = 8.dp, bottom = 0.dp)
    ) {
        // 玩家区：己方（本地玩家）始终在下方，对手在上方
        state?.players?.forEachIndexed { index, player ->
            val isLocal = player.id == ui.playerId
            PlayerArea(
                player = player,
                index = index,
                localId = ui.playerId,
                selectedIndex = if (isLocal) game.selectedIndex else -1,
                canSelect = canSelect && isLocal,
                onSelect = { viewModel.selectCard(it) },
                charInfo = game.charInfo[index],
                onPortraitClick = { infoPlayerIndex = index },
                modifier = Modifier
                    .align(if (isLocal) Alignment.BottomStart else Alignment.TopStart)
                    .then(if (isLocal) Modifier.fillMaxWidth() else Modifier)
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

        // 选择提示（ask 文字）：贴最右侧居中，仅留少量间距避开导航键
        Box(
            modifier = Modifier
                .align(Alignment.CenterEnd)
                .padding(end = 28.dp)
                .width(200.dp)
                .background(Color(0xD6FFFFFF))
                .padding(6.dp)
                .verticalScroll(rememberScrollState())
        ) {
            ChoicePanel(choice, remainingSeconds, viewModel)
        }

        // 右下角操作区：第1行主动技横排；第2行 整理 + ↑；第3行 炸弹 + ↓
        Column(
            modifier = Modifier.align(Alignment.BottomEnd).padding(end = 72.dp, bottom = 4.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Bottom
        ) {
            // 第1行：主动技按钮，横向排列（绿底白字）
            if (canSelect && state?.availableSkills?.isNotEmpty() == true) {
                Row(modifier = Modifier.horizontalScroll(rememberScrollState())) {
                    state.availableSkills.forEachIndexed { i, name ->
                        Button(
                            onClick = { viewModel.useSkill(i + 1) },
                            modifier = Modifier.height(28.dp).padding(end = 3.dp),
                            contentPadding = PaddingValues(horizontal = 5.dp, vertical = 0.dp),
                            colors = ButtonDefaults.buttonColors(
                                containerColor = Color(0xFF1B7F3A),
                                contentColor = Color.White
                            )
                        ) { Text("${i + 1}·$name", fontSize = 10.sp) }
                    }
                }
                Spacer(Modifier.height(4.dp))
            }

            // 第2行：整理手牌 + 上箭头
            Row(verticalAlignment = Alignment.CenterVertically) {
                Button(
                    onClick = viewModel::sortHand,
                    enabled = canSelect,
                    modifier = Modifier.size(width = 62.dp, height = 34.dp),
                    contentPadding = PaddingValues(0.dp)
                ) { Text("整理手牌", fontSize = 11.sp) }
                Spacer(Modifier.width(4.dp))
                Button(
                    onClick = viewModel::confirmPlay,
                    enabled = canSelect,
                    modifier = Modifier.size(width = 44.dp, height = 40.dp),
                    contentPadding = PaddingValues(0.dp)
                ) { Text("↑", fontSize = 22.sp) }
            }

            Spacer(Modifier.height(4.dp))

            // 第3行：炸弹 + 下箭头
            Row(verticalAlignment = Alignment.CenterVertically) {
                Button(
                    onClick = viewModel::useBomb,
                    enabled = canSelect,
                    modifier = Modifier.size(width = 62.dp, height = 34.dp),
                    contentPadding = PaddingValues(0.dp)
                ) { Text("炸弹", fontSize = 11.sp) }
                Spacer(Modifier.width(4.dp))
                Button(
                    onClick = viewModel::cancelSelection,
                    enabled = canSelect,
                    modifier = Modifier.size(width = 44.dp, height = 40.dp),
                    contentPadding = PaddingValues(0.dp)
                ) { Text("↓", fontSize = 22.sp) }
            }
        }

        infoPlayerIndex?.let { idx ->
            Dialog(onDismissRequest = { infoPlayerIndex = null }) {
                Card {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Text(
                            "${if (idx == 0) "一号位" else "二号位"}角色信息",
                            fontSize = 15.sp,
                            fontWeight = FontWeight.Bold
                        )
                        Spacer(Modifier.height(8.dp))
                        val infoText = game.charInfo[idx]?.takeIf { it.isNotBlank() } ?: run {
                            val playerInfo = state?.players?.getOrNull(idx)
                            buildString {
                                append("角色信息尚未同步，先显示对局内基础信息：\n\n")
                                if (playerInfo != null) {
                                    append("角色：")
                                    append(playerInfo.characterNames.joinToString(" + ").ifBlank { "未知" })
                                    append('\n')
                                    append("皮肤：")
                                    append(playerInfo.skins.joinToString("、").ifBlank { "默认" })
                                    append('\n')
                                    append("体力：${playerInfo.hp}/${playerInfo.maxHp}　护盾：${playerInfo.shield}\n")
                                    if (playerInfo.marks.isNotEmpty()) {
                                        append("标记：")
                                        append(playerInfo.marks.entries.joinToString("、") { "${it.key}:${it.value}" })
                                        append('\n')
                                    }
                                }
                                else {
                                    append("暂无对局数据")
                                }
                            }
                        }
                        Text(
                            text = infoText,
                            fontSize = 12.sp,
                            modifier = Modifier
                                .heightIn(max = 360.dp)
                                .verticalScroll(rememberScrollState())
                        )
                        Spacer(Modifier.height(12.dp))
                        Button(onClick = { infoPlayerIndex = null }) { Text("关闭") }
                    }
                }
            }
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
    charInfo: String?,
    onPortraitClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val isLocal = player.id == localId
    Row(modifier = modifier, verticalAlignment = Alignment.Bottom) {
        Column {
            // 己方：文字在上、立绘在下，让立绘贴到屏幕最下沿；对手保持立绘在上
            if (isLocal) {
                PlayerTextInfo(player, index, isLocal)
            }
            Row(
                modifier = Modifier
                    .heightIn(min = 116.dp)
                    .clickable { onPortraitClick() }
            ) {
                player.characterImagePaths.take(2).forEach { path ->
                    AssetImage(
                        path = path,
                        modifier = Modifier.size(width = 90.dp, height = 116.dp).padding(end = 2.dp),
                        maxDimension = 256,
                        contentScale = ContentScale.FillBounds
                    )
                }
            }
            if (!isLocal) {
                PlayerTextInfo(player, index, isLocal)
            }
        }

        Spacer(Modifier.width(4.dp))

        // 对方只显示手牌数，不显示牌面/牌背
        if (isLocal) {
            // 选中牌完整展开，其余牌折叠为一半宽度；总宽超出屏幕时可左右滑动。
            val cardWidth = 62.dp
            val foldStep = (cardWidth.value / 2).dp
            val fullStep = cardWidth + 2.dp
            var cursor = 0.dp
            val offsets = player.hand.cards.mapIndexed { i, _ ->
                val current = cursor
                cursor = cursor + if (i == selectedIndex) fullStep else foldStep
                current
            }
            val totalWidth = (offsets.lastOrNull() ?: 0.dp) + cardWidth

            Row(modifier = Modifier.weight(1f).horizontalScroll(rememberScrollState())) {
                Box(modifier = Modifier.width(totalWidth).height(HandCardHeight)) {
                    player.hand.cards.forEachIndexed { i, card ->
                        Box(modifier = Modifier.offset(x = offsets[i])) {
                            HandCard(
                                card = card,
                                selected = i == selectedIndex,
                                enabled = canSelect,
                                onClick = { onSelect(i) }
                            )
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun PlayerTextInfo(player: PlayerStateData, index: Int, isLocal: Boolean) {
    Text(
        "${if (index == 0) "一号位" else "二号位"}${if (isLocal) "（我）" else ""}",
        fontSize = 10.sp,
        lineHeight = 12.sp,
        fontWeight = FontWeight.Medium
    )
    val shield = if (player.shield > 0) "(+${player.shield})" else ""
    Text("体力：${player.hp}$shield/${player.maxHp}", fontSize = 10.sp, lineHeight = 12.sp)
    Text(
        "手牌数：${player.hand.cards.size}" +
            if (isLocal) "  价值：${player.hand.cards.sumOf { CardCatalog.value(it) }}" else "",
        fontSize = 10.sp,
        lineHeight = 12.sp
    )
}

@Composable
private fun HandCard(card: CardData, selected: Boolean, enabled: Boolean, onClick: () -> Unit) {
    // 不能选牌时不画选中框，也不高亮
    val showSelection = enabled && selected
    val borderColor = if (showSelection) MaterialTheme.colorScheme.primary else Color.Transparent
    Box(
        modifier = Modifier
            .size(width = 62.dp, height = HandCardHeight)
            .border(if (showSelection) 3.dp else 0.dp, borderColor)
            .background(if (showSelection) Color(0xFFE3EEFF) else Color.White)
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
            .size(width = 62.dp, height = HandCardHeight)
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
    // 服务端 discardPile 是新牌在前面：front 为当前牌，后面依次是更早的牌。
    val current = discards.firstOrNull()
    val history = discards.drop(1).take(3).reversed()

    Column(modifier = modifier, horizontalAlignment = Alignment.CenterHorizontally) {
        Row(verticalAlignment = Alignment.Top) {
            // 历史牌为当前牌 60% 高，顶部下移使其垂直居中，与 Windows 端一致。
            history.forEach { card ->
                Column(
                    horizontalAlignment = Alignment.CenterHorizontally,
                    modifier = Modifier.padding(top = 21.dp, end = 2.dp)
                ) {
                    AssetImage(
                        path = CardCatalog.imagePath(card),
                        modifier = Modifier.size(width = 38.dp, height = 57.dp),
                        maxDimension = 128
                    )
                    val reason = DiscardReason.label(card.discardReason)
                    if (reason.isNotBlank()) {
                        Text(reason, fontSize = 9.sp, color = Color.Gray, maxLines = 1)
                    }
                }
            }
            if (current != null) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    AssetImage(
                        path = CardCatalog.imagePath(current),
                        modifier = Modifier.size(width = 66.dp, height = 99.dp),
                        maxDimension = 256
                    )
                    val reason = DiscardReason.label(current.discardReason)
                    if (reason.isNotBlank()) {
                        Text(reason, fontSize = 10.sp, color = Color.Gray, maxLines = 1)
                    }
                }
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
                    .width(168.dp)
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
