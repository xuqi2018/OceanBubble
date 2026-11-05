#include "PreviewPanel.h"

#include <QDataStream>
#include <QDrag>
#include <QFont>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include <cmath>

#include "Theme.h"
#include "core/GameModel.h"
#include "ui/BubblePainter.h"

namespace
{
const qreal kTwoPi = 6.283185307179586;
} // namespace

PreviewPanel::PreviewPanel(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(260);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);

    m_bigValue = randomValue();
    m_smallValue = randomValue();

    // 波光动画：约 30fps，仅重绘这个小区域。
    m_timer.setInterval(33);
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        m_phase = std::fmod(m_phase + 0.05, kTwoPi * 10.0);
        update();
    });
    m_timer.start();
}

int PreviewPanel::randomValue()
{
    return GameModel::randomStartValue();
}

void PreviewPanel::reset()
{
    m_bigValue = randomValue();
    m_smallValue = randomValue();
    m_dragging = false;
    m_hoverBig = false;
    update();
}

void PreviewPanel::promoteAfterPlacement()
{
    // 预览泡2 晋升为预览泡1，再随机生成一个新的预览泡2。
    m_bigValue = m_smallValue;
    m_smallValue = randomValue();
    m_hoverBig = false;
    update();
}

void PreviewPanel::setInteractionEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled)
        m_hoverBig = false;
    setCursor(Qt::ArrowCursor);
    update();
}

QRectF PreviewPanel::cardRect() const
{
    return QRectF(rect()).adjusted(1.0, 1.0, -1.0, -1.0);
}

QRectF PreviewPanel::contentRect() const
{
    const QRectF card = cardRect();
    const qreal padding = qMin(card.width(), card.height()) * 0.08;
    return card.adjusted(padding, padding, -padding, -padding);
}

QRectF PreviewPanel::bubbleArea() const
{
    const QRectF content = contentRect();
    const qreal header = content.height() * 0.09;
    const qreal footer = content.height() * 0.10;
    return content.adjusted(0.0, header, 0.0, -footer);
}

QRectF PreviewPanel::bigBubbleRect() const
{
    const QRectF area = bubbleArea();
    // 黄金分割式的上下留白（0.30 / 0.70），视觉重心略偏上更稳定。
    const qreal side = qMin(area.width() * 0.68, area.height() * 0.56);
    const QPointF c(area.center().x(), area.top() + area.height() * 0.30);
    return QRectF(c.x() - side / 2.0, c.y() - side / 2.0, side, side);
}

QRectF PreviewPanel::smallBubbleRect() const
{
    const QRectF area = bubbleArea();
    const qreal side = qMin(area.width() * 0.68, area.height() * 0.56) * 0.52;
    const QPointF c(area.center().x(), area.top() + area.height() * 0.79);
    return QRectF(c.x() - side / 2.0, c.y() - side / 2.0, side, side);
}

