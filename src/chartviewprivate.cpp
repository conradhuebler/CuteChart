/*
 * CuteCharts - An enhanced chart visualization framework based on Qt Charts
 * Copyright (C) 2020-2023 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 *
 */

#include "chartconfig.h"
#include "peakcallout.h"
#include "series.h"
#include "tools.h"

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QXYSeries>

#include <QtCore/QBuffer>
#include <QtCore/QDebug>
#include <QtCore/QMimeData>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>

#include <QtGui/QCursor>
#include <QtGui/QDrag>

#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>

#include <cmath>
#include <iostream>

#include "chartviewprivate.h"

ChartViewPrivate::ChartViewPrivate(QChart* chart, QWidget* parent)
    : QChartView(parent)
    , m_chart(chart)
{
    setChart(chart);
    setRenderHint(QPainter::Antialiasing, true);
    setRubberBand(QChartView::NoRubberBand);

    // Create tracking vertical line
    m_vertical_line = std::make_unique<QGraphicsLineItem>(chart);
    QPen pen;
    pen.setWidth(1);
    pen.setColor(Qt::gray);
    m_vertical_line->setPen(pen);
    m_vertical_line->setLine(0, -1, 0, 10);
    m_vertical_line->show();

    // Create other graphic items
    m_line_position = std::make_unique<QGraphicsTextItem>(chart);
    m_select_box = std::make_unique<QGraphicsRectItem>(chart);
    m_select_box->setBrush(QColor::fromRgbF(0.68, 0.68, 0.67, 0.6)); // Semi-transparent selection

    m_zoom_box = std::make_unique<QGraphicsRectItem>(chart);
    m_zoom_box->setBrush(QColor::fromRgbF(0.18, 0.64, 0.71, 0.6)); // Semi-transparent zoom box
    setMouseTracking(true);

    connect(this, &ChartViewPrivate::zoomChanged, this, &ChartViewPrivate::updateLines);
}

ChartViewPrivate::~ChartViewPrivate()
{
    // Smart pointers will handle cleanup automatically
    removeAllHorizontalLines();
    removeAllVerticalLines();
}

void ChartViewPrivate::setZoomStrategy(ZoomStrategy strategy)
{
    m_zoom_strategy = strategy;
}

void ChartViewPrivate::setSelectStrategy(SelectStrategy strategy)
{
    m_select_strategy = strategy;
}

ZoomStrategy ChartViewPrivate::currentZoomStrategy() const
{
    return m_zoom_strategy;
}

SelectStrategy ChartViewPrivate::currentSelectStrategy() const
{
    return m_select_strategy;
}

bool ChartViewPrivate::isVerticalLineEnabled() const
{
    return m_vertical_line_visible;
}

void ChartViewPrivate::setHorizontalLinesPrec(int prec)
{
    m_horizontal_lines_prec = prec;
    updateLines();
}

void ChartViewPrivate::setVerticalLinesPrec(int prec)
{
    m_vertical_lines_prec = prec;
    updateLines();
}

void ChartViewPrivate::setVerticalLinePrec(int prec)
{
    m_vertical_line_prec = prec;
    // Update vertical line if visible
    if (m_vertical_line_visible) {
        updateVerticalLine(chart()->mapToValue(currentMousePosition()).x());
    }
}

void ChartViewPrivate::updateView(double min, double max)
{
    m_y_min = min;
    m_y_max = max;
}

QPointF ChartViewPrivate::mapToPoint(QMouseEvent* event) const
{
    return mapToPoint(QPointF(event->x(), event->y()));
}

QPointF ChartViewPrivate::mapToPoint(const QPointF& point) const
{
    QPointF inPoint(point);
    if ((currentZoomStrategy() == ZoomStrategy::Horizontal) || (currentSelectStrategy() == SelectStrategy::Horizontal)) {
        if (!m_box_started)
            inPoint.setY(m_upperleft.y());
        else
            inPoint.setY(m_lowerright.y());

    } else if ((currentZoomStrategy() == ZoomStrategy::Vertical) || (currentSelectStrategy() == SelectStrategy::Vertical)) {
        if (!m_box_started)
            inPoint.setX(m_upperleft.x());
        else
            inPoint.setX(m_lowerright.x());
    }

    return inPoint;
}

