package com.unool.android.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.HorizontalDivider
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
import com.unool.android.model.ShopHero
import com.unool.android.model.ShopItem
import com.unool.android.viewmodel.AppViewModel
import com.unool.android.viewmodel.UiState

@Composable
fun ShopScreen(ui: UiState, viewModel: AppViewModel) {
    val shop = ui.shop
    val filteredHeroes = shop.heroes.filter { it.name.contains(shop.keyword, ignoreCase = true) }
    val totalPages = if (shop.heroesTab) maxOf(1, (filteredHeroes.size + 9) / 10) else 1
    val pageHeroes = filteredHeroes.chunked(10).getOrNull(shop.page).orEmpty()

    Column(modifier = Modifier.fillMaxSize().padding(10.dp)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Button(onClick = { viewModel.setShopTab(true) }, enabled = !shop.heroesTab) {
                Text(if (shop.heroesTab) "英雄商城" else "切换英雄")
            }
            Spacer(Modifier.width(8.dp))
            Button(onClick = { viewModel.setShopTab(false) }, enabled = shop.heroesTab) {
                Text(if (shop.heroesTab) "切换道具" else "道具商城")
            }
            Spacer(Modifier.width(12.dp))
            OutlinedTextField(
                value = shop.keyword,
                onValueChange = viewModel::setShopKeyword,
                label = { Text("搜索角色") },
                singleLine = true,
                modifier = Modifier.width(180.dp).height(52.dp)
            )
            Spacer(Modifier.weight(1f))
            Text("积分：${shop.points}", fontSize = 16.sp, fontWeight = FontWeight.Bold)
            Spacer(Modifier.width(12.dp))
            Button(onClick = viewModel::refreshShop) { Text("刷新") }
            Spacer(Modifier.width(8.dp))
            Button(onClick = viewModel::exitShop) { Text("返回大厅") }
        }

        Spacer(Modifier.height(8.dp))
        Row(modifier = Modifier.fillMaxSize().weight(1f)) {
            Column(modifier = Modifier.weight(0.55f)) {
                if (!shop.dataReady) {
                    Text("正在加载商城数据...")
                } else if (shop.heroesTab) {
                    if (pageHeroes.isEmpty()) {
                        Text(if (shop.keyword.isBlank()) "暂无可浏览的英雄" else "未找到匹配的角色")
                    } else {
                        LazyColumn(modifier = Modifier.fillMaxWidth()) {
                            items(pageHeroes, key = { it.name }) { hero ->
                                HeroRow(hero, shop.expandedHero == hero.name, viewModel)
                            }
                        }
                    }
                } else {
                    if (shop.items.isEmpty()) {
                        Text("暂无可购买的道具")
                    } else {
                        LazyColumn(modifier = Modifier.fillMaxWidth()) {
                            items(shop.items, key = { it.name }) { item ->
                                ItemRow(item, shop.selectedItem == item.name, viewModel)
                            }
                        }
                    }
                }
            }

            Spacer(Modifier.width(10.dp))
            Column(modifier = Modifier.weight(0.45f)) {
                Card(modifier = Modifier.fillMaxWidth()) {
                    Column(modifier = Modifier.padding(12.dp)) {
                        val hero = shop.heroes.firstOrNull { it.name == shop.expandedHero }
                        val item = shop.items.firstOrNull { it.name == shop.selectedItem }
                        when {
                            hero != null -> HeroDetail(hero, shop.skillInfo[hero.name] ?: "", viewModel)
                            item != null -> ItemDetail(item, viewModel)
                            else -> Text("点击左侧条目查看详情", color = Color.Gray)
                        }
                    }
                }
            }
        }

        Spacer(Modifier.height(6.dp))
        Row(verticalAlignment = Alignment.CenterVertically) {
            Button(onClick = { viewModel.shopPage(-1) }, enabled = shop.page > 0) { Text("上一页") }
            Spacer(Modifier.width(8.dp))
            Text("第 ${shop.page + 1}/$totalPages 页", fontSize = 13.sp)
            Spacer(Modifier.width(8.dp))
            Button(onClick = { viewModel.shopPage(1) }, enabled = shop.page < totalPages - 1) { Text("下一页") }
            Spacer(Modifier.weight(1f))
            Text(shop.message, color = if (shop.message.contains("失败")) Color(0xFFB3261E)
            else MaterialTheme.colorScheme.primary, fontSize = 13.sp)
        }
    }
}

