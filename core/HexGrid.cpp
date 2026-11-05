#include "HexGrid.h"

#include <QtMath>

#include <cmath>

namespace
{
// 尖顶六边形的 6 个顶点相对角度（单位：度）。
// 必须从 30° 开始：顶点位于正上/正下，左右为竖直边，
// 这样与 pointy-top 的中心间距（水平 sqrt(3)R、垂直 1.5R）完全吻合，
// 相邻六边形只共享边、绝不重叠。
const qreal kVertexAngles[6] = { 30.0, 90.0, 150.0, 210.0, 270.0, 330.0 };

// 轴向坐标下六个相邻方向，顺序任意，但必须与 pointy-top 布局一致。
const int kNeighborQ[6] = { 1, 1, 0, -1, -1, 0 };
const int kNeighborR[6] = { 0, -1, -1, 0, 1, 1 };
} // namespace

void HexGrid::setArea(const QRectF &area)
{
    const qreal sqrt3 = std::sqrt(3.0);

    // 半径为 1 时整个蜂窝阵型的理论尺寸：
    // 宽 = sqrt(3) * (2*半径 + 1)，高 = 1.5 * 2*半径 + 2。
    const qreal unitWidth = sqrt3 * kSide;
    const qreal unitHeight = 1.5 * (2 * kRadius) + 2.0;

    // 稍微收缩 8px，避免描边被控件边缘裁掉。
    const qreal safeWidth = qMax<qreal>(1.0, area.width() - 8.0);
    const qreal safeHeight = qMax<qreal>(1.0, area.height() - 8.0);

    m_radius = qMax<qreal>(6.0, qMin(safeWidth / unitWidth, safeHeight / unitHeight));
    m_center = area.center();
}

QPointF HexGrid::center(int index) const
{
    const HexCoord c = coordOf(index);
    const qreal sqrt3 = std::sqrt(3.0);
    return QPointF(m_center.x() + sqrt3 * m_radius * (c.q + c.r * 0.5),
                   m_center.y() + 1.5 * m_radius * c.r);
}

QPolygonF HexGrid::hexagon(int index) const
{
    const QPointF centerPoint = center(index);
    QPolygonF polygon;
    polygon.reserve(6);
    for (int i = 0; i < 6; ++i) {
        const qreal radians = qDegreesToRadians(kVertexAngles[i]);
        polygon.append(centerPoint + QPointF(m_radius * qCos(radians), m_radius * qSin(radians)));
    }
    return polygon;
}

int HexGrid::cellAt(const QPointF &pos) const
{
    if (m_radius <= 0.0)
        return -1;

    const qreal dx = pos.x() - m_center.x();
    const qreal dy = pos.y() - m_center.y();
    const qreal sqrt3 = std::sqrt(3.0);

    // 像素坐标 -> 轴坐标（浮点）。
    const qreal qf = (sqrt3 / 3.0 * dx - dy / 3.0) / m_radius;
    const qreal rf = (2.0 / 3.0 * dy) / m_radius;

    // 立方坐标取整：把浮点坐标吸附到最近的格子中心。
    const qreal xf = qf;
    const qreal zf = rf;
    const qreal yf = -xf - zf;

    qreal rx = qRound(xf);
    qreal ry = qRound(yf);
    qreal rz = qRound(zf);

    const qreal diffX = qAbs(rx - xf);
    const qreal diffY = qAbs(ry - yf);
    const qreal diffZ = qAbs(rz - zf);

    if (diffX > diffY && diffX > diffZ)
        rx = -ry - rz;
    else if (diffY > diffZ)
        ry = -rx - rz;
    else
        rz = -rx - ry;
        

    const int q = int(rx);
    const int r = int(rz);
    if (!isValid(q, r))
        return -1;

    const int index = indexOf(q, r);
    const QPointF cellCenter = center(index);

    // 距离校验：坐标在网格范围内但落在六边形之外时视为未命中，
    // 这样拖拽到棋盘边缘的空白处不会误放置。
    const qreal distance = std::hypot(pos.x() - cellCenter.x(), pos.y() - cellCenter.y());
    if (distance > m_radius * 0.95)
        return -1;

    return index;
}

QVector<int> HexGrid::neighbors(int index) const
{
    QVector<int> result;
    if (!isValidCell(index))
        return result;

    const HexCoord c = coordOf(index);
    result.reserve(6);
    for (int i = 0; i < 6; ++i) {
        const int q = c.q + kNeighborQ[i];
        const int r = c.r + kNeighborR[i];
        if (isValid(q, r))
            result.append(indexOf(q, r));
    }
    return result;
}
