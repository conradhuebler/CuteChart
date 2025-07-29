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

#pragma once
#include <QtWidgets/QGraphicsTextItem>

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QtCore/QDebug>
#include <QtCore/QPointer>

#include <QtWidgets/QScrollArea>

#include <memory>
#include <unordered_map>

class QGridLayout;
class QPushButton;

class PeakCallOut;

struct ChartConfig;

enum class ZoomStrategy {
    None = 0,
    Horizontal = 1,
    Vertical = 2,
    Rectangular = 3
};

enum class SelectStrategy {
    None = 0,
    Horizontal = 1,
    Vertical = 2,
    Rectangular = 3
};

enum class AutoScaleStrategy {
    QtNiceNumbers = 0,
    SpaceScale = 1
};

class ChartViewPrivate : public QChartView {
    Q_OBJECT

public:
    /**
     * @brief Constructor for ChartViewPrivate
     * @param chart The chart to be displayed
     * @param parent The parent widget
     */
    explicit ChartViewPrivate(QChart* chart, QWidget* parent = nullptr);

    /**
     * @brief Destructor - cleans up all graphics items
     */
    ~ChartViewPrivate() override;

    /**
     * @brief Set the zoom strategy
     * @param strategy The zoom strategy to use
     */
    void setZoomStrategy(ZoomStrategy strategy);

    /**
     * @brief Set the selection strategy
     * @param strategy The selection strategy to use
     */
    void setSelectStrategy(SelectStrategy strategy);

    /**
     * @brief Get current zoom strategy
     * @return Current zoom strategy
     */
    ZoomStrategy currentZoomStrategy() const;

    /**
     * @brief Get current selection strategy
     * @return Current selection strategy
     */
    SelectStrategy currentSelectStrategy() const;

    /**
     * @brief Check if vertical line is enabled
     * @return True if vertical line is visible
     */
    bool isVerticalLineEnabled() const;

    /**
     * @brief Get current mouse position on the chart
     * @return Current mouse position
     */
    QPointF currentMousePosition() const;

    /**
     * @brief Add a vertical line at the specified X position
     * @param position_x X position for the line
     */
    void addVerticalLine(double position_x);

    /**
     * @brief Remove vertical line at specified X position
     * @param position_x X position of the line to remove
     * @return True if line was successfully removed
     */
    bool removeVerticalLine(double position_x);

    /**
     * @brief Remove all vertical lines
     */
    void removeAllVerticalLines();

    /**
     * @brief Add a horizontal line at the specified Y position
     * @param position_y Y position for the line
     */
    void addHorizontalLine(double position_y);

    /**
     * @brief Remove horizontal line at specified Y position
     * @param position_y Y position of the line to remove
     * @return True if line was successfully removed
     */
    bool removeHorizontalLine(double position_y);

    /**
     * @brief Remove all horizontal lines
     */
    void removeAllHorizontalLines();

    /**
     * @brief Set decimal precision for horizontal line labels
     * @param prec Number of decimal places to show
     */
    void setHorizontalLinesPrec(int prec);

    /**
     * @brief Set decimal precision for vertical line labels
     * @param prec Number of decimal places to show
     */
    void setVerticalLinesPrec(int prec);

    /**
     * @brief Set decimal precision for the tracking vertical line
     * @param prec Number of decimal places to show
     */
    void setVerticalLinePrec(int prec);

public slots:
    /**
     * @brief Update the position of the tracking vertical line
     * @param x X position where the line should be drawn
     */
    void updateVerticalLine(double x);

    /**
     * @brief Update the view with new Y axis limits
     * @param min Minimum Y value
     * @param max Maximum Y value
     */
    void updateView(double min, double max);

    /**
     * @brief Enable or disable the vertical tracking line
     * @param enabled True to show the line
     */
    void setVerticalLineEnabled(bool enabled);

