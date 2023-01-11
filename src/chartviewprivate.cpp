/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2020 Conrad Hübler <Conrad.Huebler@gmx.net>
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
    , m_vertical_line_visible(false)
    , m_zoom_strategy(Z_None)
    , m_select_strategy(S_None)
    , m_chart(chart)

{
    setChart(chart);
    // setAcceptDrops(true);
    setRenderHint(QPainter::Antialiasing, true);
    setRubberBand(QChartView::NoRubberBand);
    /*QFont font = chart->titleFont();
    font.setBold(true);
    font.setPointSize(12);
    chart->setTitleFont(font);*/

    m_vertical_line = new QGraphicsLineItem(chart);
    QPen pen;
    pen.setWidth(1);
    pen.setColor(Qt::gray);
    m_vertical_line->setPen(pen);
    m_vertical_line->setLine(0, -1, 0, 10);
    m_vertical_line->show();

    m_line_position = new QGraphicsTextItem(chart);
    m_select_box = new QGraphicsRectItem(chart);
    m_select_box->setBrush(QColor::fromRgbF(0.68, 0.68, 0.67, 1));

    m_zoom_box = new QGraphicsRectItem(chart);
    m_zoom_box->setBrush(QColor::fromRgbF(0.18, 0.64, 0.71, 1));
    setMouseTracking(true);

    connect(this, &ChartViewPrivate::ZoomChanged, this, &ChartViewPrivate::UpdateLines);
}

void ChartViewPrivate::UpdateView(double min, double max)
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
    if ((CurrentZoomStrategy() == ZoomStrategy::Z_Horizontal /* && m_single_left_click */) || (CurrentSelectStrategy() == SelectStrategy::S_Horizontal /* && m_single_right_click */)) {
        if (m_box_started == false)
            inPoint.setY(m_upperleft.y());
        else
            inPoint.setY(m_lowerright.y());

    } else if ((CurrentZoomStrategy() == ZoomStrategy::Z_Vertical /* && m_single_left_click */) || (CurrentSelectStrategy() == SelectStrategy::S_Vertical /* && m_single_right_click */)) {
        if (m_box_started == false)
            inPoint.setX(m_upperleft.x());
        else
            inPoint.setX(m_lowerright.x());
    }

    return inPoint;
}

void ChartViewPrivate::setZoom(qreal x_min, qreal x_max, qreal y_min, qreal y_max)
{
    QPointer<QValueAxis> yaxis = qobject_cast<QValueAxis*>(chart()->axes(Qt::Vertical).first());
    if (!yaxis)
        return;

    QPointer<QValueAxis> xaxis = qobject_cast<QValueAxis*>(chart()->axes(Qt::Horizontal).first());
    if (!xaxis)
        return;

    yaxis->setMin(y_min);
    yaxis->setMax(y_max);
    yaxis->setTickInterval(ChartTools::ceil(y_max + y_min) / 10.0);
    m_y_min = y_min;
    m_y_max = y_max;

    xaxis->setMin(x_min);
    xaxis->setMax(x_max);
    xaxis->setTickInterval(ChartTools::ceil(x_max + x_min) / 10.0);

    m_x_min = x_min;
    m_x_max = x_max;
}

void ChartViewPrivate::setVerticalLineEnabled(bool enabled)
{
    m_line_position->setVisible(enabled);
    m_vertical_line->setVisible(enabled);
    m_vertical_line_visible = enabled;
}

