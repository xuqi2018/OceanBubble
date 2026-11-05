#pragma once

#include <QTimer>
#include <QWidget>

// =============================================================================
// PreviewPanel.h
// 右侧预览栏：
//   * 上下两个正圆气泡：预览泡1（大）、预览泡2（小）；
//   * 气泡内数字随机取 1 / 2 / 4 / 8；
//   * 按住大气泡拖动时，渐变翻转 180°，并用 QDrag 把数据投递给棋盘；
//   * 放置成功后调用 promoteAfterPlacement()：
//       预览泡2 晋升为预览泡1，预览泡2 重新随机。
// =============================================================================

class PreviewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewPanel(QWidget *parent = nullptr);

    // 全新一局：两个气泡都重新随机。
    void reset();

    // 成功放置一个气泡后推进预览队列。
    void promoteAfterPlacement();

    // 游戏结束时锁定交互。
    void setInteractionEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QRectF cardRect() const;
    QRectF contentRect() const;
    QRectF bubbleArea() const;      // 标题与提示文字之间的区域
    QRectF bigBubbleRect() const;   // 预览泡1
    QRectF smallBubbleRect() const; // 预览泡2

    void startDrag();
    static int randomValue();

    int m_bigValue = 1;
    int m_smallValue = 1;

    qreal m_phase = 0.0;        // 波光动画相位
    bool m_dragging = false;    // 正在拖拽（此时气泡渐变翻转）
    bool m_hoverBig = false;    // 鼠标悬停在预览泡1上
    bool m_enabled = true;

    QTimer m_timer;
};