void ChartViewPrivate::setZoom(qreal x_min, qreal x_max, qreal y_min, qreal y_max)
{
    QValueAxis* yaxis = qobject_cast<QValueAxis*>(chart()->axes(Qt::Vertical).first());
    if (!yaxis)
        return;

    QValueAxis* xaxis = qobject_cast<QValueAxis*>(chart()->axes(Qt::Horizontal).first());
    if (!xaxis)
        return;

    yaxis->setMin(y_min);
    yaxis->setMax(y_max);
    yaxis->setTickInterval(ChartTools::CustomCeil(y_max + y_min) / 10.0);
    m_y_min = y_min;
    m_y_max = y_max;

    xaxis->setMin(x_min);
    xaxis->setMax(x_max);
    xaxis->setTickInterval(ChartTools::CustomCeil(x_max + x_min) / 10.0);

    m_x_min = x_min;
    m_x_max = x_max;

    emit zoomChanged();
}

void ChartViewPrivate::setVerticalLineEnabled(bool enabled)
{
    m_line_position->setVisible(enabled);
    m_vertical_line->setVisible(enabled);
    m_vertical_line_visible = enabled;
}

void ChartViewPrivate::rectangleStart()
{
    QPointF inPoint = (mapFromGlobal(QCursor::pos()));

    m_saved_zoom_strategy = m_zoom_strategy;
    m_saved_select_strategy = m_select_strategy;

    m_rect_start = mapToPoint(inPoint);

    if (m_select_pending)
        m_select_box->setRect(m_rect_start.x(), m_rect_start.y(), 0, 0);
    else if (m_zoom_pending)
        m_zoom_box->setRect(m_rect_start.x(), m_rect_start.y(), 0, 0);

    m_vertical_line->setVisible(false);
    m_line_position->setVisible(false);
    m_box_started = true;
}

QPair<QPointF, QPointF> ChartViewPrivate::getCurrentRectangle()
{
    QPointF inPoint = mapToPoint(mapFromGlobal(QCursor::pos()));

    QPointF left, right;
    left.setX(qMin(inPoint.x(), m_rect_start.x()));
    right.setX(qMax(inPoint.x(), m_rect_start.x()));

    left.setY(qMin(inPoint.y(), m_rect_start.y()));
    right.setY(qMax(inPoint.y(), m_rect_start.y()));

    if (m_box_bounded) {
        if ((right.x() > m_border_end.x() || right.x() < m_border_start.x())
            || (right.y() > m_border_end.y() || right.y() < m_border_start.y())
            || (left.x() > m_border_end.x() || left.x() < m_border_start.x())
            || (left.y() > m_border_end.y() || left.y() < m_border_start.y())) {
            right = m_border_end;
            left = m_border_start;
        }
    }

    return QPair<QPointF, QPointF>(left, right);
}

QPointF ChartViewPrivate::currentMousePosition() const
{
    return (mapFromGlobal(QCursor::pos()));
}

void ChartViewPrivate::updateCorner()
{
    m_upperleft = chart()->mapToPosition(QPointF(m_x_min, m_y_max));
    m_lowerright = chart()->mapToPosition(QPointF(m_x_max, m_y_min));
}

void ChartViewPrivate::mousePressEvent(QMouseEvent* event)
{
    updateCorner();

    if (event->button() == Qt::RightButton) {
        if (currentSelectStrategy() != SelectStrategy::None) {
            if (!m_select_pending) {
                m_single_right_click = true;
                m_select_pending = true;
                m_select_box->setVisible(true);
                rectangleStart();
            }
        }
    } else if (event->button() == Qt::LeftButton) {
        if (currentZoomStrategy() != ZoomStrategy::None) {
            m_single_left_click = true;
            m_zoom_pending = true;
            m_zoom_box->setVisible(true);
            rectangleStart();
        }
    } else if (event->button() == Qt::MiddleButton) {
        chart()->zoomReset();
        updateZoom();
    } else {
        QChartView::mousePressEvent(event);
    }
    event->ignore();
}

void ChartViewPrivate::wheelEvent(QWheelEvent* event)
{
    if (event->angleDelta().y() < 0)
        emit scaleDown();
    else
        emit scaleUp();

    // Get position under cursor for context-aware zooming
    QPointF chartPoint = chart()->mapToValue(QPointF(event->position().x(), event->position().y()));

    event->ignore();
}

void ChartViewPrivate::setSelectBox(const QPointF& topleft, const QPointF& bottomright)
{
    updateCorner();

    m_border_start = chart()->mapToPosition(topleft);
    m_border_end = chart()->mapToPosition(bottomright);

    m_saved_zoom_strategy = m_zoom_strategy;
    m_saved_select_strategy = m_select_strategy;

    setZoomStrategy(ZoomStrategy::None);

    m_select_pending = true;
    m_box_started = true;
    m_single_right_click = true;
    m_box_bounded = true;

    QRectF rect;
    rect = QRectF(chart()->mapToPosition(topleft), chart()->mapToPosition(bottomright));
    m_rect_start = chart()->mapToPosition(topleft);
    m_select_box->setRect(rect);
    m_select_box->setVisible(true);
    setFocus();
}

