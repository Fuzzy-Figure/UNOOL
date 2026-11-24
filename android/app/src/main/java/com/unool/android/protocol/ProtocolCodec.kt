package com.unool.android.protocol

import org.json.JSONArray
import org.json.JSONObject

/** 与 PC 端 source/protocol 逐字段对应的编解码器（SFML 3.1 大端线格式）。 */
object ProtocolCodec {

    fun encode(message: ClientMessage): ByteArray {
        val w = PacketWriter()
        when (message) {
            is ClientMessage.Login -> {
                w.i32(Msg.LOGIN_REQUEST)
                w.u32(Msg.PROTOCOL_VERSION)
                w.str(message.username)
                w.str(message.password)
            }

            is ClientMessage.Register -> {
                w.i32(Msg.REGISTER_REQUEST)
                w.u32(Msg.PROTOCOL_VERSION)
                w.str(message.username)
                w.str(message.password)
            }

            is ClientMessage.CheckUsername -> {
                w.i32(Msg.CHECK_USERNAME_REQUEST)
                w.str(message.username)
            }

            ClientMessage.Checkin -> w.i32(Msg.CHECKIN_REQUEST)

            is ClientMessage.ShopPurchase -> {
                w.i32(Msg.SHOP_PURCHASE)
                w.str(message.json)
            }

            is ClientMessage.SkillInfoRequest -> {
                w.i32(Msg.SKILL_INFO_REQUEST)
                w.str(message.json)
            }

            is ClientMessage.ClientInput -> {
                w.i32(Msg.CLIENT_INPUT)
                w.i32(message.key)
                w.u64(message.playerId)
                w.u64(message.selectedIndex)
            }

            // 房间（协议版本 3）：列表无载荷，创建带房间名，加入带房间号，准备带布尔
            ClientMessage.RoomListRequest -> w.i32(Msg.ROOM_LIST_REQUEST)
            is ClientMessage.RoomCreate -> {
                w.i32(Msg.ROOM_CREATE_REQUEST)
                w.str(message.roomName)
            }
            is ClientMessage.RoomJoin -> {
                w.i32(Msg.ROOM_JOIN_REQUEST)
                w.u64(message.roomId)
            }
            ClientMessage.RoomLeave -> w.i32(Msg.ROOM_LEAVE_REQUEST)
            is ClientMessage.RoomReady -> {
                w.i32(Msg.ROOM_READY_REQUEST)
                w.bool(message.ready)
            }

            ClientMessage.Heartbeat -> w.i32(Msg.HEARTBEAT)
        }
        return w.frame()
    }

    fun decode(payload: ByteArray): ServerMessage {
        val r = PacketReader(payload)
        return when (val type = r.i32()) {
            Msg.CONNECTION_INFO -> ServerMessage.ConnectionInfo(r.u64())
            Msg.CONNECTION_REFUSED -> ServerMessage.ConnectionRefused

            Msg.REGISTER_RESPONSE, Msg.LOGIN_RESPONSE -> ServerMessage.AccountResponse(
                type = type,
                ok = r.bool(),
                msg = r.str(),
                points = r.i32(),
                wins = r.i32(),
                losses = r.i32()
            )

            Msg.CHECK_USERNAME_RESPONSE -> ServerMessage.CheckUsernameResponse(r.bool())

            // 房间模式：开局包携带本局座位号（0/1），与连接槽位号无关
            Msg.GAME_START -> ServerMessage.GameStart(r.u64())

            Msg.GAME_END -> {
                val hasWinner = r.bool()
                val winnerId = if (hasWinner) r.u64() else 0L
                val winnerName = if (hasWinner) r.str() else ""
                ServerMessage.GameEnd(hasWinner, winnerId, winnerName)
            }

            Msg.GAME_STATE -> ServerMessage.State(readGameState(r))

            Msg.CHOICE -> {
                val title = r.str()
                val optionCount = r.count()
                val options = List(optionCount) { r.str() }
                val forced = r.bool()
                val errorMsg = r.str()
                val hasTimeout = r.bool()
                val timeoutMs = if (hasTimeout) r.u64() else null
                val currentPage = r.u64()
                val totalPages = r.u64()
                val metaCount = r.count()
                val metas = List(metaCount) {
                    ChoiceMeta(r.str(), r.str(), r.str(), r.str(), r.str())
                }
                ServerMessage.Choice(
                    ChoicePrompt(title, options, forced, errorMsg, timeoutMs, currentPage, totalPages, metas)
                )
            }

            Msg.POINTER_UPDATE -> ServerMessage.PointerUpdate(r.u64(), r.u64())

            Msg.CHAR_INFO -> {
                val playerIndex = r.u64()
                val fullText = r.str()
                ServerMessage.CharInfo(playerIndex, fullText)
            }

            Msg.SHOP_DATA, Msg.SHOP_RESULT, Msg.SKILL_INFO_RESPONSE ->
                ServerMessage.StringPayload(type, r.str())

            Msg.CHECKIN_RESPONSE -> {
                // 注意：本包的字段顺序与 Register/LoginResponse 不同：msg, ok, points
                val msg = r.str()
                val ok = r.bool()
                val points = r.i32()
                ServerMessage.CheckinResponse(msg, ok, points)
            }

            // 房间事件：消息号 + JSON 字符串载荷（与商城载荷同构）
            Msg.ROOM_LIST_RESPONSE -> ServerMessage.RoomList(parseRoomList(r.str()))
            Msg.ROOM_STATE_RESPONSE -> ServerMessage.RoomState(parseRoomSnapshot(r.str()))
            Msg.ROOM_ERROR_RESPONSE -> ServerMessage.RoomError(parseRoomError(r.str()))

            Msg.HEARTBEAT -> ServerMessage.Heartbeat

            else -> ServerMessage.Unknown(type)
        }
    }

