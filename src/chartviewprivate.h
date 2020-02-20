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

#pragma once
#include <QtWidgets/QGraphicsTextItem>

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QtCore/QDebug>
#include <QtCore/QPointer>

#include <QtWidgets/QScrollArea>

class QGridLayout;
class QPushButton;
class QChart;

class PeakCallOut;

struct ChartConfig;

enum ZoomStrategy {
    Z_None = 0,
    Z_Horizontal = 1,
    Z_Vertical = 2,
    Z_Rectangular = 3
};

enum SelectStrategy {
    S_None = 0,
    S_Horizontal = 1,
    S_Vertical = 2,
    S_Rectangular = 3
};

enum AutoScaleStrategy {
    QtNiceNumbers = 0,
    SpaceScale = 1
};

class ChartViewPrivate : public QtCharts::QChartView {
    Q_OBJECT
public:
    ChartViewPrivate(QtCharts::QChart* chart, QWidget* parent = Q_NULLPTR);

    inline ~ChartViewPrivate() override {}

    inline void setZoomStrategy(ZoomStrategy strategy) { m_zoom_strategy = strategy; }
    inline void setSelectStrategy(SelectStrategy strategy) { m_select_strategy = strategy; }

    inline ZoomStrategy CurrentZoomStrategy() const { return m_zoom_strategy; }
    inline SelectStrategy CurrentSelectStrategy() const { return m_select_strategy; }

    inline bool isVerticalLineEnabled() const { return m_vertical_line_visible; }

    QPointF currentMousePosition() const;
public slots:
    void UpdateVerticalLine(double x);

    void UpdateView(double min, double max);

    void setVerticalLineEnabled(bool enabled);

    void setZoom(qreal x_min, qreal x_max, qreal y_min, qreal y_max);
    void UpdateZoom();

    void setSelectBox(const QPointF& topleft, const QPointF& bottomright);

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void handleMouseMoved(const QPointF& ChartPoint);

    void RectanglStart();
    QPair<QPointF, QPointF> getCurrentRectangle();
    QPointF mapToPoint(QMouseEvent* event) const;
    QPointF mapToPoint(const QPointF& event) const;

    QPointF m_border_start, m_border_end;

    void UpdateCorner();

    QGraphicsLineItem* m_vertical_line;
    QGraphicsTextItem* m_line_position;
    QGraphicsRectItem *m_zoom_box, *m_select_box;

    QPointF m_rect_start, m_upperleft, m_lowerright;
    double m_x_min, m_x_max, m_y_min, m_y_max;

    bool m_single_left_click = false, m_single_right_click = false, m_double_right_clicked = false, m_vertical_line_visible = false, m_zoom_pending = false, m_select_pending = false;
    bool m_box_started = false, m_box_bounded = false;

    ZoomStrategy m_zoom_strategy, m_saved_zoom_strategy;
    SelectStrategy m_select_strategy, m_saved_select_strategy;
signals:
    void LockZoom();
    void UnLockZoom();
    void ZoomChanged();
    void scaleUp();
    void scaleDown();
    void AddRect(const QPointF& point1, const QPointF& point2);
    void ZoomRect(const QPointF& point1, const QPointF& point2);
    void PointDoubleClicked(const QPointF& point);
    void EscapeSelectMode();
    void RightKey();
    void LeftKey();
};
