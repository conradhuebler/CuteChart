/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtCore/QDebug>
#include <QtCore/QPointer>

#include <QtGui/QFontMetrics>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QTextDocument>

#include <QtWidgets/QGraphicsSceneMouseEvent>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>

#include "peakcallout.h"

PeakCallOut::PeakCallOut(QPointer<QChart> chart)
    : QGraphicsTextItem(chart)
    , m_chart(chart)
{
    m_color = QColor("black");
}

QRectF PeakCallOut::boundingRect() const
{
    QPointF anchor = mapFromParent(m_chart->mapToPosition(m_anchor));
    QRectF rect;
    rect.setLeft(qMin(m_rect.left(), anchor.x()));
    rect.setRight(qMax(m_rect.right(), anchor.x()));
    rect.setTop(qMin(m_rect.top(), anchor.y()));
    rect.setBottom(qMax(m_rect.bottom(), anchor.y()));
    return rect;
}

void PeakCallOut::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    int width = 40;
    int height = -100;
    if (!flip) {
        QFontMetrics fm = painter->fontMetrics();
        width = fm.horizontalAdvance(m_text) - fm.horizontalAdvance(m_text) / 2;
        height = -50;
    }
    qreal x = m_chart->mapToPosition(m_anchor).x() - width;
    qreal y = m_chart->mapToPosition(m_anchor).y() - height - 120;
    setPos(x, y);

    QGraphicsTextItem::paint(painter, option, widget);

    if (m_serie)
        setVisible(m_serie->isVisible());
}

void PeakCallOut::setColor(const QColor& color)
{
    m_color = color;
    Update();
}

void PeakCallOut::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    event->setAccepted(true);
}

void PeakCallOut::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        setPos(mapToParent(event->pos() - event->buttonDownPos(Qt::LeftButton)));
        event->setAccepted(true);
    } else {
        event->setAccepted(false);
    }
}

void PeakCallOut::setText(const QString& text, const QPointF& point)
{
    m_anchor = point;
    m_text = text;
    Update();
}

void PeakCallOut::Update()
{
    m_htmltext = tr("<h4><font color='%2'>%1</font></h4>").arg(m_text).arg(m_color.name());
    setHtml(m_htmltext);
    QFontMetrics metrics(font());
    QTextDocument doc;
    doc.setHtml(m_htmltext);
    m_textRect = metrics.boundingRect(QRect(0, 0, 300, 300), Qt::AlignLeft, m_htmltext);
    prepareGeometryChange();
    m_rect = m_textRect.adjusted(0, 0, 0, 0);
    if (doc.size().width() > 60) {
        setRotation(-90);
        flip = true;
    }
}

void PeakCallOut::setAnchor(QPointF point)
{
    m_anchor = point;
}
