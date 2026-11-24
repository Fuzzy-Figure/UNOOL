package com.unool.android.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.unool.android.model.ShopHero
import com.unool.android.model.ShopItem
import com.unool.android.model.ShopParser
import com.unool.android.model.ShopSkin
import com.unool.android.net.ConnectionState
import com.unool.android.net.GameConnection
import com.unool.android.protocol.ChoicePrompt
import com.unool.android.protocol.ClientMessage
import com.unool.android.protocol.GameStateData
import com.unool.android.protocol.Key
import com.unool.android.protocol.Msg
import com.unool.android.protocol.ServerMessage
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

enum class Screen { Login, Lobby, Shop, Game }
enum class LoginMode { Login, Register }

data class ShopUiState(
    val dataReady: Boolean = false,
    val heroesTab: Boolean = true,
    val keyword: String = "",
    val page: Int = 0,
    val heroes: List<ShopHero> = emptyList(),
    val items: List<ShopItem> = emptyList(),
    val points: Int = 0,
    val message: String = "",
    val skillInfo: Map<String, String> = emptyMap(),
    val expandedHero: String? = null,
    val selectedItem: String? = null
)

data class GameUiState(
    val state: GameStateData? = null,
    val choice: ChoicePrompt? = null,
    val choiceStartedAt: Long = 0L,
    val charInfo: Map<Int, String> = emptyMap(),
    val skillInfo: Map<String, String> = emptyMap(),
    val selectedIndex: Int = 0,
    val resultText: String = "",
    val log: List<String> = emptyList()
)

data class UiState(
    val connection: ConnectionState = ConnectionState.Disconnected,
    val host: String = "120.226.44.187",
    val port: String = "14341",
    val screen: Screen = Screen.Login,
    val loginMode: LoginMode = LoginMode.Login,
    val username: String = "",
    val password: String = "",
    val message: String = "",
    val status: String = "",
    val points: Int = 0,
    val wins: Int = 0,
    val losses: Int = 0,
    val loggedIn: Boolean = false,
    val matching: Boolean = false,
    val matchNotice: String = "",
    val checkinPopup: String? = null,
    val playerId: Long = 0L,
    val shop: ShopUiState = ShopUiState(),
    val game: GameUiState = GameUiState()
)

class AppViewModel : ViewModel() {

    private val connection = GameConnection()
    private val _ui = MutableStateFlow(UiState())
    val ui: StateFlow<UiState> = _ui.asStateFlow()

    init {
        viewModelScope.launch {
            connection.state.collect { state ->
                _ui.update { it.copy(connection = state) }
            }
        }
        viewModelScope.launch {
            connection.messages.collect { message ->
                handleServerMessage(message)
            }
        }
        // 启动即自动连接固定服务器，用户不需要输入地址和端口。
        connect()
    }

    override fun onCleared() {
        connection.close()
        super.onCleared()
    }

    // ---------------------------------------------------------------- 连接与登录

    fun setHost(value: String) = _ui.update { it.copy(host = value) }

    fun setPort(value: String) = _ui.update { it.copy(port = value) }

    fun connect() {
        val port = _ui.value.port.toIntOrNull()
        if (port == null || port !in 1..65535) {
            _ui.update { it.copy(message = "端口必须是 1-65535 的数字") }
            return
        }
        connection.connect(_ui.value.host.trim(), port)
    }

    fun reconnect() {
        connection.disconnect()
        connect()
    }

    fun disconnect() {
        connection.disconnect()
        _ui.update {
            it.copy(
                loggedIn = false,
                matching = false,
                matchNotice = "",
                screen = Screen.Login,
                message = "已断开连接"
            )
        }
    }

    fun setUsername(value: String) = _ui.update { it.copy(username = value) }

    fun setPassword(value: String) = _ui.update { it.copy(password = value) }

    fun setLoginMode(mode: LoginMode) = _ui.update {
        it.copy(loginMode = mode, message = "")
    }

