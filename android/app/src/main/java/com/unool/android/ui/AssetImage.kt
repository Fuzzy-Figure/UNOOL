package com.unool.android.ui

import android.content.Context
import android.graphics.BitmapFactory
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.produceState
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.graphics.painter.BitmapPainter
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import android.util.LruCache
import com.unool.android.net.MissingImageReporter

private val bitmapCache = object : LruCache<String, androidx.compose.ui.graphics.ImageBitmap>(32 * 1024) {
    override fun sizeOf(key: String, value: androidx.compose.ui.graphics.ImageBitmap): Int =
        value.width * value.height * 4 / 1024
}

private suspend fun loadAsset(context: Context, path: String, maxDimension: Int): androidx.compose.ui.graphics.ImageBitmap? =
    withContext(Dispatchers.IO) {
        val key = "$path|$maxDimension"
        bitmapCache.get(key)?.let { return@withContext it }

        val bytes = try {
            context.assets.open(path).use { it.readBytes() }
        } catch (e: Exception) {
            MissingImageReporter.report(path)
            return@withContext null
        }

        val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        BitmapFactory.decodeByteArray(bytes, 0, bytes.size, bounds)
        if (bounds.outWidth <= 0 || bounds.outHeight <= 0) {
            MissingImageReporter.report(path)
            return@withContext null
        }

        var sample = 1
        while (maxOf(bounds.outWidth, bounds.outHeight) / sample > maxDimension) sample *= 2

        val options = BitmapFactory.Options().apply {
            inSampleSize = sample
            inPreferredConfig = android.graphics.Bitmap.Config.RGB_565
        }
        val bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.size, options) ?: run {
            MissingImageReporter.report(path)
            return@withContext null
        }
        val image = bitmap.asImageBitmap()
        bitmapCache.put(key, image)
        image
    }

/** 从 APK assets 按相对路径加载图片（与 PC 端 images/ 目录结构一致）。 */
@Composable
fun AssetImage(
    path: String,
    contentDescription: String? = null,
    modifier: Modifier = Modifier,
    maxDimension: Int = 512,
    contentScale: ContentScale = ContentScale.Fit
) {
    val context = LocalContext.current
    val image by produceState<androidx.compose.ui.graphics.ImageBitmap?>(
        initialValue = null,
        path,
        maxDimension
    ) {
        value = loadAsset(context, path, maxDimension)
    }

    if (image != null) {
        Image(
            painter = BitmapPainter(image!!),
            contentDescription = contentDescription,
            modifier = modifier,
            contentScale = contentScale
        )
    } else {
        Box(modifier.background(Color(0xFFE8E8E8)))
    }
}

/** 卡牌缩略图（卡牌原图 180x270，这里留出足够分辨率）。 */
@Composable
fun CardImage(path: String, modifier: Modifier = Modifier) {
    AssetImage(
        path = path,
        contentDescription = null,
        modifier = modifier.size(width = 66.dp, height = 99.dp),
        maxDimension = 256
    )
}