    /**
     * @brief Set explicit zoom area
     * @param x_min Minimum X value
     * @param x_max Maximum X value
     * @param y_min Minimum Y value
     * @param y_max Maximum Y value
     */
    void setZoom(qreal x_min, qreal x_max, qreal y_min, qreal y_max);

    /**
     * @brief Update internal zoom state from current chart axes
     */
    void updateZoom();

    /**
     * @brief Set selection box with given coordinates
     * @param topleft Top left corner of selection in chart coordinates
     * @param bottomright Bottom right corner of selection in chart coordinates
     */
    void setSelectBox(const QPointF& topleft, const QPointF& bottomright);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    /**
     * @brief Handle mouse movement and update tracking elements
     * @param chartPoint Point in chart coordinates
     */
    void handleMouseMoved(const QPointF& chartPoint);

    /**
     * @brief Start rectangle (zoom or selection) at current mouse position
     */
    void rectangleStart();

    /**
     * @brief Get the current rectangle based on start point and current mouse position
     * @return Pair of points representing the rectangle (topleft, bottomright)
     */
    QPair<QPointF, QPointF> getCurrentRectangle();

    /**
     * @brief Map screen coordinates to chart coordinates with constraints based on current mode
     * @param event Mouse event to map
     * @return Point in chart coordinates
     */
    QPointF mapToPoint(QMouseEvent* event) const;

    /**
     * @brief Map screen coordinates to chart coordinates with constraints based on current mode
     * @param point Screen point to map
     * @return Point in chart coordinates
     */
    QPointF mapToPoint(const QPointF& point) const;

    /**
     * @brief Update the corner points of the chart view
     */
    void updateCorner();

    /**
     * @brief Update the positions of all marker lines
     */
    void updateLines();

    // Graphics items for tracking and visualization
    std::unique_ptr<QGraphicsLineItem> m_vertical_line;
    std::unique_ptr<QGraphicsTextItem> m_line_position;
    std::unique_ptr<QGraphicsRectItem> m_zoom_box;
    std::unique_ptr<QGraphicsRectItem> m_select_box;

    // Collections of marker lines and their labels
    std::unordered_map<double, std::unique_ptr<QGraphicsLineItem>> m_vertical_lines;
    std::unordered_map<double, std::unique_ptr<QGraphicsLineItem>> m_horizontal_lines;
    std::unordered_map<double, std::unique_ptr<QGraphicsTextItem>> m_vertical_lines_position;
    std::unordered_map<double, std::unique_ptr<QGraphicsTextItem>> m_horizontal_lines_position;

    // Rectangle selection boundaries
    QPointF m_border_start, m_border_end;
    QPointF m_rect_start, m_upperleft, m_lowerright;

    // Current axis limits
    double m_x_min, m_x_max, m_y_min, m_y_max;

    // Precision for number display
    int m_horizontal_lines_prec = 2;
    int m_vertical_lines_prec = 2;
    int m_vertical_line_prec = 4;

    // State flags
    bool m_single_left_click = false;
    bool m_single_right_click = false;
    bool m_double_right_clicked = false;
    bool m_vertical_line_visible = false;
    bool m_zoom_pending = false;
    bool m_select_pending = false;
    bool m_box_started = false;
    bool m_box_bounded = false;

    // Strategy settings
    ZoomStrategy m_zoom_strategy{ ZoomStrategy::None };
    ZoomStrategy m_saved_zoom_strategy{ ZoomStrategy::None };
    SelectStrategy m_select_strategy{ SelectStrategy::None };
    SelectStrategy m_saved_select_strategy{ SelectStrategy::None };

    // The chart being displayed
    QChart* m_chart;

signals:
    void lockZoom();
    void unlockZoom();
    void zoomChanged();
    void scaleUp();
    void scaleDown();
    void addRect(const QPointF& point1, const QPointF& point2);
    void zoomRect(const QPointF& point1, const QPointF& point2);
    void pointDoubleClicked(const QPointF& point);
    void escapeSelectMode();
    void rightKey();
    void leftKey();
};
