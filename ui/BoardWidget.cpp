#include "BoardWidget.h"

#include <QDataStream>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QtMath>

#include <algorithm>

#include "Theme.h"
#include "core/GameModel.h"
#include "ui/BubblePainter.h"

namespace
{
const qreal kTwoPi = 6.283185307179586;
} // namespace

BoardWidget::BoardWidget(GameModel *model, QWidget *parent)
    : QWidget(parent)
    , m_model(model)
{
    Q_ASSERT(m_model);

    setAcceptDrops(true);
    setMinimumSize(520, 480);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 整个区域每帧都会被完全重绘，跳过系统背景擦除可以省一点 CPU。
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    m_clock.start();
    m_timer.setInterval(33);   // 约 30fps，用于波光与气泡动画
    connect(&m_timer, &QTimer::timeout, this, &BoardWidget::advanceAnimations);
    m_timer.start();

    connect(m_model, &GameModel::cellFilled, this, &BoardWidget::handleCellFilled);
    connect(m_model, &GameModel::mergeResolved, this, &BoardWidget::handleMerge);
    connect(m_model, &GameModel::resetDone, this, &BoardWidget::clearAnimations);
}

void BoardWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // 棋盘四周留白，网格自动缩放到可用区域并居中。
    m_grid.setArea(QRectF(rect()).adjusted(20.0, 20.0, -20.0, -20.0));
    update();
}

void BoardWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const qint64 now = m_clock.elapsed();

    // 1. 海洋渐变背景板
    paintOcean(painter);
    paintDecorations(painter, now);

    // 2. 所有六角凹槽（先画槽，再画槽里的气泡）
    //    棋盘是半径 3 的蜂窝阵型，7x7 索引中只有六边形轮廓内的槽位有效。
    for (int index = 0; index < HexGrid::cellCount(); ++index) {
        if (!HexGrid::isValidCell(index))
            continue;
        BubblePainter::drawHexSlot(&painter, m_grid.hexagon(index), index == m_hoverCell);
    }

    // 3. 已有气泡：带入场缩放与波光相位差
    const qreal phase = (now % 5000) / 5000.0 * kTwoPi * 2.0;
    for (int index = 0; index < HexGrid::cellCount(); ++index) {
        if (!HexGrid::isValidCell(index))
            continue;

        const int value = m_model->cellValue(index);
        if (value <= 0)
            continue;

        const qreal scale = spawnScale(index, now);
        const qreal bubbleRadius = m_grid.radius() * 0.80 * scale;
        if (bubbleRadius <= 0.5)
            continue;

        const QPointF c = m_grid.center(index);
        const QRectF bubbleRect(c.x() - bubbleRadius, c.y() - bubbleRadius,
                                bubbleRadius * 2.0, bubbleRadius * 2.0);
        BubblePainter::drawBubble(&painter, bubbleRect, value, phase + index * 0.37);
    }

    // 4. 合并残影：被吃掉的气泡放大淡出
    for (const GhostBubble &ghost : m_ghosts) {
        const qreal t = qBound(0.0, (now - ghost.startedAt) / 300.0, 1.0);
        if (t >= 1.0)
            continue;

        const qreal radius = m_grid.radius() * 0.80 * (1.0 + 0.35 * t);
        const QPointF c = m_grid.center(ghost.index);
        const QRectF bubbleRect(c.x() - radius, c.y() - radius, radius * 2.0, radius * 2.0);

        painter.save();
        painter.setOpacity(1.0 - t);
        BubblePainter::drawBubble(&painter, bubbleRect, ghost.value, phase + ghost.index * 0.37);
        painter.restore();
    }
}

void BoardWidget::paintOcean(QPainter &painter) const
{
    // 需求指定：海洋蓝 -> 白色（近白）的渐变。
    QLinearGradient gradient(rect().topLeft(), rect().bottomLeft());
    gradient.setColorAt(0.00, QColor(7, 46, 102));
    gradient.setColorAt(0.24, QColor(13, 82, 152));
    gradient.setColorAt(0.52, QColor(44, 148, 204));
    gradient.setColorAt(0.78, QColor(150, 220, 244));
    gradient.setColorAt(1.00, QColor(242, 251, 255));

    painter.fillRect(rect(), gradient);
}

