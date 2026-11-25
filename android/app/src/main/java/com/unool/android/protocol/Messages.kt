package com.unool.android.protocol

object Msg {
    const val NONE = 0

    // 客户端 -> 服务端
    const val CLIENT_INPUT = 101
    const val REGISTER_REQUEST = 102
    const val LOGIN_REQUEST = 103
    const val CHECK_USERNAME_REQUEST = 104
    const val SHOP_PURCHASE = 107
    const val SKILL_INFO_REQUEST = 108
    const val CHECKIN_REQUEST = 109
    // 房间功能（协议版本 3）：登录后不再匹配，改为房间组队
    const val ROOM_LIST_REQUEST = 110
    const val ROOM_CREATE_REQUEST = 111
    const val ROOM_JOIN_REQUEST = 112
    const val ROOM_LEAVE_REQUEST = 113
    const val ROOM_READY_REQUEST = 114
    /** 客户端本地图片加载失败，携带相对路径；服务端只打印警告 */
    const val IMAGE_MISSING_WARNING = 115
    /** 用户确认后强制登录；服务端收到时才顶掉旧设备 */
    const val FORCE_LOGIN_REQUEST = 116
    /** 客户端按真实选项序号选择（1-based，0=取消） */
    const val CHOICE_SELECT = 117

    // 服务端 -> 客户端
    const val CONNECTION_INFO = 201
    const val CONNECTION_REFUSED = 202
    const val REGISTER_RESPONSE = 203
    const val LOGIN_RESPONSE = 204
    const val CHECK_USERNAME_RESPONSE = 205
    const val GAME_START = 206
    const val GAME_END = 207
    const val GAME_STATE = 208
    const val CHOICE = 209
    const val POINTER_UPDATE = 210
    const val CHAR_INFO = 211
    const val SHOP_DATA = 212
    const val SHOP_RESULT = 213
    const val SKILL_INFO_RESPONSE = 214
    const val CHECKIN_RESPONSE = 215
    const val ROOM_LIST_RESPONSE = 218
    const val ROOM_STATE_RESPONSE = 219
    const val ROOM_ERROR_RESPONSE = 220
    /** 同账号在另一端登录，本端被顶下线（包体只有一个原因串） */
    const val SESSION_REPLACED = 221

    const val HEARTBEAT = 901
    const val PROTOCOL_VERSION = 3L

    // 房间容量：一桌两个座位
    const val ROOM_CAPACITY = 2
}

/** PC 端 sf::Keyboard::Scancode 数值，必须原样发给服务端 */
object Key {
    const val A = 0
    const val B = 1
    const val D = 3
    const val E = 4
    const val H = 7
    const val S = 18
    const val W = 22
    const val NUM_1 = 26
    const val NUM_9 = 34
    const val NUM_0 = 35
    const val ENTER = 36
    const val ESCAPE = 37
    const val BACKSPACE = 38
    const val TAB = 39
    const val SPACE = 40
    const val UP = 89
    const val DOWN = 88
    const val LEFT = 87
    const val RIGHT = 86

    fun digit(index: Int): Int = NUM_1 + index - 1 // 1..9
}

data class CardData(val color: Int, val name: Int, val discardReason: Int)

data class HandData(val cards: List<CardData>, val selectedIndex: Int)

data class PlayerStateData(
    val id: Long,
    val hp: Int,
    val maxHp: Int,
    val shield: Long,
    val characterNames: List<String>,
    val skins: List<String>,
    val hand: HandData,
    val marks: Map<String, Long>,
    val wins: Long,
    val losses: Long,
    val characterImagePaths: List<String>
)

data class GameStateData(
    val currentPlayerIndex: Long,
    val currentColor: Int,
    val currentName: Int,
    val direction: Int,
    val players: List<PlayerStateData>,
    val discards: List<CardData>,
    val seatOrder: List<Long>,
    val operatingPlayerId: Long?,
    val gameCount: Long,
    val availableSkills: List<String> = emptyList()
)

data class ChoiceMeta(
    val characterName: String,
    val level: String,
    val hp: String,
    val skinName: String,
    val imagePath: String
)

data class ChoicePrompt(
    val title: String,
    val options: List<String>,
    val forced: Boolean,
    val errorMsg: String,
    val timeoutMs: Long?,
    val currentPage: Long,
    val totalPages: Long,
    val optionMeta: List<ChoiceMeta>
)

// ---------------------------------------------------------------- 房间

/** 大厅房间列表里的一行 */
data class RoomSummary(
    val id: Long,
    val name: String,
    val owner: String,
    val members: Int,
    val capacity: Int,
    val inGame: Boolean
)

/** 房间内的一名成员 */
data class RoomMember(val name: String, val ready: Boolean, val owner: Boolean)

/** 当前所在房间的完整快照；closed=true 表示已离开 / 已解散 */
data class RoomSnapshot(
    val id: Long,
    val name: String,
    val owner: String,
    val capacity: Int,
    val inGame: Boolean,
    val closed: Boolean,
    val reason: String,
    val members: List<RoomMember>
)

sealed interface ClientMessage {
    data class Login(val username: String, val password: String) : ClientMessage
    data class Register(val username: String, val password: String) : ClientMessage
    data class CheckUsername(val username: String) : ClientMessage
    data object Checkin : ClientMessage
    data class ShopPurchase(val json: String) : ClientMessage
    data class SkillInfoRequest(val json: String) : ClientMessage
    data class ClientInput(val key: Int, val playerId: Long, val selectedIndex: Long) : ClientMessage
    data object RoomListRequest : ClientMessage
    data class RoomCreate(val roomName: String) : ClientMessage
    data class RoomJoin(val roomId: Long) : ClientMessage
    data object RoomLeave : ClientMessage
    data class RoomReady(val ready: Boolean) : ClientMessage
    data class ImageMissingWarning(val path: String) : ClientMessage
    data class ForceLogin(val username: String, val password: String) : ClientMessage
    data class ChoiceSelect(val choiceIndex: Long, val selectedIndex: Long) : ClientMessage
    data object Heartbeat : ClientMessage
}

sealed interface ServerMessage {
    data class ConnectionInfo(val playerId: Long) : ServerMessage
    data object ConnectionRefused : ServerMessage
    data class AccountResponse(val type: Int, val ok: Boolean, val msg: String, val points: Int, val wins: Int, val losses: Int) : ServerMessage
    data class CheckUsernameResponse(val exists: Boolean) : ServerMessage
    /** seat：本局座位号（0/1），房间模式下由服务端指定 */
    data class GameStart(val seat: Long) : ServerMessage
    data class GameEnd(val hasWinner: Boolean, val winnerId: Long, val winnerName: String) : ServerMessage
    data class State(val state: GameStateData) : ServerMessage
    data class Choice(val prompt: ChoicePrompt) : ServerMessage
    data class PointerUpdate(val playerId: Long, val selectedIndex: Long) : ServerMessage
    data class CharInfo(val playerIndex: Long, val fullText: String) : ServerMessage
    data class StringPayload(val type: Int, val json: String) : ServerMessage
    data class CheckinResponse(val msg: String, val ok: Boolean, val points: Int) : ServerMessage
    data class RoomList(val rooms: List<RoomSummary>) : ServerMessage
    data class RoomState(val room: RoomSnapshot) : ServerMessage
    data class RoomError(val msg: String) : ServerMessage
    /** 被顶下线：提示原因后回登录界面，不当作普通断线处理 */
    data class SessionReplaced(val reason: String) : ServerMessage
    data object Heartbeat : ServerMessage
    data class Unknown(val type: Int) : ServerMessage
    data object Disconnected : ServerMessage
}
