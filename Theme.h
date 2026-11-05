#pragma once

#include <QColor>
#include <QtGlobal>

// =============================================================================
// Theme.h
// 全局主题常量：界面配色、拖拽数据格式、黄金分割比例。
// 所有绘制模块（棋盘、预览栏、计分板、主窗口）都从这里取色，
// 保证整体视觉风格统一为「海洋 + 气泡」。
// =============================================================================
namespace Theme
{
// 拖拽气泡使用的 MIME 类型：预览栏投递，棋盘接收。
static const char kMimeBubble[] = "application/x-ocean-bubble";

// 黄金分割比例（约 1.618），用于窗口与右侧栏内部的比例划分。
static const qreal kGoldenRatio = 1.6180339887498948;

// 文本颜色
static const QColor kTextPrimary(236, 251, 255);      // 主文字：接近白色的浅蓝
static const QColor kTextSecondary(168, 220, 245);    // 次要文字：柔和的冰蓝

// 玻璃卡片配色（计分板 / 预览栏共用）
static const QColor kCardTop(8, 44, 96, 235);         // 卡片顶部：深海蓝
static const QColor kCardBottom(12, 86, 146, 225);    // 卡片底部：更亮的海蓝
static const QColor kCardBorder(140, 225, 255, 120);  // 卡片描边：带透明的亮蓝
} // namespace Theme
