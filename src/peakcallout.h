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

#pragma once

#include <QtCore/QPointer>
#include <QtGui/QFont>

#include <QtWidgets/QGraphicsItem>

#include <QtCharts>

#include <QtCharts/QChartGlobal>
#include <QtCharts/QChartView>

class QGraphicsSceneMouseEvent;

/**
 * @brief The PeakCallOut class represents a callout annotation for peaks in a chart
 *
 * This class allows for creating text annotations that point to specific data points
 * in a chart. The callouts can be moved by the user and will follow their associated
 * data points if the chart is zoomed or panned.
 */
class PeakCallOut : public QGraphicsTextItem {
public:
    /**
     * @brief Constructor for the PeakCallOut
     * @param parent The chart that this callout belongs to
     */
    explicit PeakCallOut(QPointer<QChart> parent);

    /**
     * @brief Set the text and anchor point for this callout
     * @param text The text to display in the callout
     * @param point The point on the chart to anchor this callout to
     */
    void setText(const QString& text, const QPointF& point);

    /**
     * @brief Set the anchor point for this callout
     * @param point The point on the chart to anchor this callout to
     */
    void setAnchor(QPointF point);

    /**
     * @brief Get the bounding rectangle for this callout
     * @return The bounding rectangle including the callout and its anchor line
     */
    QRectF boundingRect() const override;

    /**
     * @brief Paint the callout and its anchor line
     * @param painter The painter to use
     * @param option Style options for painting
     * @param widget The widget being painted on
     */
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    /**
     * @brief Associate this callout with a specific series
     * @param serie The series this callout is associated with
     *
     * When a series is associated with a callout, the callout will be hidden
     * when the series is hidden.
     */
    void setSeries(const QPointer<QAbstractSeries> serie) { m_series = serie; }

    /**
     * @brief Update the callout's appearance
     *
     * This should be called after changing properties like color or text.
     */
    void update();

    /**
     * @brief Place the callout at its offset from the anchor (Claude Generated)
     *
     * Converts the anchor from data to pixel coordinates; call after axis ranges
     * or plot area changed. Callouts the user dragged keep their own offset.
     */
    void reposition();

    /**
     * @brief Arrange several callouts so their cards do not overlap (Claude Generated)
     *
     * Cards are stacked in levels above their anchors (sorted by anchor x), greedy and
     * horizontally clamped to the chart. Callouts moved by the user are left alone but
     * still treated as obstacles.
     */
    static void arrange(const QList<PeakCallOut*>& callouts);

    /// Card rectangle in parent (chart) coordinates
    QRectF cardRect() const;
    QPointF anchorPosition() const;
    bool userMoved() const { return m_userMoved; }

public slots:
    /**
     * @brief Set the color of the callout text
     * @param color The color to use for the text
     */
    void setColor(const QColor& color);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    QPainterPath shape() const override;

private:
    QString m_text; ///< The plain text content
    QPointF m_anchor; ///< Anchor in data coordinates (the peak)
    QPointF m_offset = QPointF(0, -28); ///< Bottom centre of the card relative to the anchor in pixels
    QPointer<QChart> m_chart; ///< The chart this callout belongs to
    QColor m_color; ///< The color of the callout text
    bool m_userMoved = false; ///< Offset was set by dragging, arrange() keeps it
    bool m_dragging = false;

    QPointer<QAbstractSeries> m_series; ///< The series this callout is associated with
};
