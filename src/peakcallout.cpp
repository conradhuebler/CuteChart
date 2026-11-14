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

#include <algorithm>
#include <QtCore/QPointer>

#include <QtGui/QFontMetrics>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QTextDocument>
#include <QtWidgets/QStyleOptionGraphicsItem>

#include <QtWidgets/QGraphicsSceneMouseEvent>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>

#include "peakcallout.h"

PeakCallOut::PeakCallOut(QPointer<QChart> chart)
    : QGraphicsTextItem(chart)
    , m_chart(chart)
    , m_color(Qt::black)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setCursor(Qt::OpenHandCursor);
    document()->setDocumentMargin(3);
}

QPointF PeakCallOut::anchorPosition() const
{
    return m_chart ? m_chart->mapToPosition(m_anchor) : QPointF();
}

QRectF PeakCallOut::cardRect() const
{
    return QRectF(pos(), QGraphicsTextItem::boundingRect().size());
}

QPainterPath PeakCallOut::shape() const
{
    QPainterPath path;
    path.addRect(QGraphicsTextItem::boundingRect());
    return path;
}

QRectF PeakCallOut::boundingRect() const
{
    // Card plus the connection line to the anchor
    const QRectF card = QGraphicsTextItem::boundingRect();
    const QPointF anchor = mapFromParent(anchorPosition());
    return card.united(QRectF(anchor, QSizeF(0, 0))).adjusted(-5, -5, 5, 5);
}

void PeakCallOut::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing);
    const QRectF card = QGraphicsTextItem::boundingRect();
    const QPointF anchor = mapFromParent(anchorPosition());

    // Thin connection from the card to the peak, ending in a small dot
    painter->setPen(QPen(m_color, 1.0));
    painter->drawLine(card.center(), anchor);
    painter->setBrush(m_color);
    painter->drawEllipse(anchor, 2.5, 2.5);

    painter->setPen(QPen(m_color, 1));
    painter->setBrush(QBrush(QColor(250, 250, 250, 235)));
    painter->drawRoundedRect(card, 4, 4);

    QStyleOptionGraphicsItem opt(*option);
    opt.state &= ~QStyle::State_Selected; // no dotted selection frame
    QGraphicsTextItem::paint(painter, &opt, widget);
}

void PeakCallOut::setColor(const QColor& color)
{
    m_color = color;
    update();
}

void PeakCallOut::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    setCursor(Qt::ClosedHandCursor);
    m_dragging = true;
    QGraphicsTextItem::mousePressEvent(event);
    event->accept();
}

void PeakCallOut::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsTextItem::mouseMoveEvent(event);
    // Remember the dragged position as offset to the anchor so it follows zoom/pan
    const QSizeF s = QGraphicsTextItem::boundingRect().size();
    prepareGeometryChange();
    m_offset = pos() + QPointF(s.width() / 2, s.height()) - anchorPosition();
    m_userMoved = true;
}

void PeakCallOut::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    m_dragging = false;
    setCursor(Qt::OpenHandCursor);
    QGraphicsTextItem::mouseReleaseEvent(event);
}

void PeakCallOut::setText(const QString& text, const QPointF& point)
{
    m_anchor = point;
    m_text = text;
    update();
}

void PeakCallOut::update()
{
    // Without size-hints from HTML tags: the document knows the real extent
    setHtml(QString("<span style='color:%2; font-weight:bold'>%1</span>").arg(m_text, m_color.name()));
    prepareGeometryChange();
    reposition();
    QGraphicsTextItem::update();
}

void PeakCallOut::reposition()
{
    if (!m_chart || m_dragging)
        return;
    const QSizeF s = QGraphicsTextItem::boundingRect().size();
    prepareGeometryChange();
    setPos(anchorPosition() + m_offset - QPointF(s.width() / 2, s.height()));
}

void PeakCallOut::setAnchor(QPointF point)
{
    m_anchor = point;
    update();
}

void PeakCallOut::arrange(const QList<PeakCallOut*>& callouts)
{
    QList<PeakCallOut*> free;
    QList<QRectF> placed;
    for (PeakCallOut* c : callouts) {
        if (!c || !c->isVisible() || !c->m_chart)
            continue;
        c->reposition();
        if (c->m_userMoved)
            placed << c->cardRect();
        else
            free << c;
    }
    if (free.isEmpty())
        return;

    std::sort(free.begin(), free.end(), [](PeakCallOut* a, PeakCallOut* b) {
        return a->anchorPosition().x() < b->anchorPosition().x();
    });

    const QRectF bounds = free.first()->m_chart->rect();
    const qreal gap = 24, spacing = 4;
    for (PeakCallOut* c : free) {
        const QSizeF s = c->QGraphicsTextItem::boundingRect().size();
        const QPointF a = c->anchorPosition();
        qreal x = qBound(bounds.left() + 2, a.x() - s.width() / 2, qMax(bounds.left() + 2, bounds.right() - s.width() - 2));
        QRectF r;
        qreal y = a.y() - gap - s.height();
        for (int level = 0; level < 40; ++level) {
            r = QRectF(QPointF(x, y), s);
            bool hit = false;
            for (const QRectF& p : placed)
                if (r.adjusted(-spacing, -spacing, spacing, spacing).intersects(p)) {
                    hit = true;
                    y = qMin(y, p.top() - spacing - s.height()); // stack above the blocker
                    break;
                }
            if (!hit)
                break;
        }
        placed << r;
        c->prepareGeometryChange();
        c->m_offset = QPointF(r.center().x() - a.x(), r.bottom() - a.y());
        c->setPos(r.topLeft());
    }
}
