package com.unool.android.protocol

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

            ClientMessage.ReadyToPlay -> w.i32(Msg.READY_TO_PLAY)
            ClientMessage.CancelMatchmaking -> w.i32(Msg.CANCEL_MATCHMAKING)
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

            Msg.GAME_START -> ServerMessage.GameStart

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

            Msg.MATCHMAKING_CANCELLED -> ServerMessage.MatchmakingCancelled(r.str())
            Msg.MATCHMAKING_STARTED -> ServerMessage.MatchmakingStarted(r.str())

            Msg.HEARTBEAT -> ServerMessage.Heartbeat

            else -> ServerMessage.Unknown(type)
        }
    }

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