void ChartViewPrivate::addHorizontalLine(double position_y)
{
    auto line = std::make_unique<QGraphicsLineItem>(m_chart);
    QPen pen;
    pen.setWidth(2);
    pen.setColor(Qt::darkGray);
    line->setPen(pen);
    line->setVisible(true);

    auto text = std::make_unique<QGraphicsTextItem>(m_chart);

    m_horizontal_lines.emplace(position_y, std::move(line));
    m_horizontal_lines_position.emplace(position_y, std::move(text));
    updateLines();
}

void ChartViewPrivate::addVerticalLine(double position_x)
{
    auto line = std::make_unique<QGraphicsLineItem>(m_chart);
    QPen pen;
    pen.setWidth(2);
    pen.setColor(Qt::darkGray);
    line->setPen(pen);
    line->setVisible(true);

    auto text = std::make_unique<QGraphicsTextItem>(m_chart);

    m_vertical_lines.emplace(position_x, std::move(line));
    m_vertical_lines_position.emplace(position_x, std::move(text));
    updateLines();
}

bool ChartViewPrivate::removeVerticalLine(double position_x)
{
    if (m_vertical_lines.find(position_x) != m_vertical_lines.end()) {
        m_vertical_lines.erase(position_x);
        m_vertical_lines_position.erase(position_x);
        return true;
    }
    return false;
}

bool ChartViewPrivate::removeHorizontalLine(double position_y)
{
    if (m_horizontal_lines.find(position_y) != m_horizontal_lines.end()) {
        m_horizontal_lines.erase(position_y);
        m_horizontal_lines_position.erase(position_y);
        return true;
    }
    return false;
}

void ChartViewPrivate::removeAllHorizontalLines()
{
    m_horizontal_lines.clear();
    m_horizontal_lines_position.clear();
}

void ChartViewPrivate::removeAllVerticalLines()
{
    m_vertical_lines.clear();
    m_vertical_lines_position.clear();
}

void ChartViewPrivate::updateLines()
{
    // Update vertical marker lines
    for (const auto& pair : m_vertical_lines) {
        double x = pair.first;
        auto& line = pair.second;

        QPointF start = chart()->mapToPosition(QPointF(x, m_y_min));
        QPointF end = chart()->mapToPosition(QPointF(x, 0.95 * m_y_max));

        line->setLine(start.x(), start.y(), end.x(), end.y());

        auto& textItem = m_vertical_lines_position[x];
        textItem->setPlainText(QString::number(x, 'f', m_horizontal_lines_prec));
        QPointF position = chart()->mapToPosition(QPointF(x, 0.99 * m_y_max));

        textItem->setPos(QPointF(position.x() + 20 * textItem->document()->size().width(), position.y()));
        textItem->setVisible(m_vertical_lines_prec != -1);
    }

    // Update horizontal marker lines
    for (const auto& pair : m_horizontal_lines) {
        double y = pair.first;
        auto& line = pair.second;

        QPointF start = chart()->mapToPosition(QPointF(m_x_min, y));
        QPointF end = chart()->mapToPosition(QPointF(0.95 * m_x_max, y));

        line->setLine(start.x(), start.y(), end.x(), end.y());

        auto& textItem = m_horizontal_lines_position[y];
        textItem->setPlainText(QString::number(y, 'f', m_vertical_lines_prec));
        QPointF position = chart()->mapToPosition(QPointF(m_x_min, y));

        textItem->setPos(position.x() - 20 * textItem->document()->size().width(),
            position.y() - 10);
        textItem->setVisible(m_horizontal_lines_prec != -1);
    }
}

void ChartViewPrivate::mouseMoveEvent(QMouseEvent* event)
{
    if (chart()->axes(Qt::Horizontal).isEmpty() || chart()->axes(Qt::Vertical).isEmpty()) {
        return;
    }

    if (m_select_pending || m_zoom_pending) {
        QPair<QPointF, QPointF> inPoint = getCurrentRectangle();

        QRectF rect = QRectF(inPoint.first, inPoint.second);
        if (m_zoom_pending)
            m_zoom_box->setRect(rect);
        else if (m_select_pending)
            m_select_box->setRect(rect);

        QChartView::mouseMoveEvent(event);
        return;
    }

    QPointF chartPoint = chart()->mapToValue(QPointF(event->x(), event->y()));
    updateVerticalLine(chartPoint.x());
    QChartView::mouseMoveEvent(event);
}