void ChartViewPrivate::RectanglStart()
{
    QPointF inPoint = (mapFromGlobal(QCursor::pos()));

    m_saved_zoom_strategy = m_zoom_strategy;
    m_saved_select_strategy = m_select_strategy;

    m_rect_start = mapToPoint(inPoint);

    if (m_select_pending)
        m_select_box->setRect(m_rect_start.x(), m_rect_start.y(), 0, 0);
    else if (m_zoom_pending)
        m_zoom_box->setRect(m_rect_start.x(), m_rect_start.y(), 0, 0);

    m_vertical_line->hide();
    m_line_position->hide();
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

void ChartViewPrivate::UpdateCorner()
{
    m_upperleft = chart()->mapToPosition(QPointF(m_x_min, m_y_max));
    m_lowerright = chart()->mapToPosition(QPointF(m_x_max, m_y_min));
}

void ChartViewPrivate::mousePressEvent(QMouseEvent* event)
{
    UpdateCorner();

    if (event->button() == Qt::RightButton || event->buttons() == Qt::RightButton) {
        if (CurrentSelectStrategy() != SelectStrategy::S_None) {
            if (!m_select_pending) {
                m_single_right_click = true;
                m_select_pending = true;
                m_select_box->show();
                RectanglStart();
            }
        }
    } else if (event->button() == Qt::LeftButton || event->buttons() == Qt::LeftButton) {
        if (CurrentZoomStrategy() != ZoomStrategy::Z_None) {
            m_single_left_click = true;
            m_zoom_pending = true;
            m_zoom_box->show();
            RectanglStart();
        }

    } else if (event->button() == Qt::MiddleButton || event->buttons() == Qt::MiddleButton) {
        chart()->zoomReset();
        UpdateZoom();
    } else
        QChartView::mousePressEvent(event);
    event->ignore();
}

void ChartViewPrivate::wheelEvent(QWheelEvent* event)
{
    if (event->angleDelta().y() < 0)
        emit scaleDown();
    else
        emit scaleUp();

    QPointF inPoint;
    QPointF chartPoint;
    inPoint.setX(event->position().x());
    inPoint.setY(event->position().y());
    chartPoint = chart()->mapToValue(inPoint);

    event->ignore();
}

void ChartViewPrivate::setSelectBox(const QPointF& topleft, const QPointF& bottomright)
{
    UpdateCorner();

    m_border_start = chart()->mapToPosition(topleft);
    m_border_end = chart()->mapToPosition(bottomright);

    m_saved_zoom_strategy = m_zoom_strategy;
    m_saved_select_strategy = m_select_strategy;

    setZoomStrategy(ZoomStrategy::Z_None);

    m_select_pending = true;
    m_box_started = true;
    m_single_right_click = true;
    m_box_bounded = true;

    QRectF rect;
    rect = QRectF(chart()->mapToPosition(topleft), chart()->mapToPosition(bottomright));
    m_rect_start = chart()->mapToPosition(topleft);
    m_select_box->setRect(rect);
    m_select_box->show();
    setFocus();
}

void ChartViewPrivate::addHorizontalLine(double position_y)
{
    QGraphicsLineItem* line = new QGraphicsLineItem(m_chart);
    QPen pen;
    pen.setWidth(2);
    pen.setColor(Qt::darkGray);
    line->setPen(pen);
    line->show();

    QGraphicsTextItem* text = new QGraphicsTextItem(m_chart);

    m_horizontal_lines.insert(position_y, line);
    m_horizontal_lines_position.insert(position_y, text);
    UpdateLines();
}

void ChartViewPrivate::addVerticalLine(double position_y)
{
    QGraphicsLineItem* line = new QGraphicsLineItem(m_chart);
    QPen pen;
    pen.setWidth(2);
    pen.setColor(Qt::darkGray);
    line->setPen(pen);
    line->show();

    QGraphicsTextItem* text = new QGraphicsTextItem(m_chart);

    m_vertical_lines.insert(position_y, line);
    m_vertical_lines_position.insert(position_y, text);
    UpdateLines();
}

bool ChartViewPrivate::removeVerticalLine(double position_y)
{
    if (m_vertical_lines.contains(position_y)) {
        delete m_vertical_lines_position[position_y];
        delete m_vertical_lines[position_y];
        m_vertical_lines_position.remove(position_y);
        m_vertical_lines.remove(position_y);
        return true;
    } else
        return false;
}

bool ChartViewPrivate::removeHorizontalLine(double position_y)
{
    if (m_horizontal_lines.contains(position_y)) {
        delete m_horizontal_lines_position[position_y];
        delete m_horizontal_lines[position_y];
        m_horizontal_lines.remove(position_y);
        m_horizontal_lines_position.remove(position_y);
        return true;
    } else
        return false;
}

void ChartViewPrivate::removeAllHorizontalLines()
{
    qDeleteAll(m_horizontal_lines);
    qDeleteAll(m_horizontal_lines_position);
    m_horizontal_lines.clear();
    m_horizontal_lines_position.clear();
}

void ChartViewPrivate::removeAllVerticalLines()
{
    qDeleteAll(m_vertical_lines);
    qDeleteAll(m_vertical_lines_position);
    m_vertical_lines.clear();
    m_vertical_lines_position.clear();
}

void ChartViewPrivate::UpdateLines()
{
    auto keys = m_vertical_lines.keys();
    for (double x : keys) {
        QPointF start = chart()->mapToPosition(QPointF(x, m_y_min));
        QPointF end = chart()->mapToPosition(QPointF(x, 0.95 * m_y_max));

        m_vertical_lines[x]->setLine(start.x(), start.y(), end.x(), end.y());

        m_vertical_lines_position[x]->setPlainText(QString::number(x, 'f', m_horizontal_lines_prec));
        QPointF position = chart()->mapToPosition(QPointF(x, 0.99 * m_y_max));

        m_vertical_lines_position[x]->setPos(QPointF(position.x() + 20 * m_vertical_lines_position[x]->textWidth(), position.y()));

        m_vertical_lines_position[x]->setVisible(m_vertical_lines_prec != -1);
    }

    keys = m_horizontal_lines.keys();
    for (double y : keys) {
        QPointF start = chart()->mapToPosition(QPointF(m_x_min, y));
        QPointF end = chart()->mapToPosition(QPointF(0.95 * m_x_max, y));

        m_horizontal_lines[y]->setLine(start.x(), start.y(), end.x(), end.y());

        m_horizontal_lines_position[y]->setPos(chart()->mapToPosition(QPointF(y, 0.99 * m_y_max)));
        m_horizontal_lines_position[y]->setPlainText(QString::number(y, 'f', m_vertical_lines_prec));
        m_horizontal_lines_position[y]->setVisible(m_horizontal_lines_prec != -1);
    }
}

void ChartViewPrivate::mouseMoveEvent(QMouseEvent* event)
{
    if (this->chart()->axes(Qt::Horizontal).isEmpty() || this->chart()->axes(Qt::Vertical).isEmpty()) {
        return;
    }

    if (m_select_pending || m_zoom_pending) {
        QPair<QPointF, QPointF> inPoint = getCurrentRectangle();

        QRectF rect;
        rect = QRectF(inPoint.first, inPoint.second);
        if (m_zoom_pending)
            m_zoom_box->setRect(rect);
        else if (m_select_pending)
            m_select_box->setRect(rect);

        QChartView::mouseMoveEvent(event);
        return;
    }

    QPointF widgetPoint;
    QPointF chartPoint;
    widgetPoint.setX(event->x());
    widgetPoint.setY(event->y());
    chartPoint = chart()->mapToValue(widgetPoint);
    UpdateVerticalLine(chartPoint.x()); //, widgetPoint);
    QChartView::mouseMoveEvent(event);
}

void ChartViewPrivate::handleMouseMoved(const QPointF& ChartPoint)
{
    UpdateVerticalLine(ChartPoint.x());
}

void ChartViewPrivate::UpdateVerticalLine(double x)
{
    UpdateZoom();
    QPointF start = chart()->mapToPosition(QPointF(x, m_y_min));
    QPointF end = chart()->mapToPosition(QPointF(x, 0.95 * m_y_max));

    m_vertical_line->setLine(start.x(), start.y(), end.x(), end.y());
    m_line_position->setPlainText(QString::number(x, 'f', m_vertical_line_prec));
    QPointF position = chart()->mapToPosition(QPointF(x, 0.99 * m_y_max));
    m_line_position->setPos(position.x() + 20 * m_line_position->textWidth(), position.y());
}

void ChartViewPrivate::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton || event->buttons() == Qt::MiddleButton) {
        chart()->zoomReset();
        UpdateZoom();
        emit ZoomChanged();
    } else if (event->button() == Qt::RightButton || event->buttons() == Qt::RightButton) {
        if (m_select_pending) {
            m_single_right_click = false;
            m_select_pending = false;
            QPair<QPointF, QPointF> rect = getCurrentRectangle();

            if ((m_border_start.x() <= rect.first.x() && m_border_end.x() >= rect.second.x()) || !m_box_bounded) {
                emit AddRect(chart()->mapToValue(rect.first), chart()->mapToValue(rect.second));

            } else {
                emit AddRect(chart()->mapToValue(m_border_start), chart()->mapToValue(m_border_end));
            }
            m_vertical_line->setVisible(m_vertical_line_visible);
            m_line_position->setVisible(m_vertical_line_visible);
            m_select_box->hide();

            setSelectStrategy(m_saved_select_strategy);
            setZoomStrategy(m_saved_zoom_strategy);
            m_box_started = false;
            m_box_bounded = false;
        }
    } else if (event->button() == Qt::LeftButton || event->buttons() == Qt::LeftButton) {
        if (m_zoom_pending) {
            QPair<QPointF, QPointF> rect = getCurrentRectangle();
            chart()->zoomIn(QRectF(rect.first, rect.second));
            ZoomRect(chart()->mapToValue(rect.first), chart()->mapToValue(rect.second));
            UpdateZoom();
            emit ZoomChanged();
            m_zoom_pending = false;
            m_single_left_click = false;
            m_zoom_box->hide();
            m_vertical_line->setVisible(m_vertical_line_visible);
            m_line_position->setVisible(m_vertical_line_visible);
            m_box_started = false;
            m_box_bounded = false;
        }
    } else {
        QChartView::mouseReleaseEvent(event);
        if (chart()->axes(Qt::Vertical).isEmpty())
            return;

        QPointer<QValueAxis> yaxis = qobject_cast<QValueAxis*>(chart()->axes(Qt::Vertical).first());
        if (!yaxis)
            return;
        UpdateView(yaxis->min(), yaxis->max());
    }
}

