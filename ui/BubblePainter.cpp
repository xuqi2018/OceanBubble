#include "BubblePainter.h"

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QtMath>

#include <cmath>

namespace
{
const qreal kTwoPi = 6.283185307179586;

// 根据数值位数自动收缩字号，保证 4096 这类四位数也能放进气泡。
qreal fontSizeForRadius(qreal radius, int digits)
{
    if (digits <= 2)
        return radius * 0.95;
    if (digits == 3)
        return radius * 0.78;
    return radius * 0.62;
}
} // namespace

void BubblePainter::drawBubble(QPainter *painter, const QRectF &bounds, int value,
                               qreal shimmerPhase, bool inverted)
{
    if (!painter || bounds.width() <= 1.0 || bounds.height() <= 1.0)
        return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setPen(Qt::NoPen);

    const QPointF center = bounds.center();
    const qreal radius = qMin(bounds.width(), bounds.height()) / 2.0;

    // 光源方向：静态时在左上（高光、仿玻璃质感）；拖拽时翻到右下。
    const QPointF lightOffset = inverted ? QPointF(0.42, 0.42) : QPointF(-0.42, -0.42);
    const QPointF light = center + lightOffset * radius;

    // ---------------------------------------------------------------
    // 1. 外发光：让气泡像悬浮在水中一样
    // ---------------------------------------------------------------
    QRadialGradient glow(center, radius * 1.32);
    glow.setColorAt(0.72, QColor(150, 235, 255, 0));
    glow.setColorAt(0.84, QColor(150, 235, 255, inverted ? 95 : 55));
    glow.setColorAt(1.00, QColor(150, 235, 255, 0));
    painter->setBrush(glow);
    painter->drawEllipse(center, radius * 1.32, radius * 1.32);

    // ---------------------------------------------------------------
    // 2. 球体主体渐变（海洋色）+ 180° 反色版本
    // ---------------------------------------------------------------
    QRadialGradient body(light, radius * 1.12, center);
    if (!inverted) {
        body.setColorAt(0.00, QColor(215, 250, 255));
        body.setColorAt(0.30, QColor(110, 222, 250));
        body.setColorAt(0.60, QColor(38, 158, 218));
        body.setColorAt(0.84, QColor(12, 92, 168));
        body.setColorAt(1.00, QColor(5, 48, 110));
    } else {
        body.setColorAt(0.00, QColor(5, 48, 110));
        body.setColorAt(0.30, QColor(12, 92, 168));
        body.setColorAt(0.60, QColor(38, 158, 218));
        body.setColorAt(0.84, QColor(110, 222, 250));
        body.setColorAt(1.00, QColor(215, 250, 255));
    }
    painter->setBrush(body);
    painter->drawEllipse(bounds);

    // ---------------------------------------------------------------
    // 3. 内缘高光：勾勒出气泡膜的厚度
    // ---------------------------------------------------------------
    QPen rimPen(QColor(214, 248, 255, inverted ? 175 : 230));
    rimPen.setWidthF(qMax<qreal>(1.2, radius * 0.035));
    painter->setPen(rimPen);
    painter->setBrush(Qt::NoBrush);
    const qreal inset = rimPen.widthF() / 2.0;
    painter->drawEllipse(bounds.adjusted(inset, inset, -inset, -inset));

    // ---------------------------------------------------------------
    // 4. 波光粼粼：裁剪在圆内绘制流动高光 + 闪烁点
    // ---------------------------------------------------------------
    QPainterPath clipPath;
    clipPath.addEllipse(bounds);
    painter->save();
    painter->setClipPath(clipPath);
    painter->setPen(Qt::NoPen);

    const qreal travel = qSin(shimmerPhase) * radius * 0.55;
    QRadialGradient sheen(QPointF(center.x() - radius * 0.15 + travel,
                                  center.y() - radius * 0.28),
                          radius * 1.05);
    sheen.setColorAt(0.0, QColor(255, 255, 255, 40));
    sheen.setColorAt(0.55, QColor(255, 255, 255, 10));
    sheen.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter->setBrush(sheen);
    painter->drawEllipse(center, radius * 1.05, radius * 1.05);

    for (int i = 0; i < 3; ++i) {
        const qreal t = shimmerPhase * 1.15 + i * (kTwoPi / 3.0);
        const qreal sx = center.x() + qCos(t) * radius * 0.52;
        const qreal sy = center.y() + qSin(t * 1.37) * radius * 0.46;
        const qreal sparkRadius = radius * (0.045 + 0.022 * (0.5 + 0.5 * qSin(t * 2.1)));

        QRadialGradient spark(QPointF(sx, sy), sparkRadius * 2.8);
        spark.setColorAt(0.0, QColor(255, 255, 255, 130));
        spark.setColorAt(1.0, QColor(255, 255, 255, 0));
        painter->setBrush(spark);
        painter->drawEllipse(QPointF(sx, sy), sparkRadius * 2.8, sparkRadius * 2.8);
    }

    // 静态镜面高光，随光源方向移动。
    painter->setBrush(QColor(255, 255, 255, 130));
    painter->drawEllipse(QRectF(light.x() - radius * 0.17,
                                light.y() - radius * 0.085,
                                radius * 0.34,
                                radius * 0.17));

    painter->restore();   // 解除裁剪

    // ---------------------------------------------------------------
    // 5. 气泡上的数字
    // ---------------------------------------------------------------
    if (value > 0) {
        const QString text = QString::number(value);
        QFont font(QStringLiteral("Segoe UI"));
        font.setPixelSize(qMax(10, int(fontSizeForRadius(radius, text.length()))));
        font.setWeight(QFont::Black);
        painter->setFont(font);

        const QRectF textRect = bounds;

        // 深色投影，保证在浅色/深色区域都清晰。
        painter->setPen(QColor(3, 34, 76, inverted ? 140 : 190));
        painter->drawText(textRect.translated(0.0, radius * 0.06),
                          Qt::AlignCenter, text);

        painter->setPen(QColor(255, 255, 255, 248));
        painter->drawText(textRect, Qt::AlignCenter, text);
    }

    painter->restore();
}