    // ---------------------------------------------------------------- 房间 JSON

    // 解析失败一律退化为「空列表 / 空快照」，不让一个坏包把整条连接打断
    private fun parseRoomList(json: String): List<RoomSummary> = try {
        val root = JSONObject(json)
        val array: JSONArray = root.optJSONArray("rooms") ?: JSONArray()
        List(array.length()) { index ->
            val item = array.getJSONObject(index)
            RoomSummary(
                id = item.optLong("id"),
                name = item.optString("name"),
                owner = item.optString("owner"),
                members = item.optInt("members"),
                capacity = item.optInt("capacity", Msg.ROOM_CAPACITY),
                inGame = item.optBoolean("inGame")
            )
        }
    } catch (_: Exception) {
        emptyList()
    }

    private fun parseRoomSnapshot(json: String): RoomSnapshot = try {
        val item = JSONObject(json)
        val array: JSONArray = item.optJSONArray("members") ?: JSONArray()
        val members = List(array.length()) { index ->
            val member = array.getJSONObject(index)
            RoomMember(
                name = member.optString("name"),
                ready = member.optBoolean("ready"),
                owner = member.optBoolean("owner")
            )
        }
        RoomSnapshot(
            id = item.optLong("id"),
            name = item.optString("name"),
            owner = item.optString("owner"),
            capacity = item.optInt("capacity", Msg.ROOM_CAPACITY),
            inGame = item.optBoolean("inGame"),
            closed = item.optBoolean("closed"),
            reason = item.optString("reason"),
            members = members
        )
    } catch (_: Exception) {
        // 解析失败当成「房间已关闭」处理，客户端会稳定回到大厅
        RoomSnapshot(0, "", "", Msg.ROOM_CAPACITY, false, true, "房间状态解析失败", emptyList())
    }

    private fun parseRoomError(json: String): String {
        return try {
            val msg = JSONObject(json).optString("msg")
            if (msg.isBlank()) "房间操作失败" else msg
        } catch (_: Exception) {
            if (json.isBlank()) "房间操作失败" else json
        }
    }

    // ---------------------------------------------------------------- 对局状态

    private fun readCard(r: PacketReader): CardData {
        val color = r.i32()
        val name = r.i32()
        val reason = r.i32()
        return CardData(color, name, reason)
    }

    private fun readPlayer(r: PacketReader): PlayerStateData {
        val id = r.u64()
        val hp = r.i32()
        val maxHp = r.i32()
        val shield = r.u64()
        val characterCount = r.count()
        val characterNames = List(characterCount) { r.str() }
        val skinCount = r.count()
        val skins = List(skinCount) { r.str() }

        val cardCount = r.count()
        val cards = List(cardCount) { readCard(r) }
        val rawSelected = r.u64()
        val selectedIndex = if (cards.isEmpty()) 0L else rawSelected.coerceIn(0L, cards.size - 1L)

        val markCount = r.count()
        val marks = HashMap<String, Long>(markCount)
        repeat(markCount) { marks[r.str()] = r.u64() }

        val wins = r.u64()
        val losses = r.u64()
        val pathCount = r.count()
        val paths = List(pathCount) { r.str() }

        return PlayerStateData(
            id, hp, maxHp, shield, characterNames, skins,
            HandData(cards, selectedIndex.toInt()), marks, wins, losses, paths
        )
    }

    private fun readGameState(r: PacketReader): GameStateData {
        val currentPlayerIndex = r.u64()
        val currentColor = r.i32()
        val currentName = r.i32()
        val direction = r.i32()
        val playerCount = r.count()
        val players = List(playerCount) { readPlayer(r) }

        val discardCount = r.count()
        val discards = List(discardCount) { readCard(r) }

        val seatCount = r.count()
        val seatOrder = List(seatCount) { r.u64() }

        val hasOperating = r.bool()
        val operatingPlayerId = if (hasOperating) r.u64() else null
        val gameCount = r.u64()

        //可用主动技列表：服务端追加在包尾（数字键1..顺序）
        val skillCount = r.count()
        val availableSkills = List(skillCount) { r.str() }

        return GameStateData(
            currentPlayerIndex, currentColor, currentName, direction,
            players, discards, seatOrder, operatingPlayerId, gameCount, availableSkills
        )
    }
}
