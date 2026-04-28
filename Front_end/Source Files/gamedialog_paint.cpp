#include "gamedialog.h"
#include "llkappsettings.h"
#include <QLineF>

void GameDialog::paintEvent(QPaintEvent*){
    QPainter painter(this);
    //绘制背景
    if(!m_bgGame.isNull())
        painter.drawPixmap(0, 0, 800, 600, m_bgGame);

    QFont statFont("KaiTi", 13, QFont::Bold);
    painter.setFont(statFont);
    painter.setPen(QColor(92, 55, 24));
    painter.setBrush(QColor(255, 248, 235, 210));
    painter.drawRoundedRect(150, 5, 500, 30, 10, 10);
    painter.setBrush(Qt::NoBrush);
    const QString timeLabel = m_bCustomMode ? "限时" : "用时";
    const qint64 timeValue = m_bCustomMode ? CurrentRemainingMs() : CurrentActiveElapsedMs();
    const QString statText = QString("重排次数：%1    提示次数：%2    %3：%4")
        .arg(m_resetCount)
        .arg(m_tipCount)
        .arg(timeLabel)
        .arg(FormatElapsedText(timeValue));
    if(m_bCustomMode){
        // 限时模式下显示精简统计 + 时间 + 剩余时间进度条
        const QString compactText = QString("重排:%1   提示:%2   限时:%3")
            .arg(m_resetCount)
            .arg(m_tipCount)
            .arg(FormatElapsedText(timeValue));
        painter.drawText(QRect(165, 5, 270, 30), Qt::AlignVCenter | Qt::AlignLeft, compactText);

        const QRect barRect(445, 11, 185, 18);
        const qreal totalMs = static_cast<qreal>(m_modeConfig.timeLimitSeconds) * 1000.0;
        const qreal remainMs = static_cast<qreal>(CurrentRemainingMs());
        const qreal ratio = (totalMs > 0.0) ? qBound(0.0, remainMs / totalMs, 1.0) : 0.0;

        painter.setPen(QPen(QColor(140, 108, 72), 1));
        painter.setBrush(QColor(248, 236, 216, 230));
        painter.drawRoundedRect(barRect, 8, 8);

        const int fillWidth = static_cast<int>(barRect.width() * ratio);
        if(fillWidth > 0){
            QRect fillRect = barRect.adjusted(1, 1, -(barRect.width() - fillWidth) - 1, -1);
            painter.setPen(Qt::NoPen);
            QColor fillColor(84, 199, 116, 230); //剩余时间大于40%为绿色
            if(ratio <= 0.2){
                //剩余时间小于等于20%为红色并闪烁
                fillColor = QColor(232, 86, 86, 230);
                const bool flashOn = ((CurrentActiveElapsedMs() / 260) % 2) == 0;
                if(!flashOn){
                    fillColor.setAlpha(90);
                }
            }else if(ratio <= 0.4){
                //剩余时间小于等于40%为黄色
                fillColor = QColor(247, 212, 79, 230);
            }
            painter.setBrush(fillColor);
            painter.drawRoundedRect(fillRect, 7, 7);
        }

        if(!m_limitTimeIcon.isNull()){
            const int iconSize = 18;
            const int iconX = barRect.left() + static_cast<int>(ratio * barRect.width()) - iconSize / 2;
            const int clampedX = qBound(barRect.left() - iconSize / 2, iconX, barRect.right() - iconSize / 2);
            const int iconY = barRect.center().y() - iconSize / 2;
            painter.drawPixmap(clampedX, iconY, iconSize, iconSize, m_limitTimeIcon);
        }

        painter.setBrush(Qt::NoBrush);
    }else{
        painter.drawText(QRect(180, 5, 440, 30), Qt::AlignVCenter | Qt::AlignLeft, statText);
    }

    if(!m_bHasGameProgress || m_icons.isEmpty()) return;
    //绘制游戏地图
    const int cellW = m_icons[0].width();
    const int cellH = m_icons[0].height();
    const int gapX = 5;
    const int gapY = 3;
    const int rows = MapRows();
    const int cols = MapCols();
    const int gridW = cols * cellW + (cols - 1) * gapX;
    const int gridH = rows * cellH + (rows - 1) * gapY;
    int xOffset = (width() - gridW) / 2;
    int yOffset = (height() - gridH) / 2 - 47;
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            int val = m_pGameLogic->GetMapVal(i, j);
            if(val <= 0) continue;
            int x = xOffset + j * (cellW + gapX);
            int y = yOffset + i * (cellH + gapY);
            int iconIndex = (val - 1) % m_icons.size();

            painter.drawPixmap(x, y, m_icons[iconIndex]);
        }
    }
    if(m_bFirstSelected){
        int selectX = xOffset + m_firstPoint.col * (cellW + gapX);
        int selectY = yOffset + m_firstPoint.row * (cellH + gapY);
        if(m_bSelectBlinkVisible){
            const QColor selectColor = LLKAppSettings::instance().selectionFrameColor();
            QPen selectPen(selectColor, 3, Qt::SolidLine);
            painter.setPen(selectPen);
            painter.drawRect(selectX, selectY, cellW, cellH);
        }
    }
    DrawHintOverlay(painter, xOffset, yOffset, cellW, cellH, gapX, gapY);
    DrawTipLine(painter, xOffset, yOffset, cellW, cellH, gapX, gapY);
    DrawNoticeOverlay(painter);
}