void BoardWidget::paintDecorations(QPainter &painter, qint64 now) const
{
    const qreal seconds = now / 1000.0;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    // 缓慢上升的细小气泡，增加「水下」氛围（纯装饰，不参与游戏逻辑）。
    for (int i = 0; i < 9; ++i) {
        const qreal seed = i * 1.37;
        const qreal x = width() * (0.06 + 0.105 * i) + qSin(seconds * 0.32 + seed) * 16.0;
        const qreal y = height() * (0.90 - 0.085 * (i % 5)) + qCos(seconds * 0.26 + seed) * 12.0;
        const qreal radius = 6.0 + (i % 4) * 6.0;

        QRadialGradient glow(QPointF(x, y), radius);
        glow.setColorAt(0.0, QColor(255, 255, 255, 34));
        glow.setColorAt(1.0, QColor(255, 255, 255, 0));
        painter.setBrush(glow);
        painter.drawEllipse(QPointF(x, y), radius, radius);
    }

    // 从水面斜射下来的光带。
    const QPolygonF beam = QPolygonF()
                           << QPointF(width() * 0.06, 0.0)
                           << QPointF(width() * 0.24, 0.0)
                           << QPointF(width() * 0.58, height())
                           << QPointF(width() * 0.34, height());
    QLinearGradient ray(QPointF(width() * 0.10, 0.0), QPointF(width() * 0.34, height() * 0.75));
    ray.setColorAt(0.0, QColor(255, 255, 255, 34));
    ray.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.setBrush(ray);
    painter.drawPolygon(beam);

    painter.restore();
}

qreal BoardWidget::spawnScale(int index, qint64 now) const
{
    const auto it = m_spawnAt.constFind(index);
    if (it == m_spawnAt.constEnd())
        return 1.0;

    const qreal t = qBound(0.0, (now - it.value()) / 260.0, 1.0);

    // easeOutBack：先快速放大，再轻微回弹到 1.0。
    const qreal c1 = 1.70158;
    const qreal c3 = c1 + 1.0;
    const qreal x = t - 1.0;
    return 1.0 + c3 * x * x * x + c1 * x * x;
}

void BoardWidget::handleCellFilled(int index, int)
{
    m_spawnAt[index] = m_clock.elapsed();
}

void BoardWidget::handleMerge(const QVector<int> &sources, int targetIndex,
                              int sourceValue, int)
{
    const qint64 now = m_clock.elapsed();
    for (int index : sources)
        m_ghosts.append(GhostBubble{ index, sourceValue, now });

    // 升级后的目标格子也播放一次弹跳。
    m_spawnAt[targetIndex] = now;
}

void BoardWidget::clearAnimations()
{
    m_spawnAt.clear();
    m_ghosts.clear();
    m_hoverCell = -1;
    update();
}

void BoardWidget::advanceAnimations()
{
    const qint64 now = m_clock.elapsed();

    // 清理已经播完的动画数据，避免哈希表无限增长。
    for (auto it = m_spawnAt.begin(); it != m_spawnAt.end();) {
        if (now - it.value() > 420)
            it = m_spawnAt.erase(it);
        else
            ++it;
    }

    m_ghosts.erase(std::remove_if(m_ghosts.begin(), m_ghosts.end(),
                                  [now](const GhostBubble &ghost) {
                                      return now - ghost.startedAt > 320;
                                  }),
                   m_ghosts.end());

    update();
}

// =============================================================================
// 拖放交互：预览栏通过 QDrag 传来自定义 MIME 数据，这里负责命中和放置。
// =============================================================================

void BoardWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(QString::fromLatin1(Theme::kMimeBubble)))
        event->acceptProposedAction();
    else
        event->ignore();
}

void BoardWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (!event->mimeData()->hasFormat(QString::fromLatin1(Theme::kMimeBubble))) {
        event->ignore();
        return;
    }

    const int cell = m_grid.cellAt(event->pos());
    const int validCell = (cell >= 0 && m_model->isCellEmpty(cell) &&
                           m_model->state() == GameModel::Playing)
                          ? cell : -1;

    if (validCell != m_hoverCell) {
        m_hoverCell = validCell;
        update();
    }

    // 只有悬停在空格子上才允许放下，其它位置显示「禁止」光标。
    if (validCell >= 0)
        event->acceptProposedAction();
    else
        event->ignore();
}

void BoardWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    QWidget::dragLeaveEvent(event);
    if (m_hoverCell != -1) {
        m_hoverCell = -1;
        update();
    }
}

void BoardWidget::dropEvent(QDropEvent *event)
{
    const int cell = m_grid.cellAt(event->pos());
    m_hoverCell = -1;
    update();

    if (!event->mimeData()->hasFormat(QString::fromLatin1(Theme::kMimeBubble))) {
        event->ignore();
        return;
    }

    if (cell < 0 || !m_model->isCellEmpty(cell) || m_model->state() != GameModel::Playing) {
        event->ignore();
        return;
    }

    QByteArray payload = event->mimeData()->data(QString::fromLatin1(Theme::kMimeBubble));
    QDataStream stream(&payload, QIODevice::ReadOnly);
    int value = 0;
    stream >> value;
    if (value <= 0) {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
    emit bubbleDropped(cell, value);
}
