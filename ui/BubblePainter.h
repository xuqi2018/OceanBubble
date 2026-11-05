#pragma once

#include <QPixmap>
#include <QPolygonF>

class QPainter;

// =============================================================================
// BubblePainter.h
// 统一的「海洋气泡」绘制工具，供棋盘、预览栏、拖拽图标复用：
//   * drawBubble   —— 绘制带渐变、波光、数字的气泡；
//   * drawHexSlot  —— 绘制棋盘上的六角凹槽；
//   * renderBubble —— 把气泡渲染成透明位图（拖拽时跟随鼠标的那张图）。
//
// inverted 参数用于拖拽状态：把径向渐变的光源与色标整体翻转 180°，
// 使正在拖拽的气泡与静态预览明显不同，提升辨识度。
// =============================================================================

namespace BubblePainter
{
// 在 bounds（应为正方形）内绘制一个气泡。
// shimmerPhase 为波光动画相位（弧度），由调用方按时间推进。
void drawBubble(QPainter *painter, const QRectF &bounds, int value,
                qreal shimmerPhase, bool inverted = false);

// 绘制一个六角凹槽。hovered 为 true 时高亮（拖拽悬停 / 可放置）。
void drawHexSlot(QPainter *painter, const QPolygonF &polygon, bool hovered = false);

// 生成透明背景的气泡位图，用于 QDrag 的拖动影像。
QPixmap renderBubble(int pixelSize, int value, qreal shimmerPhase, bool inverted = false);
} // namespace BubblePainter
