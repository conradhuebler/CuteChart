/*
 * CuteCharts - An enhanced chart visualization framework based on Qt Charts
 * Copyright (C) 2017-2023 Conrad Hübler <Conrad.Huebler@gmx.net>
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
    , m_color(Qt::black)
{
    // Make callout interactive
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setCursor(Qt::OpenHandCursor);
}

QRectF PeakCallOut::boundingRect() const
{
    QPointF anchor = mapFromParent(m_chart->mapToPosition(m_anchor));
    QRectF rect;
    rect.setLeft(qMin(m_rect.left(), anchor.x()));
    rect.setRight(qMax(m_rect.right(), anchor.x()));
    rect.setTop(qMin(m_rect.top(), anchor.y()));
    rect.setBottom(qMax(m_rect.bottom(), anchor.y()));

    // Add some margin for the anchor line
    rect.adjust(-5, -5, 5, 5);

    return rect;
}

void PeakCallOut::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    // Calculate positioning based on available space
    int width = 40;
    int height = -100;

    if (!m_rotated) {
        QFontMetrics fm = painter->fontMetrics();
        width = fm.horizontalAdvance(m_text) - fm.horizontalAdvance(m_text) / 2;
        height = -50;
    }

    // Position callout relative to anchor point
    QPointF anchorPos = m_chart->mapToPosition(m_anchor);
    qreal x = anchorPos.x() - width;
    qreal y = anchorPos.y() - height - 50;
    setPos(x, y);

    // Draw connection line to anchor point
    painter->setPen(QPen(m_color, 1.5, Qt::DashLine));
    painter->drawLine(QPointF(width, height + 40), mapFromParent(anchorPos));

    // Draw text with background
    painter->setPen(QPen(QColor(60, 60, 60), 1));
    painter->setBrush(QBrush(QColor(240, 240, 240, 220)));
    painter->drawRoundedRect(boundingRect().adjusted(5, 5, -5, -5), 5, 5);

    // Let the base class handle text rendering
    QGraphicsTextItem::paint(painter, option, widget);

    // Hide callout if associated series is hidden
    if (m_series)
        setVisible(m_series->isVisible());
}

void PeakCallOut::setColor(const QColor& color)
{
    m_color = color;
    update();
}

void PeakCallOut::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    setCursor(Qt::ClosedHandCursor);
    event->accept();
}

void PeakCallOut::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        setPos(mapToParent(event->pos() - event->buttonDownPos(Qt::LeftButton)));
        event->accept();
    } else {
        event->ignore();
    }
}

void PeakCallOut::setText(const QString& text, const QPointF& point)
{
    m_anchor = point;
    m_text = text;
    update();
}

void PeakCallOut::update()
{
    m_htmlText = tr("<h4><font color='%2'>%1</font></h4>").arg(m_text).arg(m_color.name());
    setHtml(m_htmlText);

    QFontMetrics metrics(font());
    QTextDocument doc;
    doc.setHtml(m_htmlText);
    m_textRect = metrics.boundingRect(QRect(0, 0, 300, 300), Qt::AlignLeft, m_htmlText);
    prepareGeometryChange();
    m_rect = m_textRect.adjusted(-5, -5, 5, 5);

    // Rotate text for better readability with longer texts
    if (doc.size().width() > 60) {
        setRotation(-90);
        m_rotated = true;
    } else {
        setRotation(0);
        m_rotated = false;
    }
}

void PeakCallOut::setAnchor(QPointF point)
{
    m_anchor = point;
    update();
}
