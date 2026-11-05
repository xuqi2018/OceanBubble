#pragma once

#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

// =============================================================================
// ScorePanel.h
// 右侧计分板：显示本局得分与历史最高分，分数变化时数字会轻微弹跳。
// 外观是一张半透明的深海蓝玻璃卡片，与海洋主题一致。
// =============================================================================

class ScorePanel : public QWidget
{
    Q_OBJECT

public:
    explicit ScorePanel(QWidget *parent = nullptr);

public slots:
    void setScore(int score);
    void setBestScore(int best);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onPulseTick();

private:
    int m_score = 0;
    int m_bestScore = 0;

    qint64 m_scoreStamp = 0;        // 最近一次加分的时间戳
    QElapsedTimer m_clock;          // 单调时钟，用于脉冲动画
    QTimer m_pulseTimer;            // 仅在分数变化后短暂运行
};