void GameDialog::DrawHintOverlay(QPainter& painter, int xOffset, int yOffset, int cellW, int cellH, int gapX, int gapY){
    if(m_hintItems.isEmpty() || !m_bHintVisible || m_hintIndex < 0 || m_hintIndex >= m_hintItems.size()){
        return;
    }

    const HintItem& hint = m_hintItems[m_hintIndex];
    const QColor glowColor(255, 176, 56, 210);
    const QColor fillColor(255, 241, 196, 70);
    const QColor lineColor(255, 140, 0, 240);

    QPen glowPen(glowColor, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(glowPen);
    painter.setBrush(fillColor);

    auto drawCell = [&](const Vertex& vertex){
        const int x = xOffset + vertex.col * (cellW + gapX);
        const int y = yOffset + vertex.row * (cellH + gapY);
        painter.drawRoundedRect(x - 2, y - 2, cellW + 4, cellH + 4, 8, 8);
    };

    drawCell(hint.first);
    drawCell(hint.second);

    QPen linePen(lineColor, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(linePen);
    painter.setBrush(Qt::NoBrush);

    QVector<QPointF> points;
    points.reserve(hint.path.size());
    for(const Vertex& vertex : hint.path){
        points.push_back(QPointF(
            xOffset + vertex.col * (cellW + gapX) + cellW / 2.0,
            yOffset + vertex.row * (cellH + gapY) + cellH / 2.0
        ));
    }

    for(int i = 1; i < points.size(); ++i){
        painter.drawLine(points[i - 1], points[i]);
    }
}

void GameDialog::DrawTipLine(QPainter& painter, int xOffset, int yOffset, int cellW, int cellH, int gapX, int gapY){
    if(m_linkPath.size() < 2 || m_linkDrawProgress <= 0.0) return;
    const LLKAppSettings& settings = LLKAppSettings::instance();
    QPen linePen(settings.linkLineColor(), settings.linkLineWidth(), Qt::SolidLine);
    painter.setPen(linePen);

    QVector<QPointF> pathPoints;
    pathPoints.reserve(m_linkPath.size());
    for(const Vertex& vertex : m_linkPath){
        pathPoints.push_back(QPointF(
            xOffset + vertex.col * (cellW + gapX) + cellW / 2.0,
            yOffset + vertex.row * (cellH + gapY) + cellH / 2.0
        ));
    }

    qreal totalLength = 0.0;
    for(int i = 1; i < pathPoints.size(); ++i){
        totalLength += QLineF(pathPoints[i - 1], pathPoints[i]).length();
    }
    if(totalLength <= 0.0) return;

    const qreal targetLength = totalLength * m_linkDrawProgress;
    qreal drawnLength = 0.0;

    QPointF prevPoint = pathPoints[0];
    for(int i = 1; i < pathPoints.size(); ++i){
        const QPointF currPoint = pathPoints[i];
        const qreal segmentLength = QLineF(prevPoint, currPoint).length();
        if(segmentLength <= 0.0){
            prevPoint = currPoint;
            continue;
        }

        if(drawnLength + segmentLength <= targetLength){
            painter.drawLine(prevPoint, currPoint);
            drawnLength += segmentLength;
            prevPoint = currPoint;
            continue;
        }

        const qreal remainLength = targetLength - drawnLength;
        if(remainLength > 0.0){
            const qreal ratio = remainLength / segmentLength;
            const QPointF endPoint(
                prevPoint.x() + (currPoint.x() - prevPoint.x()) * ratio,
                prevPoint.y() + (currPoint.y() - prevPoint.y()) * ratio
            );
            painter.drawLine(prevPoint, endPoint);
        }
        break;
    }
}

void GameDialog::DrawNoticeOverlay(QPainter& painter){
    if(m_noticeText.isEmpty()){
        return;
    }

    qreal alpha = 0.8;
    qreal scale = 1.0;
    if(m_noticeProgress < 0.15){
        const qreal phase = m_noticeProgress / 0.15;
        alpha = phase;
        scale = 0.85 + 0.15 * phase;
    }else if(m_noticeProgress > 0.55){
        const qreal phase = (0.7 - m_noticeProgress) / 0.15;
        alpha = phase;
        scale = 0.85 + 0.15 * phase;
    }

    const QRect centerRect = QRect(0, 0, 240, 60);
    const QPoint centerPoint(width() / 2, height() / 2 - 50);

    painter.save();
    painter.translate(centerPoint);
    painter.scale(scale, scale);
    painter.translate(-centerRect.center());

    QColor fillColor(63, 40, 19, static_cast<int>(210 * alpha));
    QColor borderColor(255, 214, 156, static_cast<int>(240 * alpha));
    QColor textColor(255, 248, 235, static_cast<int>(255 * alpha));

    painter.setPen(QPen(borderColor, 3));
    painter.setBrush(fillColor);
    painter.drawRoundedRect(centerRect, 18, 18);

    QFont noticeFont("KaiTi", 18, QFont::Bold);
    painter.setFont(noticeFont);
    painter.setPen(textColor);
    painter.drawText(centerRect, Qt::AlignCenter, m_noticeText);

    painter.restore();
}
