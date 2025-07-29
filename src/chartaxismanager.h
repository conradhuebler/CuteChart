/*
 * CuteCharts - Axis management component
 * Copyright (C) 2023 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "chartviewprivate.h"
#include "tools.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include <QtCharts/QValueAxis>

class QChart;

/**
 * @brief Manages chart axis operations and scaling
 *
 * Extracted from ChartView to handle all axis-related operations:
 * - Axis creation and configuration
 * - Scaling strategies (nice numbers, space scaling, etc.)
 * - Range management and formatting
 * - Tick interval calculations
 */
class ChartAxisManager : public QObject {
    Q_OBJECT

public:
    explicit ChartAxisManager(QChart* chart, QObject* parent = nullptr);
    ~ChartAxisManager() override = default;

    /**
     * @brief Initialize axes for the chart
     */
    void initializeAxes();

    /**
     * @brief Get X axis
     * @return Pointer to X axis or nullptr if not initialized
     */
    QPointer<QValueAxis> axisX() const { return m_xAxis; }

    /**
     * @brief Get Y axis
     * @return Pointer to Y axis or nullptr if not initialized
     */
    QPointer<QValueAxis> axisY() const { return m_yAxis; }

    /**
     * @brief Check if axes are initialized
     * @return True if both axes exist
     */
    bool hasAxes() const { return m_xAxis && m_yAxis; }

    /**
     * @brief Set X axis range
     * @param min Minimum value
     * @param max Maximum value
     * @param nice Whether to use nice number scaling
     */
    void setXRange(qreal min, qreal max, bool nice = false);

    /**
     * @brief Set Y axis range
     * @param min Minimum value
     * @param max Maximum value
     * @param nice Whether to use nice number scaling
     */
    void setYRange(qreal min, qreal max, bool nice = false);

    /**
     * @brief Set X axis minimum
     * @param min Minimum value
     * @param nice Whether to use nice number scaling
     */
    void setXMin(qreal min, bool nice = false);

    /**
     * @brief Set X axis maximum
     * @param max Maximum value
     * @param nice Whether to use nice number scaling
     */
    void setXMax(qreal max, bool nice = false);

    /**
     * @brief Set Y axis minimum
     * @param min Minimum value
     * @param nice Whether to use nice number scaling
     */
    void setYMin(qreal min, bool nice = false);

    /**
     * @brief Set Y axis maximum
     * @param max Maximum value
     * @param nice Whether to use nice number scaling
     */
    void setYMax(qreal max, bool nice = false);

    /**
     * @brief Get current X range
     * @return QPair with (min, max) values
     */
    QPair<qreal, qreal> getXRange() const;

    /**
     * @brief Get current Y range
     * @return QPair with (min, max) values
     */
    QPair<qreal, qreal> getYRange() const;

    /**
     * @brief Set axis labels
     * @param xLabel X axis label
     * @param yLabel Y axis label
     */
    void setAxisLabels(const QString& xLabel, const QString& yLabel);

    /**
     * @brief Get current axis labels
     * @return QPair with (xLabel, yLabel)
     */
    QPair<QString, QString> getAxisLabels() const;

    /**
     * @brief Set auto-scale strategy
     * @param strategy Strategy to use for automatic scaling
     */
    void setAutoScaleStrategy(AutoScaleStrategy strategy) { m_autoScaleStrategy = strategy; }

    /**
     * @brief Get current auto-scale strategy
     * @return Current auto-scale strategy
     */
    AutoScaleStrategy getAutoScaleStrategy() const { return m_autoScaleStrategy; }

public slots:
    /**
     * @brief Format axes according to current data and strategy
     */
    void formatAxis();

    /**
     * @brief Force axis formatting without checking conditions
     */
    void forceFormatAxis();

    /**
     * @brief Apply Qt nice numbers scaling to axes
     */
    void applyQtNiceNumbersScale();

    /**
     * @brief Apply space scaling to axes
     */
    void applySpaceScale();

    /**
     * @brief Zoom to rectangle defined by two points
     * @param point1 First corner of zoom rectangle
     * @param point2 Opposite corner of zoom rectangle
     */
    void zoomToRect(const QPointF& point1, const QPointF& point2);

    /**
     * @brief Auto-scale axes based on current strategy
     */
    void autoScale();

signals:
    /**
     * @brief Emitted when axis ranges change
     */
    void axisRangeChanged();

    /**
     * @brief Emitted when axis labels change
     */
    void axisLabelsChanged();

private:
    QChart* m_chart;
    QPointer<QValueAxis> m_xAxis;
    QPointer<QValueAxis> m_yAxis;

    QString m_xLabel;
    QString m_yLabel;

    AutoScaleStrategy m_autoScaleStrategy = AutoScaleStrategy::QtNiceNumbers;

    qreal m_xMin = 0.0;
    qreal m_xMax = 10.0;
    qreal m_yMin = 0.0;
    qreal m_yMax = 10.0;

    /**
     * @brief Apply scaling to a single axis
     * @param axis Axis to scale
     * @param min Reference to minimum value (may be modified)
     * @param max Reference to maximum value (may be modified)
     */
    void scaleAxis(QPointer<QValueAxis> axis, qreal& min, qreal& max);

    /**
     * @brief Calculate optimal tick interval for axis
     * @param min Minimum value
     * @param max Maximum value
     * @return Optimal tick interval
     */
    qreal calculateTickInterval(qreal min, qreal max) const;

    /**
     * @brief Update axis appearance and formatting
     */
    void updateAxisAppearance();
};