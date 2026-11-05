#pragma once

#include <QWidget>

class BoardWidget;
class GameModel;
class PreviewPanel;
class ScorePanel;

// =============================================================================
// GameWindow.h
// 主窗口：负责整体布局与模块之间的连接。
//
// 布局（按需求）：
//   ┌──────────────────────────────┬───────────────┐
//   │                              │   计分板      │
//   │      棋盘（背景板）3/4        ├───────────────┤
//   │                              │   预览栏      │
//   └──────────────────────────────┴───────────────┘
// =============================================================================

class GameWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GameWindow(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onBubbleDropped(int cellIndex, int value);
    void onGameFinished(int finalScore, bool newRecord);
    void startNewGame();

private:
    void buildLayout();
    void connectSignals();
    void applyInitialGeometry();

    GameModel *m_model = nullptr;      // 游戏规则
    BoardWidget *m_board = nullptr;    // 背景
    PreviewPanel *m_preview = nullptr; // 预览
    ScorePanel *m_score = nullptr;     // 分数
};