void PreviewPanel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // ---------------------------------------------------------------
    // 玻璃卡片背景
    // ---------------------------------------------------------------
    const QRectF card = cardRect();
    const qreal corner = qMin(card.width(), card.height()) * 0.07;

    QLinearGradient background(card.topLeft(), card.bottomRight());
    background.setColorAt(0.0, Theme::kCardTop);
    background.setColorAt(1.0, Theme::kCardBottom);
    painter.setPen(QPen(Theme::kCardBorder, 1.4));
    painter.setBrush(background);
    painter.drawRoundedRect(card, corner, corner);

    QLinearGradient sheen(card.topLeft(), QPointF(card.right(), card.top() + card.height() * 0.35));
    sheen.setColorAt(0.0, QColor(255, 255, 255, 46));
    sheen.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(sheen);
    painter.drawRoundedRect(card.adjusted(1.5, 1.5, -1.5, -1.5),
                            qMax<qreal>(0.0, corner - 1.0),
                            qMax<qreal>(0.0, corner - 1.0));

    // ---------------------------------------------------------------
    // 标题与提示
    // ---------------------------------------------------------------
    const QRectF content = contentRect();

    QFont titleFont(QStringLiteral("Segoe UI"));
    titleFont.setPixelSize(qMax(11, int(content.height() * 0.065)));
    titleFont.setWeight(QFont::DemiBold);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    painter.setFont(titleFont);
    painter.setPen(Theme::kTextSecondary);
    painter.drawText(QRectF(content.left(), content.top(),
                            content.width(), content.height() * 0.08),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("预览"));

    QFont hintFont(QStringLiteral("Segoe UI"));
    hintFont.setPixelSize(qMax(10, int(content.height() * 0.055)));
    painter.setFont(hintFont);
    painter.setPen(QColor(210, 245, 255, 165));
    painter.drawText(QRectF(content.left(), content.bottom() - content.height() * 0.08,
                            content.width(), content.height() * 0.08),
                     Qt::AlignHCenter | Qt::AlignVCenter,
                     m_enabled ? QStringLiteral("按住上方气泡拖到棋盘")
                               : QStringLiteral("本局已结束"));

    // ---------------------------------------------------------------
    // 两个预览气泡
    // ---------------------------------------------------------------
    const QRectF big = bigBubbleRect();
    const QRectF small = smallBubbleRect();

    // 悬停时给预览泡1 加一圈提示性光环。
    if (m_hoverBig && m_enabled && !m_dragging) {
        const qreal halo = big.width() * 0.06;
        painter.setPen(QPen(QColor(255, 255, 255, 90), 2.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(big.adjusted(-halo, -halo, halo, halo));
    }

    // 拖拽时 preview1 的渐变翻转 180°，静态时为海洋色球体。
    BubblePainter::drawBubble(&painter, big, m_bigValue, m_phase, m_dragging);
    // 预览泡2 稍微错开相位，两个气泡的波光不同步，画面更灵动。
    BubblePainter::drawBubble(&painter, small, m_smallValue, m_phase + 2.1, false);

    // ---------------------------------------------------------------
    // 两个气泡之间的推进箭头
    // ---------------------------------------------------------------
    const qreal gapTop = big.bottom();
    const qreal gapBottom = small.top();
    if (gapBottom - gapTop > 16.0) {
        const qreal cx = card.center().x();
        const qreal cy = (gapTop + gapBottom) / 2.0;
        const qreal halfWidth = qMin<qreal>(14.0, card.width() * 0.05);
        const qreal halfHeight = 4.5;

        QPainterPath arrow;
        arrow.moveTo(cx - halfWidth, cy - halfHeight);
        arrow.lineTo(cx, cy + halfHeight);
        arrow.lineTo(cx + halfWidth, cy - halfHeight);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(190, 240, 255, 150), 2.0,
                            Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(arrow);
    }
}

// =============================================================================
// 交互：按下预览泡1 立即翻转渐变并启动 QDrag。
// =============================================================================

void PreviewPanel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_enabled) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (bigBubbleRect().contains(event->pos())) {
        event->accept();
        startDrag();
        return;
    }

    QWidget::mousePressEvent(event);
}

void PreviewPanel::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_enabled) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const bool hover = bigBubbleRect().contains(event->pos());
    if (hover != m_hoverBig) {
        m_hoverBig = hover;
        setCursor(hover ? Qt::OpenHandCursor : Qt::ArrowCursor);
        update();
    }

    QWidget::mouseMoveEvent(event);
}

void PreviewPanel::leaveEvent(QEvent *event)
{
    if (m_hoverBig) {
        m_hoverBig = false;
        setCursor(Qt::ArrowCursor);
        update();
    }
    QWidget::leaveEvent(event);
}

void PreviewPanel::startDrag()
{
    // 先切换为反色状态并立即重绘，用户按下鼠标的瞬间就能看到辨识度变化。
    m_dragging = true;
    repaint();

    QMimeData *mime = new QMimeData;
    QByteArray payload;
    {
        QDataStream stream(&payload, QIODevice::WriteOnly);
        stream << m_bigValue;
    }
    mime->setData(QString::fromLatin1(Theme::kMimeBubble), payload);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mime);

    // 拖动影像同样使用反色气泡，并且按高 DPI 缩放比例的像素尺寸渲染，避免模糊。
    const qreal dpr = devicePixelRatioF();
    const int logicalSize = qMax(96, int(bigBubbleRect().width() * 0.92));
    QPixmap dragPixmap = BubblePainter::renderBubble(int(logicalSize * dpr), m_bigValue,
                                                     m_phase, true);
    dragPixmap.setDevicePixelRatio(dpr);
    drag->setPixmap(dragPixmap);
    drag->setHotSpot(dragPixmap.rect().center());

    drag->exec(Qt::CopyAction);

    // 拖拽结束（无论是否放置成功）都恢复静态渐变。
    m_dragging = false;
    update();
}
