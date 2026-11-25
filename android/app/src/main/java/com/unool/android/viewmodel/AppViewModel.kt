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
import com.unool.android.protocol.RoomSnapshot
import com.unool.android.protocol.RoomSummary
import com.unool.android.protocol.ServerMessage
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

// 房间功能（协议版本 3）：登录后不再匹配，改为
// Login → Lobby（开始游戏 / 商城 / 签到）→ Rooms（房间列表）→ Room（房内准备）→ Game
enum class Screen { Login, Lobby, Rooms, Room, Shop, Game }
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
    val checkinPopup: String? = null,
    val playerId: Long = 0L,
    // === 房间 ===
    val roomNameInput: String = "",
    val rooms: List<RoomSummary> = emptyList(),
    val selectedRoom: Int = 0,
    val room: RoomSnapshot? = null,
    val shop: ShopUiState = ShopUiState(),
    val game: GameUiState = GameUiState()
) {
    /** 自己在房间内是否已准备 */
    val selfReady: Boolean
        get() = room?.members?.any { it.name == username && it.ready } == true

    /** 自己是否是房主 */
    val isOwner: Boolean
        get() = room != null && room.owner == username
}

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
        // 启动即按当前地址连接；登录页可修改 IP / 端口后重新连接。
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
                screen = Screen.Login,
                room = null,
                rooms = emptyList(),
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
        if (s.connection !is ConnectionState.Connected) {
            _ui.update { it.copy(message = "请先连接服务器") }
            return
        }
        if (s.username.isBlank() || s.password.isBlank()) {
            _ui.update { it.copy(message = "用户名和密码不能为空") }
            return
        }
        _ui.update { it.copy(message = "登录中...") }
        connection.send(ClientMessage.Login(s.username, s.password))
    }

    fun register() {
        val s = _ui.value
        if (s.connection !is ConnectionState.Connected) {
            _ui.update { it.copy(message = "请先连接服务器") }
            return
        }
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

    // ---------------------------------------------------------------- 房间

    /** 主大厅「开始游戏」：进入房间界面并拉一次房间列表 */
    fun startGame() {
        if (!_ui.value.loggedIn) return
        _ui.update { it.copy(screen = Screen.Rooms, message = "", status = "") }
        connection.send(ClientMessage.RoomListRequest)
    }

    /** 房间界面「返回大厅」 */
    fun backToLobby() {
        _ui.update { it.copy(screen = Screen.Lobby, message = "", status = "") }
    }

    fun setRoomName(value: String) = _ui.update { it.copy(roomNameInput = value) }

    fun refreshRooms() {
        if (!_ui.value.loggedIn) return
        connection.send(ClientMessage.RoomListRequest)
    }

    fun selectRoom(index: Int) = _ui.update { it.copy(selectedRoom = index) }

    /** 创建房间：房间名留空时以「账号的房间」命名（服务端同样兜底） */
    fun createRoom() {
        val s = _ui.value
        if (!s.loggedIn) return
        val name = s.roomNameInput.ifBlank { "${s.username}的房间" }
        _ui.update { it.copy(message = "正在创建房间...") }
        connection.send(ClientMessage.RoomCreate(name))
    }

    fun joinRoom(index: Int) {
        val s = _ui.value
        if (!s.loggedIn) return
        val room = s.rooms.getOrNull(index) ?: return
        if (room.inGame) {
            _ui.update { it.copy(message = "该房间正在对局中，无法加入") }
            return
        }
        if (room.members >= room.capacity) {
            _ui.update { it.copy(message = "该房间已满") }
            return
        }
        _ui.update { it.copy(selectedRoom = index, message = "正在加入房间...") }
        connection.send(ClientMessage.RoomJoin(room.id))
    }

    fun leaveRoom() {
        // 离开房间回到「房间界面」（列表），不是主大厅
        _ui.update { it.copy(room = null, screen = Screen.Rooms, message = "", status = "") }
        connection.send(ClientMessage.RoomLeave)
        connection.send(ClientMessage.RoomListRequest)
    }

    fun toggleReady() {
        val s = _ui.value
        if (s.room == null || s.room.inGame) return
        connection.send(ClientMessage.RoomReady(!s.selfReady))
    }

    fun openShop() {
        _ui.update { it.copy(screen = Screen.Shop, shop = ShopUiState()) }
        connection.send(
            ClientMessage.ShopPurchase(ShopParser.purchaseJson("request", "", "", ""))
        )
    }

    fun exitShop() {
        _ui.update { it.copy(screen = Screen.Lobby) }
        // 商城期间房间列表可能变了：回大厅立刻重拉一次
        refreshRooms()
    }

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

    //发动第 index 个主动技（1-based），与服务端可用技能列表顺序一致
    fun useSkill(index: Int) = sendKey(Key.digit(index))
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

            is ServerMessage.RoomList -> _ui.update { state ->
                val selected = if (message.rooms.isEmpty()) 0
                else state.selectedRoom.coerceIn(0, message.rooms.size - 1)
                state.copy(rooms = message.rooms, selectedRoom = selected)
            }

            is ServerMessage.RoomState -> {
                val room = message.room
                if (room.closed) {
                    // 房间解散 / 已离开：回到房间界面（列表）并提示原因
                    _ui.update {
                        it.copy(
                            room = null,
                            screen = Screen.Rooms,
                            message = room.reason.ifBlank { "已离开房间" },
                            status = ""
                        )
                    }
                } else {
                    _ui.update {
                        // 房间页 / 对局页由快照里的 inGame 决定：开局快照（inGame=true）可能在
                        // GameStart 之后到达，不能无条件把界面按回房间页；对局结束（inGame=false）
                        // 时再退回房间页等下一局。
                        it.copy(
                            room = room,
                            screen = if (room.inGame) Screen.Game else Screen.Room,
                            message = ""
                        )
                    }
                }
            }

            is ServerMessage.RoomError -> _ui.update {
                it.copy(message = message.msg, status = "")
            }

            // 被顶下线：不当作普通断线，提示原因并退回登录界面
            is ServerMessage.SessionReplaced -> {
                appendLog(message.reason)
                connection.disconnect()
                _ui.update {
                    it.copy(
                        screen = Screen.Login,
                        loggedIn = false,
                        room = null,
                        rooms = emptyList(),
                        message = message.reason,
                        status = ""
                    )
                }
            }

            is ServerMessage.GameStart -> {
                // 房间模式：开局包携带本局座位号（0/1），对局内所有玩家索引都用座位号
                _ui.update {
                    it.copy(
                        screen = Screen.Game,
                        playerId = message.seat,
                        room = null,
                        status = "游戏开始！"
                    )
                }
                appendLog("游戏开始！座位 ${message.seat}")
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
                // 对局状态只在对局中存在：收到它就说明对局已经开始，兜底切到对局页，
                // 避免「漏掉 GameStart 后一直停在房间页」时没有任何自救路径。
                state.copy(
                    screen = Screen.Game,
                    game = state.game.copy(state = message.state, selectedIndex = selected)
                )
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
                    // 主大厅只有按钮：房间列表在点「开始游戏」进入房间界面时才拉
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