void ChartViewPrivate::mouseDoubleClickEvent(QMouseEvent* event)
{
    UpdateCorner();

    if (event->button() == Qt::RightButton || event->buttons() == Qt::RightButton) {
        event->ignore();
    } else if (event->button() == Qt::LeftButton || event->buttons() == Qt::LeftButton) {
        QPointF inPoint;
        inPoint.setX(event->x());
        inPoint.setY(event->y());
        emit PointDoubleClicked(chart()->mapToValue(inPoint));
    } else
        event->ignore();
}

void ChartViewPrivate::UpdateZoom()
{
    if (!chart()->series().size())
        return;
    QPointer<QValueAxis> yaxis = qobject_cast<QValueAxis*>(chart()->axes(Qt::Vertical).first());
    if (!yaxis)
        return;

    QPointer<QValueAxis> xaxis = qobject_cast<QValueAxis*>(chart()->axes(Qt::Horizontal).first());
    if (!xaxis)
        return;

    m_y_min = yaxis->min();
    m_y_max = yaxis->max();

    m_x_min = xaxis->min();
    m_x_max = xaxis->max();

    /*
    double xinterval = ChartTools::IdealInterval(m_x_min, m_x_max ), yinterval = ChartTools::IdealInterval(m_y_min, m_y_max );

    xaxis->setTickInterval(xinterval);
    xaxis->setTickAnchor(m_x_min);
    yaxis->setTickInterval(yinterval);
    yaxis->setTickAnchor(m_y_min);

    qDebug() << xaxis->tickAnchor() << yaxis->tickAnchor();
    */
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
        m_select_box->hide();
        m_zoom_box->hide();
        m_vertical_line->setVisible(m_vertical_line_visible);
        m_line_position->setVisible(m_vertical_line_visible);
        emit EscapeSelectMode();
        break;
    case Qt::Key_Left:
        emit LeftKey();
        break;
    case Qt::Key_Right:
        emit RightKey();
        break;
    default:
        QGraphicsView::keyPressEvent(event);
        break;
    }
}