void ChartViewPrivate::handleMouseMoved(const QPointF& chartPoint)
{
    updateVerticalLine(chartPoint.x());
}

void ChartViewPrivate::updateVerticalLine(double x)
{
    updateZoom();
    QPointF start = chart()->mapToPosition(QPointF(x, m_y_min));
    QPointF end = chart()->mapToPosition(QPointF(x, 0.95 * m_y_max));

    m_vertical_line->setLine(start.x(), start.y(), end.x(), end.y());
    m_line_position->setPlainText(QString::number(x, 'f', m_vertical_line_prec));
    QPointF position = chart()->mapToPosition(QPointF(x, 0.99 * m_y_max));
    m_line_position->setPos(position.x() - m_line_position->document()->size().width() / 2, position.y() - 20);
}

void ChartViewPrivate::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        chart()->zoomReset();
        updateZoom();
        emit zoomChanged();
    } else if (event->button() == Qt::RightButton) {
        if (m_select_pending) {
            m_single_right_click = false;
            m_select_pending = false;
            QPair<QPointF, QPointF> rect = getCurrentRectangle();

            if ((m_border_start.x() <= rect.first.x() && m_border_end.x() >= rect.second.x()) || !m_box_bounded) {
                emit addRect(chart()->mapToValue(rect.first), chart()->mapToValue(rect.second));
            } else {
                emit addRect(chart()->mapToValue(m_border_start), chart()->mapToValue(m_border_end));
            }

            m_vertical_line->setVisible(m_vertical_line_visible);
            m_line_position->setVisible(m_vertical_line_visible);
            m_select_box->setVisible(false);

            setSelectStrategy(m_saved_select_strategy);
            setZoomStrategy(m_saved_zoom_strategy);
            m_box_started = false;
            m_box_bounded = false;
        }
    } else if (event->button() == Qt::LeftButton) {
        if (m_zoom_pending) {
            QPair<QPointF, QPointF> rect = getCurrentRectangle();
            chart()->zoomIn(QRectF(rect.first, rect.second));
            emit zoomRect(chart()->mapToValue(rect.first), chart()->mapToValue(rect.second));
            updateZoom();
            emit zoomChanged();
            m_zoom_pending = false;
            m_single_left_click = false;
            m_zoom_box->setVisible(false);
            m_vertical_line->setVisible(m_vertical_line_visible);
            m_line_position->setVisible(m_vertical_line_visible);
            m_box_started = false;
            m_box_bounded = false;
        }
    } else {
        QChartView::mouseReleaseEvent(event);
        if (chart()->axes(Qt::Vertical).isEmpty())
            return;

        QValueAxis* yaxis = qobject_cast<QValueAxis*>(chart()->axes(Qt::Vertical).first());
        if (!yaxis)
            return;

        updateView(yaxis->min(), yaxis->max());
    }
}

void ChartViewPrivate::mouseDoubleClickEvent(QMouseEvent* event)
{
    updateCorner();

    if (event->button() == Qt::RightButton) {
        event->ignore();
    } else if (event->button() == Qt::LeftButton) {
        QPointF chartPoint = chart()->mapToValue(QPointF(event->x(), event->y()));
        emit pointDoubleClicked(chartPoint);
    } else {
        event->ignore();
    }
}

void ChartViewPrivate::updateZoom()
{
    if (chart()->series().isEmpty())
        return;

    QValueAxis* yaxis = qobject_cast<QValueAxis*>(chart()->axes(Qt::Vertical).first());
    if (!yaxis)
        return;

    QValueAxis* xaxis = qobject_cast<QValueAxis*>(chart()->axes(Qt::Horizontal).first());
    if (!xaxis)
        return;

    m_y_min = yaxis->min();
    m_y_max = yaxis->max();

    m_x_min = xaxis->min();
    m_x_max = xaxis->max();
}

void ChartViewPrivate::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        m_double_right_clicked = false;
        m_zoom_pending = false;
        m_select_pending = false;
        m_single_left_click = false;
        m_single_right_click = false;
        m_select_box->setVisible(false);
        m_zoom_box->setVisible(false);
        m_vertical_line->setVisible(m_vertical_line_visible);
        m_line_position->setVisible(m_vertical_line_visible);
        emit escapeSelectMode();
        break;
    case Qt::Key_Left:
        emit leftKey();
        break;
    case Qt::Key_Right:
        emit rightKey();
        break;
    default:
        QGraphicsView::keyPressEvent(event);
        break;
    }
}