    fun submitLogin() {
        val s = _ui.value
        if (s.username.isBlank() || s.password.isBlank()) {
            _ui.update { it.copy(message = "用户名和密码不能为空") }
            return
        }
        _ui.update { it.copy(message = "登录中...") }
        connection.send(ClientMessage.Login(s.username, s.password))
    }

    fun register() {
        val s = _ui.value
        if (s.username.isBlank()) {
            _ui.update { it.copy(message = "用户名不能为空") }
            return
        }
        if (s.password.isBlank()) {
            _ui.update { it.copy(message = "密码不能为空") }
            return
        }
        _ui.update { it.copy(loginMode = LoginMode.Register, message = "检查用户名...") }
        connection.send(ClientMessage.CheckUsername(s.username))
    }

    fun checkin() {
        _ui.update { it.copy(status = "正在签到...") }
        connection.send(ClientMessage.Checkin)
    }

    fun dismissCheckin() = _ui.update { it.copy(checkinPopup = null) }

    fun readyToPlay() {
        _ui.update {
            it.copy(
                matching = true,
                matchNotice = "",
                status = "已准备，等待另一名玩家点击开始游戏"
            )
        }
        connection.send(ClientMessage.ReadyToPlay)
    }

    fun cancelMatchmaking() {
        _ui.update { it.copy(status = "正在退出匹配...") }
        connection.send(ClientMessage.CancelMatchmaking)
    }

    fun openShop() {
        _ui.update { it.copy(screen = Screen.Shop, shop = ShopUiState()) }
        connection.send(
            ClientMessage.ShopPurchase(ShopParser.purchaseJson("request", "", "", ""))
        )
    }

    fun exitShop() = _ui.update { it.copy(screen = Screen.Lobby) }

    // ---------------------------------------------------------------- 商城操作

    fun setShopTab(heroes: Boolean) = _ui.update {
        it.copy(shop = it.shop.copy(heroesTab = heroes, page = 0, message = ""))
    }

    fun setShopKeyword(value: String) = _ui.update {
        it.copy(shop = it.shop.copy(keyword = value, page = 0))
    }

    fun shopPage(delta: Int) = _ui.update { state ->
        val shop = state.shop
        val total = if (shop.heroesTab) {
            val count = shop.heroes.count {
                it.name.contains(shop.keyword, ignoreCase = true)
            }
            maxOf(1, (count + 9) / 10)
        } else 1
        state.copy(shop = shop.copy(page = (shop.page + delta).coerceIn(0, total - 1)))
    }

    fun toggleHero(name: String) = _ui.update { state ->
        state.copy(
            shop = state.shop.copy(
                expandedHero = if (state.shop.expandedHero == name) null else name
            )
        )
    }

    fun toggleItem(name: String) = _ui.update { state ->
        state.copy(shop = state.shop.copy(selectedItem = name))
    }

    fun buyHero(hero: ShopHero) {
        when {
            hero.unlocked -> setShopMessage("已解锁")
            hero.price == null -> setShopMessage("不可购买")
            else -> {
                setShopMessage("正在购买...")
                connection.send(
                    ClientMessage.ShopPurchase(
                        ShopParser.purchaseJson("character", hero.name, "", "")
                    )
                )
            }
        }
    }

    fun buySkin(hero: ShopHero, skin: ShopSkin) {
        when {
            !hero.unlocked -> setShopMessage("请先购买角色")
            skin.unlocked -> setShopMessage("已解锁")
            skin.price == null -> setShopMessage("不售卖")
            else -> {
                setShopMessage("正在购买...")
                connection.send(
                    ClientMessage.ShopPurchase(
                        ShopParser.purchaseJson("skin", "", hero.name, skin.name)
                    )
                )
            }
        }
    }

    fun buyItem(item: ShopItem) {
        if (!item.available) {
            setShopMessage("不可购买")
            return
        }
        setShopMessage("正在购买...")
        connection.send(
            ClientMessage.ShopPurchase(ShopParser.purchaseJson("item", item.name, "", ""))
        )
    }

    fun refreshShop() {
        connection.send(
            ClientMessage.ShopPurchase(ShopParser.purchaseJson("request", "", "", ""))
        )
    }

