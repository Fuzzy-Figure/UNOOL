package com.unool.android.model

import com.unool.android.protocol.CardData

/** 与 header/core/Card.h 的枚举数值保持一致（不要重排，线上序列化依赖这些值）。 */
object CardColor {
    const val NO = 0
    const val BLUE = 1
    const val GREEN = 2
    const val RED = 3
    const val YELLOW = 4
    const val BLACK = 5
    const val PURPLE = 6

    fun label(value: Int): String = when (value) {
        BLUE -> "蓝"
        GREEN -> "绿"
        RED -> "红"
        YELLOW -> "黄"
        BLACK -> "黑"
        PURPLE -> "紫"
        NO -> "无"
        else -> ""
    }
}

object CardName {
    const val NO = 0
    const val NUMBER_0 = 1
    const val NUMBER_9 = 10
    const val ACTION_REV = 11
    const val ACTION_SKIP = 12
    const val ACTION_DRAW2 = 13
    const val WILD_PAL = 14
    const val WILD_DRAW4 = 15
    const val BACK = 16
    const val SECRET_毒酒 = 17
    const val SECRET_药酒 = 18
    const val SECRET_过载 = 19
    const val SECRET_指虎 = 20

    fun label(value: Int): String = when (value) {
        in NUMBER_0..NUMBER_9 -> (value - NUMBER_0).toString()
        ACTION_REV -> "反转"
        ACTION_SKIP -> "封禁"
        ACTION_DRAW2 -> "+2"
        WILD_PAL -> "变色"
        WILD_DRAW4 -> "+4"
        BACK -> "背面"
        SECRET_毒酒 -> "毒酒"
        SECRET_药酒 -> "药酒"
        SECRET_过载 -> "过载"
        SECRET_指虎 -> "指虎"
        NO -> "无"
        else -> "未知"
    }
}

object DiscardReason {
    const val NONE = 0
    const val USE = 1
    const val DISCARD = 2
    const val RECAST = 3
    const val DECREE = 4
    const val JUDGE = 5

    fun label(value: Int): String = when (value) {
        USE -> "打出"
        DISCARD -> "弃置"
        RECAST -> "重铸"
        DECREE -> "决议"
        JUDGE -> "判定"
        else -> ""
    }
}

/** 对应 source/core/Card.cpp 的 imagePaths 表与 value()。 */
object CardCatalog {
    fun imagePath(card: CardData): String {
        val color = card.color
        val name = card.name
        val colorFolder = when (color) {
            CardColor.BLUE -> "blue"
            CardColor.GREEN -> "green"
            CardColor.RED -> "red"
            CardColor.YELLOW -> "yellow"
            CardColor.BLACK -> "black"
            CardColor.PURPLE -> "purple"
            else -> ""
        }

        return when {
            name in CardName.NUMBER_0..CardName.ACTION_DRAW2 && colorFolder.isNotEmpty() -> {
                val file = when {
                    name <= CardName.NUMBER_9 -> "number_${name - CardName.NUMBER_0}"
                    name == CardName.ACTION_REV -> "action_rev"
                    name == CardName.ACTION_SKIP -> "action_skip"
                    else -> "action_draw2"
                }
                "images/cards/$colorFolder/$file.jpg"
            }

            name == CardName.WILD_PAL -> "images/cards/black/wild_pal.jpg"
            name == CardName.WILD_DRAW4 -> "images/cards/black/wild_draw4.jpg"
            name == CardName.BACK -> "images/cards/back.jpg"

            name == CardName.SECRET_毒酒 -> "images/cards/secret/毒酒.jpg"
            name == CardName.SECRET_药酒 -> "images/cards/secret/药酒.jpg"
            name == CardName.SECRET_过载 -> "images/cards/secret/过载.jpg"
            name == CardName.SECRET_指虎 -> "images/cards/secret/指虎.jpg"

            else -> "images/default.jpg"
        }
    }

    fun value(card: CardData): Int = when (card.name) {
        in CardName.NUMBER_0..CardName.NUMBER_9 -> card.name - CardName.NUMBER_0
        CardName.ACTION_REV, CardName.ACTION_SKIP, CardName.ACTION_DRAW2 -> 20
        CardName.WILD_PAL, CardName.WILD_DRAW4 -> 50
        CardName.SECRET_毒酒, CardName.SECRET_药酒,
        CardName.SECRET_过载, CardName.SECRET_指虎 -> 10
        else -> 0
    }

    fun label(card: CardData): String =
        CardColor.label(card.color) + CardName.label(card.name)

    const val POINTER_IMAGE = "images/cards/pointer/默认.jpg"
    const val GHOST_IMAGE = "images/marks/幽灵.jpg"
    const val DEFAULT_IMAGE = "images/default.jpg"
}
