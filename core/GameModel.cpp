#include "GameModel.h"

#include <QRandomGenerator>
#include <QSettings>
#include <QVector>

namespace
{
// QSettings 中保存历史最高分的键名。
const char kBestScoreKey[] = "score/best";
} // namespace

GameModel::GameModel(QObject *parent)
    : QObject(parent)
{
    m_cells.fill(0, HexGrid::cellCount());
    loadBestScore();
}

void GameModel::reset()
{
    m_cells.fill(0, HexGrid::cellCount());
    m_score = 0;
    m_state = Playing;

    emit resetDone();
    emit scoreChanged(m_score);
    emit bestScoreChanged(m_bestScore);
}

int GameModel::cellValue(int index) const
{
    if (index < 0 || index >= m_cells.size())
        return 0;
    return m_cells.at(index);
}

bool GameModel::isCellEmpty(int index) const
{
    return cellValue(index) == 0;
}

bool GameModel::isBoardFull() const
{
    // 只统计六边形轮廓内的有效槽位（外部索引槽永远为空）。
    for (int index = 0; index < m_cells.size(); ++index) {
        if (HexGrid::isValidCell(index) && m_cells.at(index) == 0)
            return false;
    }
    return true;
}

int GameModel::randomStartValue()
{
    static const int kValues[4] = { 1, 2, 4, 8 };
    return kValues[QRandomGenerator::global()->bounded(4)];
}

int GameModel::pointsForValue(int value)
{
    // 3 个 1 -> 1 分，3 个 2 -> 2 分，3 个 4 -> 3 分 ...
    // 等价于 log2(value) + 1。
    int logarithm = 0;
    int v = value;
    while (v > 1) {
        v >>= 1;
        ++logarithm;
    }
    return logarithm + 1;
}

QVector<int> GameModel::connectedGroup(int startIndex) const
{
    QVector<int> group;
    if (startIndex < 0 || startIndex >= m_cells.size())
        return group;

    const int seedValue = m_cells.at(startIndex);
    if (seedValue == 0)
        return group;

    // 深度优先搜索：只收集数值完全相同的连通格子。
    QVector<bool> visited(m_cells.size(), false);
    QVector<int> stack;
    stack.append(startIndex);
    visited[startIndex] = true;

    while (!stack.isEmpty()) {
        const int current = stack.takeLast();
        group.append(current);

        const QVector<int> adjacent = m_grid.neighbors(current);
        for (int next : adjacent) {
            if (!visited.at(next) && m_cells.at(next) == seedValue) {
                visited[next] = true;
                stack.append(next);
            }
        }
    }
    return group;
}

int GameModel::resolveCascades(int startIndex)
{
    int steps = 0;         // 本次连锁已经发生的合并次数
    int gained = 0;        // 本次连锁累计得分
    int lastAward = 0;     // 上一次合并获得的分数（用于翻倍）
    int current = startIndex;

    // 只有「刚刚变化过的格子」才可能形成新的三连，
    // 所以每一轮都从 current 出发寻找连通组即可。
    while (current >= 0 && m_cells.at(current) != 0) {
        const QVector<int> group = connectedGroup(current);
        if (group.size() < 3)
            break;   // 不足 3 个，连锁结束

        // 保留 current 作为升级落点（视觉上就是刚放下的那个气泡升级），
        // 另外吃掉同组中的两个格子。
        QVector<int> sources;
        for (int index : group) {
            if (index == current)
                continue;
            sources.append(index);
            if (sources.size() == 2)
                break;
        }
        if (sources.size() < 2)
            break;

        const int oldValue = m_cells.at(current);
        for (int index : sources) {
            m_cells[index] = 0;
            emit cellCleared(index);
        }

        const int newValue = oldValue * 2;
        m_cells[current] = newValue;

        // 计分：第一跳取基础分，之后每多一跳翻一倍。
        const int basePoints = pointsForValue(oldValue);
        lastAward = (steps == 0) ? basePoints : lastAward * 2;
        gained += lastAward;
        m_score += lastAward;

        emit mergeResolved(sources, current, oldValue, newValue);
        emit cellFilled(current, newValue);

        ++steps;
    }

    if (steps > 0) {
        emit scoreChanged(m_score);
        emit cascadeFinished(gained, steps);
    }
    return gained;
}

bool GameModel::placeBubble(int index, int value)
{
    if (m_state != Playing)
        return false;
    if (!HexGrid::isValidCell(index))
        return false;
    if (m_cells.at(index) != 0)
        return false;
    if (value <= 0)
        return false;

    m_cells[index] = value;
    emit cellFilled(index, value);

    // 放置后立即结算可能产生的连锁升级。
    resolveCascades(index);

    // 连锁结束后若棋盘被填满，则本局结束。
    if (isBoardFull()) {
        m_state = Finished;

        const bool newRecord = m_score > m_bestScore;
        if (newRecord) {
            m_bestScore = m_score;
            saveBestScore();
            emit bestScoreChanged(m_bestScore);
        }
        emit gameFinished(m_score, newRecord);
    }
    return true;
}

void GameModel::loadBestScore()
{
    QSettings settings;
    m_bestScore = settings.value(QLatin1String(kBestScoreKey), 0).toInt();
}

void GameModel::saveBestScore() const
{
    QSettings settings;
    settings.setValue(QLatin1String(kBestScoreKey), m_bestScore);
    settings.sync();
}
