package com.unool.android.ui

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

@Composable
fun UnoolTheme(content: @Composable () -> Unit) {
    val colors = lightColorScheme(
        primary = Color(0xFF2E6FBF),
        onPrimary = Color.White,
        secondary = Color(0xFFE8F1FF),
        background = Color.White,
        surface = Color(0xFFF6F8FB),
        error = Color(0xFFB3261E)
    )
    MaterialTheme(
        colorScheme = if (isSystemInDarkTheme()) colors.copy(
            background = Color(0xFF121212),
            surface = Color(0xFF1E1E1E)
        ) else colors,
        content = content
    )
}