@Composable
private fun HeroRow(hero: ShopHero, expanded: Boolean, viewModel: AppViewModel) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 3.dp)
            .clickable { viewModel.toggleHero(hero.name) }
    ) {
        Row(
            modifier = Modifier.padding(10.dp).fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column(modifier = Modifier.weight(1f)) {
                Text("${hero.name}（${hero.level}）", fontWeight = FontWeight.Medium)
                Text(
                    when {
                        hero.unlocked -> "已解锁"
                        hero.price == null -> "不可购买"
                        else -> "售价 ${hero.price}"
                    },
                    fontSize = 12.sp,
                    color = Color.Gray
                )
            }
            Button(onClick = { viewModel.buyHero(hero) }, enabled = !hero.unlocked) {
                Text("购买")
            }
        }
    }
}

@Composable
private fun ItemRow(item: ShopItem, selected: Boolean, viewModel: AppViewModel) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 3.dp)
            .clickable { viewModel.toggleItem(item.name) }
    ) {
        Row(
            modifier = Modifier.padding(10.dp).fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column(modifier = Modifier.weight(1f)) {
                Text("${item.name}  当前数量：${item.count}", fontWeight = FontWeight.Medium)
                Text(
                    if (item.available) "售价 ${item.price}" else "不可购买",
                    fontSize = 12.sp,
                    color = Color.Gray
                )
            }
            Button(onClick = { viewModel.buyItem(item) }, enabled = item.available) {
                Text("购买")
            }
        }
    }
}

@Composable
private fun HeroDetail(hero: ShopHero, skillText: String, viewModel: AppViewModel) {
    Text("${hero.name}（${hero.level}）", fontSize = 17.sp, fontWeight = FontWeight.Bold)
    Spacer(Modifier.height(6.dp))
    Text(skillText.ifBlank { "技能描述加载中..." }, fontSize = 13.sp)
    Spacer(Modifier.height(10.dp))
    HorizontalDivider()
    Spacer(Modifier.height(8.dp))
    Text("皮肤：", fontWeight = FontWeight.Medium)
    hero.skins.forEach { skin ->
        Row(
            modifier = Modifier.fillMaxWidth().padding(vertical = 3.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                "${skin.name}（${skin.quality}）",
                fontSize = 13.sp,
                modifier = Modifier.weight(1f)
            )
            Text(
                when {
                    skin.unlocked -> "已解锁"
                    skin.price == null -> "不售卖"
                    else -> "售价 ${skin.price}"
                },
                fontSize = 11.sp,
                color = Color.Gray
            )
            Spacer(Modifier.width(6.dp))
            Button(onClick = { viewModel.buySkin(hero, skin) }, enabled = !skin.unlocked && skin.price != null) {
                Text("购买")
            }
        }
    }
}

@Composable
private fun ItemDetail(item: ShopItem, viewModel: AppViewModel) {
    Text(item.name, fontSize = 17.sp, fontWeight = FontWeight.Bold)
    Spacer(Modifier.height(6.dp))
    Text("效果说明", fontWeight = FontWeight.Medium)
    Text(item.description.ifBlank { "该道具暂无说明" }, fontSize = 13.sp)
    Spacer(Modifier.height(8.dp))
    item.maxCount?.let { Text("购买上限 $it 次，已购 ${item.count} 次", fontSize = 12.sp, color = Color.Gray) }
    Spacer(Modifier.height(10.dp))
    Button(onClick = { viewModel.buyItem(item) }, enabled = item.available) { Text("购买") }
}