    fun requestSkillInfo(names: List<String>) {
        if (names.isEmpty()) return
        connection.send(ClientMessage.SkillInfoRequest(ShopParser.skillRequestJson(names)))
    }

    private fun setShopMessage(message: String) = _ui.update {
        it.copy(shop = it.shop.copy(message = message))
    }

    // ---------------------------------------------------------------- 游戏操作

    fun selectCard(index: Int) = _ui.update {
        it.copy(game = it.game.copy(selectedIndex = index))
    }

    private fun sendKey(key: Int) {
        val s = _ui.value
        connection.send(
            ClientMessage.ClientInput(key, s.playerId, s.game.selectedIndex.toLong())
        )
    }

    fun confirmPlay() = sendKey(Key.UP)
    fun cancelSelection() = sendKey(Key.DOWN)
    fun sortHand() = sendKey(Key.SPACE)
    fun useBomb() = sendKey(Key.B)
    fun chooseOption(index: Int) = sendKey(Key.digit(index))
    fun cancelChoice() = sendKey(Key.NUM_0)
    fun previousChoicePage() = sendKey(Key.LEFT)
    fun nextChoicePage() = sendKey(Key.RIGHT)

    // ---------------------------------------------------------------- 入站消息

    private fun handleServerMessage(message: ServerMessage) {
        when (message) {
            is ServerMessage.ConnectionInfo -> _ui.update { it.copy(playerId = message.playerId) }

            ServerMessage.ConnectionRefused -> _ui.update {
                it.copy(message = "连接被拒绝（服务器已满）")
            }

            is ServerMessage.AccountResponse -> onAccountResponse(message)

            is ServerMessage.CheckUsernameResponse -> {
                if (message.exists) {
                    _ui.update {
                        it.copy(message = "该用户名已存在", loginMode = LoginMode.Register)
                    }
                } else {
                    _ui.update { it.copy(message = "用户名可用，继续注册") }
                    val s = _ui.value
                    connection.send(ClientMessage.Register(s.username, s.password))
                }
            }

            is ServerMessage.CheckinResponse -> _ui.update {
                it.copy(
                    checkinPopup = message.msg,
                    points = message.points,
                    status = if (message.ok) "签到成功" else "签到失败",
                    shop = it.shop.copy(points = message.points)
                )
            }

            is ServerMessage.MatchmakingStarted -> {
                if (message.username.isBlank() || message.username == _ui.value.username) return
                val notice = "对手「${message.username}」已开始匹配，请点击「开始游戏」加入"
                _ui.update { it.copy(matchNotice = notice, status = notice) }
                appendLog(notice)
            }

            is ServerMessage.MatchmakingCancelled -> _ui.update {
                it.copy(matching = false, matchNotice = "", status = "", message = "")
            }

            ServerMessage.GameStart -> {
                _ui.update {
                    it.copy(screen = Screen.Game, matching = false, status = "游戏开始！")
                }
                appendLog("游戏开始！")
            }

            is ServerMessage.GameEnd -> {
                val text = when {
                    !message.hasWinner -> "游戏结束，无人获胜！"
                    message.winnerName.isNotBlank() ->
                        "游戏结束，${message.winnerName}（${message.winnerId}）获胜！"
                    else -> "游戏结束，${message.winnerId} 获胜！"
                }
                _ui.update { it.copy(game = it.game.copy(resultText = text)) }
                appendLog(text)
            }

            is ServerMessage.State -> _ui.update { state ->
                val maxIndex = state.game.state?.players
                    ?.firstOrNull { it.id == state.playerId }
                    ?.hand?.cards?.size?.minus(1) ?: -1
                val selected = if (maxIndex < 0) 0 else state.game.selectedIndex.coerceIn(0, maxIndex)
                state.copy(game = state.game.copy(state = message.state, selectedIndex = selected))
            }

            is ServerMessage.PointerUpdate -> _ui.update { state ->
                if (message.playerId != state.playerId) return@update state
                state.copy(game = state.game.copy(selectedIndex = message.selectedIndex.toInt()))
            }

            is ServerMessage.CharInfo -> _ui.update { state ->
                val updated = HashMap(state.game.charInfo)
                updated[message.playerIndex.toInt()] = message.fullText
                state.copy(game = state.game.copy(charInfo = updated))
            }

            is ServerMessage.Choice -> {
                val prompt = message.prompt
                val empty = prompt.title.isEmpty() && prompt.options.isEmpty()
                _ui.update { state ->
                    val previous = state.game.choice
                    val restarted = previous == null ||
                        previous.title != prompt.title ||
                        previous.forced != prompt.forced ||
                        previous.totalPages != prompt.totalPages
                    state.copy(
                        game = state.game.copy(
                            choice = if (empty) null else prompt,
                            choiceStartedAt = if (restarted) System.currentTimeMillis()
                            else state.game.choiceStartedAt
                        )
                    )
                }
            }

            is ServerMessage.StringPayload -> when (message.type) {
                Msg.SHOP_DATA -> onShopData(message.json)
                Msg.SHOP_RESULT -> onShopResult(message.json)
                Msg.SKILL_INFO_RESPONSE -> onSkillInfo(message.json)
            }

            is ServerMessage.Heartbeat -> Unit

            ServerMessage.Disconnected -> appendLog("与服务器断开连接")

            is ServerMessage.Unknown -> Unit
        }
    }

