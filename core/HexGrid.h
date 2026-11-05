#pragma once

#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QVector>

// =============================================================================
// HexGrid.h
// 六角网格几何工具。
//
// 布局采用「轴向坐标」(axial coordinates)，尖顶朝向(pointy-top)：
//   * q 表示横向，r 表示纵向；
//   * 相邻的 6 个格子偏移固定为 (1,0) (1,-1) (0,-1) (-1,0) (-1,1) (0,1)；
//   * 棋盘取半径 3 的正六边形蜂窝阵型：以中心格为圆心向外 3 圈，
//     共 1 + 6 + 12 + 18 = 37 个六角槽，轮廓与参考图一致（中心 + 六角花环）。
//
// 坐标范围是 q,r ∈ [-3, 3]，但只有满足 |q|、|r|、|q+r| 均不超过 3 的格子有效，
// 因此内部用 7x7 的槽位数组承载，非法槽位永远不会被访问或绘制。
//
// 本类只负责几何计算，不保存格子里的数值（数值由 GameModel 保存）。
// =============================================================================

struct HexCoord
{
    int q;
    int r;
};

class HexGrid
{
public:
    static const int kRadius = 3;                // 蜂窝半径（圈数）
    static const int kSide = 2 * kRadius + 1;    // 索引数组边长：7

    // 索引数组槽位总数（含六边形轮廓外的无效格）。
    static int cellCount() { return kSide * kSide; }
    // 实际可用的六角槽数量：37。
    static int playableCount() { return 3 * kRadius * (kRadius + 1) + 1; }

    // 轴向坐标是否落在六边形棋盘内。
    static bool isValid(int q, int r)
    {
        return q >= -kRadius && q <= kRadius
               && r >= -kRadius && r <= kRadius
               && (q + r) >= -kRadius && (q + r) <= kRadius;
    }

    static int indexOf(int q, int r) { return (r + kRadius) * kSide + (q + kRadius); }
    static HexCoord coordOf(int index)
    {
        return HexCoord{ index % kSide - kRadius, index / kSide - kRadius };
    }
    static bool isValidCell(int index)
    {
        if (index < 0 || index >= cellCount())
            return false;
        const HexCoord c = coordOf(index);
        return isValid(c.q, c.r);
    }

    // 根据棋盘可用区域重新计算六边形半径与中心点，使整个蜂窝居中且不溢出。
    void setArea(const QRectF &area);

    qreal radius() const { return m_radius; }

    // 返回某个格子中心在控件坐标系中的位置。
    QPointF center(int index) const;

    // 返回某个格子的正六边形轮廓（用于绘制凹槽）。
    QPolygonF hexagon(int index) const;

    // 根据鼠标/拖拽位置反查命中的格子索引，未命中任何格子返回 -1。
    int cellAt(const QPointF &pos) const;

    // 返回某个格子的相邻格子索引（六条边各一个，自动剔除越界）。
    QVector<int> neighbors(int index) const;

private:
    qreal m_radius = 40.0;   // 正六边形外接圆半径
    QPointF m_center;        // 整个棋盘的几何中心（也是 (0,0) 格的中心）
};
