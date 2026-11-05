#include "GameWindow.h"

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>
#include <QtGlobal>

#include "Theme.h"
#include "core/GameModel.h"
#include "ui/BoardWidget.h"
#include "ui/PreviewPanel.h"
#include "ui/ScorePanel.h"

GameWindow::GameWindow(QWidget *parent)
    : QWidget(parent)
    , m_model(new GameModel(this))
{
    setWindowTitle(QStringLiteral("海洋消消泡"));
    setMinimumSize(900, 620);

    buildLayout();
    connectSignals();
    applyInitialGeometry();

    m_score->setBestScore(m_model->bestScore());
    m_score->setScore(m_model->score());
}

void GameWindow::buildLayout()
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 棋盘：占窗口宽度的 3/4。
    m_board = new BoardWidget(m_model, this);

    auto *side = new QWidget(this);
    side->setMinimumWidth(250);
    auto *sideLayout = new QVBoxLayout(side);
    sideLayout->setContentsMargins(18, 18, 18, 18);
    sideLayout->setSpacing(16);

    m_score = new ScorePanel(side);
    m_preview = new PreviewPanel(side);

    // 右侧栏内部按黄金分割分配高度：小份 38.2%，大份 61.8%。
    const int smaller = qRound(1000.0 * (1.0 - 1.0 / Theme::kGoldenRatio));
    const int larger = 1000 - smaller;
    sideLayout->addWidget(m_score, smaller);
    sideLayout->addWidget(m_preview, larger);

    root->addWidget(m_board, 3);
    root->addWidget(side, 1);
}

void GameWindow::connectSignals()
{
    // 棋盘拖放 -> 模型结算 -> 预览队列推进。
    connect(m_board, &BoardWidget::bubbleDropped, this, &GameWindow::onBubbleDropped);

    // 模型状态 -> 界面显示。
    connect(m_model, &GameModel::scoreChanged, m_score, &ScorePanel::setScore);
    connect(m_model, &GameModel::bestScoreChanged, m_score, &ScorePanel::setBestScore);

    // 结束弹窗使用队列连接：此时 dropEvent 还在 QDrag 的嵌套事件循环里，
    // 立刻弹出模态对话框会打乱拖放流程，所以等本次事件处理完再弹。
    connect(m_model, &GameModel::gameFinished, this, &GameWindow::onGameFinished,
            Qt::QueuedConnection);
}

void GameWindow::applyInitialGeometry()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        resize(1280, 860);
        return;
    }

    // 需求：窗口横纵各占屏幕约 2/3，不全屏也不至于太小。
    const QRect available = screen->availableGeometry();
    int width = int(available.width() * 2.0 / 3.0);
    int height = int(available.height() * 2.0 / 3.0);

    width = qMin(qMax(width, 900), available.width());
    height = qMin(qMax(height, 620), available.height());

    resize(width, height);
    move(available.center() - QPoint(width / 2, height / 2));
}

void GameWindow::paintEvent(QPaintEvent *)
{
    // 右侧栏留白处的深海渐变背景，衬托玻璃卡片。
    QPainter painter(this);
    QLinearGradient background(rect().topLeft(), rect().bottomRight());
    background.setColorAt(0.0, QColor(3, 20, 46));
    background.setColorAt(0.55, QColor(6, 44, 90));
    background.setColorAt(1.0, QColor(9, 70, 120));
    painter.fillRect(rect(), background);
}

void GameWindow::onBubbleDropped(int cellIndex, int value)
{
    if (m_model->state() != GameModel::Playing)
        return;

    // 模型会再次校验格子是否为空，失败时预览栏保持不变。
    if (m_model->placeBubble(cellIndex, value))
        m_preview->promoteAfterPlacement();
}

void GameWindow::onGameFinished(int finalScore, bool newRecord)
{
    m_preview->setInteractionEnabled(false);

    QString message;
    if (newRecord) {
        message = QStringLiteral("恭喜！刷新新纪录！%1分数！").arg(finalScore);
    } else {
        message = QStringLiteral("游戏结束！本次得分 %1 分，历史最高 %2 分。")
                      .arg(finalScore)
                      .arg(m_model->bestScore());
    }

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("海洋消消泡"));
    box.setIcon(newRecord ? QMessageBox::Information : QMessageBox::Warning);
    box.setText(message);

    QPushButton *againButton = box.addButton(QStringLiteral("再来一局"), QMessageBox::AcceptRole);
    box.addButton(QStringLiteral("退出游戏"), QMessageBox::RejectRole);
    box.setDefaultButton(againButton);
    box.exec();

    if (box.clickedButton() == againButton)
        startNewGame();
    else
        close();
}

void GameWindow::startNewGame()
{
    m_model->reset();
    m_preview->reset();
    m_preview->setInteractionEnabled(true);
    m_score->setScore(0);
    m_score->setBestScore(m_model->bestScore());
}