    private fun onAccountResponse(message: ServerMessage.AccountResponse) {
        when (message.type) {
            Msg.REGISTER_RESPONSE -> {
                if (message.ok) {
                    _ui.update { it.copy(message = "注册成功，自动登录中...") }
                    val s = _ui.value
                    connection.send(ClientMessage.Login(s.username, s.password))
                } else {
                    _ui.update { it.copy(message = "注册失败: ${message.msg}") }
                }
            }

            Msg.LOGIN_RESPONSE -> {
                if (message.ok) {
                    _ui.update {
                        it.copy(
                            loggedIn = true,
                            screen = Screen.Lobby,
                            points = message.points,
                            wins = message.wins,
                            losses = message.losses,
                            message = "登录成功",
                            shop = it.shop.copy(points = message.points)
                        )
                    }
                    appendLog("登录成功: ${_ui.value.username} 积分=${message.points} 胜=${message.wins} 负=${message.losses}")
                } else {
                    _ui.update { it.copy(message = "登录失败: ${message.msg}") }
                }
            }
        }
    }

    private fun onShopData(json: String) {
        try {
            val snapshot = ShopParser.parseShopData(json)
            _ui.update { state ->
                state.copy(
                    points = snapshot.points,
                    shop = state.shop.copy(
                        dataReady = true,
                        points = snapshot.points,
                        heroes = snapshot.heroes,
                        items = snapshot.items,
                        message = ""
                    )
                )
            }
            val known = _ui.value.shop.skillInfo.keys.toSet()
            val missing = _ui.value.shop.heroes.map { it.name }.filter { it !in known }
            requestSkillInfo(missing)
        } catch (e: Exception) {
            setShopMessage("商城数据解析失败")
        }
    }

    private fun onShopResult(json: String) {
        try {
            val result = ShopParser.parseResult(json)
            _ui.update { state ->
                state.copy(
                    points = result.points,
                    shop = state.shop.copy(
                        points = result.points,
                        message = if (result.ok) "购买成功" else result.msg.ifBlank { "积分不足！" }
                    )
                )
            }
            if (result.ok) refreshShop()
        } catch (e: Exception) {
            setShopMessage("购买结果解析失败")
        }
    }

    private fun onSkillInfo(json: String) {
        try {
            val skills = ShopParser.parseSkills(json)
            _ui.update { state -> state.copy(shop = state.shop.copy(skillInfo = state.shop.skillInfo + skills)) }
        } catch (e: Exception) {
            // 技能描述解析失败只影响展示，不打断流程
        }
    }

    private fun appendLog(line: String) = _ui.update { state ->
        state.copy(game = state.game.copy(log = (state.game.log + line).takeLast(40)))
    }
}