void BubblePainter::drawHexSlot(QPainter *painter, const QPolygonF &polygon, bool hovered)
{
    if (!painter || polygon.isEmpty())
        return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    path.addPolygon(polygon);
    path.closeSubpath();

    const QRectF bounds = polygon.boundingRect();

    // 底部柔和阴影，让凹槽像嵌进水里。
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(6, 34, 74, 45));
    painter->drawPath(path.translated(0.0, 2.5));

    // 凹槽填充：半透明玻璃质感，悬停时更亮。
    QLinearGradient fill(bounds.topLeft(), bounds.bottomRight());
    if (hovered) {
        fill.setColorAt(0.0, QColor(210, 250, 255, 135));
        fill.setColorAt(1.0, QColor(90, 205, 245, 110));
    } else {
        fill.setColorAt(0.0, QColor(220, 250, 255, 48));
        fill.setColorAt(1.0, QColor(120, 200, 240, 34));
    }
    painter->setBrush(fill);
    painter->drawPath(path);

    // 双色描边：外深内亮，无论在深蓝还是近白的背景上都清晰。
    QPen darkPen(QColor(10, 70, 120, hovered ? 150 : 95));
    darkPen.setWidthF(hovered ? 3.4 : 2.8);
    darkPen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(darkPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);

    QPen lightPen(hovered ? QColor(255, 255, 255, 245) : QColor(180, 240, 255, 190));
    lightPen.setWidthF(hovered ? 2.0 : 1.6);
    lightPen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(lightPen);
    painter->drawPath(path);

    painter->restore();
}

QPixmap BubblePainter::renderBubble(int pixelSize, int value, qreal shimmerPhase, bool inverted)
{
    const int size = qMax(8, pixelSize);
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 四周留出空间给外发光，避免被位图边缘裁断。
    const qreal margin = size * 0.06;
    drawBubble(&painter, QRectF(margin, margin, size - 2.0 * margin, size - 2.0 * margin),
               value, shimmerPhase, inverted);
    painter.end();

    return pixmap;
}
