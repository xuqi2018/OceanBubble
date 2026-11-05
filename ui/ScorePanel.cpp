#include "ScorePanel.h"

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include "Theme.h"

ScorePanel::ScorePanel(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(190);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_clock.start();
    m_pulseTimer.setInterval(25);
    connect(&m_pulseTimer, &QTimer::timeout, this, &ScorePanel::onPulseTick);
}

void ScorePanel::setScore(int score)
{
    if (score == m_score)
        return;

    m_score = score;
    m_scoreStamp = m_clock.elapsed();
    m_pulseTimer.start();
    update();
}

void ScorePanel::setBestScore(int best)
{
    if (best == m_bestScore)
        return;

    m_bestScore = best;
    update();
}

void ScorePanel::onPulseTick()
{
    if (m_clock.elapsed() - m_scoreStamp > 400)
        m_pulseTimer.stop();
    update();
}

void ScorePanel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QRectF card = QRectF(rect()).adjusted(1.0, 1.0, -1.0, -1.0);
    const qreal corner = qMin(card.width(), card.height()) * 0.09;
    const qreal h = card.height();

    // ---------------------------------------------------------------
    // 玻璃卡片背景
    // ---------------------------------------------------------------
    QLinearGradient background(card.topLeft(), card.bottomRight());
    background.setColorAt(0.0, Theme::kCardTop);
    background.setColorAt(1.0, Theme::kCardBottom);
    painter.setPen(QPen(Theme::kCardBorder, 1.4));
    painter.setBrush(background);
    painter.drawRoundedRect(card, corner, corner);

    QLinearGradient sheen(card.topLeft(), QPointF(card.right(), card.top() + h * 0.40));
    sheen.setColorAt(0.0, QColor(255, 255, 255, 42));
    sheen.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(sheen);
    painter.drawRoundedRect(card.adjusted(1.5, 1.5, -1.5, -1.5),
                            qMax<qreal>(0.0, corner - 1.0),
                            qMax<qreal>(0.0, corner - 1.0));

    // ---------------------------------------------------------------
    // 装饰：卡片内部右下角的淡淡水泡（裁剪在圆角内）
    // ---------------------------------------------------------------
    painter.save();
    QPainterPath clip;
    clip.addRoundedRect(card, corner, corner);
    painter.setClipPath(clip);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(200, 245, 255, 28), 2.0));
    painter.drawEllipse(QPointF(card.right() - card.width() * 0.14,
                                card.bottom() - card.height() * 0.14),
                        card.width() * 0.11, card.width() * 0.11);
    painter.setPen(QPen(QColor(200, 245, 255, 20), 1.6));
    painter.drawEllipse(QPointF(card.right() - card.width() * 0.26,
                                card.bottom() - card.height() * 0.22),
                        card.width() * 0.055, card.width() * 0.055);
    painter.restore();

    // ---------------------------------------------------------------
    // 标题
    // ---------------------------------------------------------------
    const qreal padX = card.width() * 0.11;
    const qreal padY = card.height() * 0.10;
    const qreal innerWidth = card.width() - 2.0 * padX;

    QFont titleFont(QStringLiteral("Segoe UI"));
    titleFont.setPixelSize(qMax(11, int(h * 0.072)));
    titleFont.setWeight(QFont::Bold);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    painter.setFont(titleFont);
    painter.setPen(Theme::kTextSecondary);
    painter.drawText(QRectF(card.left() + padX, card.top() + padY, innerWidth, h * 0.13),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("海洋消消泡"));

    // ---------------------------------------------------------------
    // “得分”标签
    // ---------------------------------------------------------------
    QFont labelFont(QStringLiteral("Segoe UI"));
    labelFont.setPixelSize(qMax(10, int(h * 0.062)));
    labelFont.setWeight(QFont::DemiBold);
    painter.setFont(labelFont);
    painter.setPen(QColor(165, 222, 246, 200));
    painter.drawText(QRectF(card.left() + padX, card.top() + h * 0.28, innerWidth, h * 0.08),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("得分"));

    // ---------------------------------------------------------------
    // 当前分数（带脉冲缩放）
    // ---------------------------------------------------------------
    const QString scoreText = QString::number(m_score);
    const qreal widthLimit = innerWidth / qMax<qreal>(1.0, scoreText.length() * 0.62);
    const int scoreFontPx = qMax(20, int(qMin(h * 0.27, widthLimit)));

    QFont scoreFont(QStringLiteral("Segoe UI"));
    scoreFont.setPixelSize(scoreFontPx);
    scoreFont.setWeight(QFont::Black);
    painter.setFont(scoreFont);

    const QRectF scoreRect(card.left() + padX, card.top() + h * 0.36, innerWidth, h * 0.30);

    qreal scale = 1.0;
    if (m_pulseTimer.isActive()) {
        const qreal t = qBound<qreal>(0.0, (m_clock.elapsed() - m_scoreStamp) / 380.0, 1.0);
        if (t < 1.0)
            scale = 1.0 + 0.12 * qSin(t * M_PI);
    }

    painter.save();
    if (scale != 1.0) {
        painter.translate(scoreRect.center());
        painter.scale(scale, scale);
        painter.translate(-scoreRect.center());
    }

    painter.setPen(QColor(3, 30, 66, 140));
    painter.drawText(scoreRect.translated(0.0, 2.0),
                     Qt::AlignLeft | Qt::AlignVCenter, scoreText);
    painter.setPen(QColor(255, 255, 255, 248));
    painter.drawText(scoreRect, Qt::AlignLeft | Qt::AlignVCenter, scoreText);
    painter.restore();

    // ---------------------------------------------------------------
    // 波浪分隔线
    // ---------------------------------------------------------------
    const qreal lineY = card.top() + h * 0.69;
    QPainterPath wave;
    wave.moveTo(card.left() + padX, lineY);
    wave.cubicTo(card.left() + card.width() * 0.40, lineY - 3.0,
                 card.left() + card.width() * 0.60, lineY + 3.0,
                 card.right() - padX, lineY);
    painter.setPen(QPen(QColor(170, 235, 255, 85), 1.4));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(wave);

    // ---------------------------------------------------------------
    // 历史最高分
    // ---------------------------------------------------------------
    QFont bestFont(QStringLiteral("Segoe UI"));
    bestFont.setPixelSize(qMax(10, int(h * 0.060)));
    bestFont.setWeight(QFont::DemiBold);
    painter.setFont(bestFont);

    const QRectF bestRect(card.left() + padX, card.top() + h * 0.73, innerWidth, h * 0.12);
    painter.setPen(QColor(185, 232, 250, 190));
    painter.drawText(bestRect, Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("历史最高"));
    painter.setPen(Theme::kTextPrimary);
    painter.drawText(bestRect, Qt::AlignRight | Qt::AlignVCenter,
                     QString::number(m_bestScore));
}
