#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include "core/HexGrid.h"

class GameModel;

// =============================================================================
// BoardWidget.h
// 游戏棋盘（即需求中的「背景板」）：
//   * 绘制海洋渐变背景、六角凹槽、气泡；
//   * 接收从预览栏拖来的气泡（Qt 拖放事件）；
//   * 处理气泡入场、合并消失等轻量动画。
//
// 动画由 30fps 的 QTimer 驱动（37 个槽位的数据量在主线程完全够用，
// 不需要额外线程，避免无意义的线程同步开销）。
// =============================================================================

class BoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BoardWidget(GameModel *model, QWidget *parent = nullptr);

signals:
    // 用户把气泡拖放到某个空格子上。
    void bubbleDropped(int cellIndex, int value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void handleCellFilled(int index, int value);
    void handleMerge(const QVector<int> &sources, int targetIndex,
                     int sourceValue, int newValue);
    void clearAnimations();
    void advanceAnimations();

private:
    // 合并时原气泡消失的残影。
    struct GhostBubble
    {
        int index;
        int value;
        qint64 startedAt;
    };

    void paintOcean(QPainter &painter) const;
    void paintDecorations(QPainter &painter, qint64 now) const;

    // 新气泡的弹出缩放系数（easeOutBack，0 -> 1 -> 轻微回弹）。
    qreal spawnScale(int index, qint64 now) const;

    GameModel *m_model = nullptr;
    HexGrid m_grid;

    QTimer m_timer;             // 动画时钟
    QElapsedTimer m_clock;      // 单调计时，用于计算动画进度
    QHash<int, qint64> m_spawnAt;   // 格子索引 -> 入场动画开始时间
    QVector<GhostBubble> m_ghosts;  // 合并残影
    int m_hoverCell = -1;       // 拖拽悬停的格子（-1 表示没有）
};
