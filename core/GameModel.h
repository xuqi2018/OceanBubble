#pragma once

#include <QObject>
#include <QVector>

#include "HexGrid.h"

// =============================================================================
// GameModel.h
// 游戏核心逻辑（与界面完全解耦）：
//   * 保存六边形蜂窝棋盘中 37 个六角槽的气泡数值；
//   * 处理放置气泡、三连合并、连锁升级、计分；
//   * 判断游戏结束并持久化历史最高分。
//
// 所有状态变化都通过信号通知界面，界面只负责绘制与动画，
// 因此模型可以独立测试、也方便后续替换渲染层。
//
// 计分规则（与需求一致）：
//   3 个 1 合并 = 1 分；3 个 2 = 2 分；3 个 4 = 3 分 ...
//   即 merge 基础分 = log2(旧数值) + 1。
//   同一次放置触发的连锁中，第 2 次合并的分数是第一份的两倍，
//   第 3 次再翻倍 ... 例如 1 + 2 + 4 = 7 分。
// =============================================================================

class GameModel : public QObject
{
    Q_OBJECT

public:
    enum State
    {
        Playing,   // 游戏进行中
        Finished   // 棋盘已满，本局结束
    };

    explicit GameModel(QObject *parent = nullptr);

    // 重置为全新一局（历史最高分保留）。
    void reset();

    int cellValue(int index) const;
    bool isCellEmpty(int index) const;
    bool isBoardFull() const;

    int score() const { return m_score; }
    int bestScore() const { return m_bestScore; }
    State state() const { return m_state; }

    // 随机生成预览数值：只能是 1 / 2 / 4 / 8。
    static int randomStartValue();

    // 尝试把数值为 value 的气泡放到 index 格。
    // 成功返回 true（放置 + 合并 + 计分全部完成），失败返回 false
    // （游戏已结束 / 越界 / 格子已被占用）。
    bool placeBubble(int index, int value);

signals:
    // 某个格子被放入或升级为 value，界面据此播放入场/弹跳动画。
    void cellFilled(int index, int value);

    // 某个格子因合并被清空。
    void cellCleared(int index);

    // 一次合并：sources 中的气泡消失，targetIndex 升级为 newValue。
    void mergeResolved(const QVector<int> &sources, int targetIndex, int sourceValue, int newValue);

    // 分数变化（一次连锁结束后统一通知一次）。
    void scoreChanged(int score);

    // 一次连锁结束：gained 为本连锁总得分，steps 为连锁次数。
    void cascadeFinished(int gained, int steps);

    // 游戏结束：finalScore 本局分数，newRecord 是否刷新历史最高分。
    void gameFinished(int finalScore, bool newRecord);

    // 完成一次重置。
    void resetDone();

    // 历史最高分变化。
    void bestScoreChanged(int best);

private:
    static int pointsForValue(int value);

    // 从 startIndex 出发，找出所有数值相同的连通格子（含自身）。
    QVector<int> connectedGroup(int startIndex) const;

    // 递归处理连锁升级，返回本次连锁总得分。
    int resolveCascades(int startIndex);

    void loadBestScore();
    void saveBestScore() const;

    HexGrid m_grid;           // 只用到 neighbors()，负责邻接关系
    QVector<int> m_cells;     // 每个格子的数值，0 表示空槽
    int m_score = 0;
    int m_bestScore = 0;
    State m_state = Playing;
};
