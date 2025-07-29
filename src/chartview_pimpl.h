/*
 * CuteCharts - ChartView with PIMPL pattern (Claude Generated)
 * Copyright (C) 2016-2023 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QPointF>
#include <QtCore/QPointer>
#include <QtWidgets/QScrollArea>

#include <memory>

// Forward declarations to reduce header dependencies
class QAbstractSeries;
class QLineSeries;
class QValueAxis;
class QResizeEvent;

enum class ZoomStrategy;
enum class SelectStrategy;
enum class AutoScaleStrategy;

/**
 * @brief ChartView with PIMPL pattern for reduced compilation dependencies
 *
 * This version uses the PIMPL (Pointer to Implementation) pattern to hide
 * implementation details and reduce header dependencies. The actual
 * implementation is moved to ChartViewImpl in the .cpp file.
 *
 * Benefits:
 * - Faster compilation (fewer headers in public interface)
 * - Better encapsulation (implementation details hidden)
 * - Stable ABI (implementation changes don't break client code)
 */
class ChartView : public QScrollArea {
    Q_OBJECT

public:
    explicit ChartView();
    ~ChartView() override;

    // === Strategy Management ===

    /**
     * @brief Set the zooming strategy for the chart
     * @param strategy The zoom strategy to use
     */
    void setZoomStrategy(ZoomStrategy strategy);

    /**
     * @brief Set the selection strategy for the chart
     * @param strategy The selection strategy to use
     */
    void setSelectStrategy(SelectStrategy strategy);

    /**
     * @brief Get the current zoom strategy
     * @return The current zoom strategy
     */
    ZoomStrategy currentZoomStrategy() const;

    /**
     * @brief Get the current selection strategy
     * @return The current selection strategy
     */
    SelectStrategy currentSelectStrategy() const;

    // === Series Management ===

    /**
     * @brief Add a series to the chart
     * @param series The series to add
     * @param callout Whether to add a callout annotation for this series
     */
    void addSeries(QAbstractSeries* series, bool callout = false);

    /**
     * @brief Remove a series from the chart
     * @param series The series to remove
     */
    void removeSeries(QAbstractSeries* series);

    /**
     * @brief Get all series in the chart
     * @return List of all series
     */
    QList<QAbstractSeries*> series() const;

    /**
     * @brief Add a linear series defined by slope and intercept
     * @param m Slope
     * @param n Y-intercept
     * @param min Minimum X value
     * @param max Maximum X value
     * @return Pointer to the created LineSeries
     */
    QLineSeries* addLinearSeries(qreal m, qreal n, qreal min, qreal max);

    /**
     * @brief Clear all series from the chart
     */
    void clearChart();

    // === Chart Properties ===

    /**
     * @brief Enable or disable animations for the chart
     * @param animation Whether animations should be enabled
     */
    void setAnimationEnabled(bool animation);

    /**
     * @brief Set the chart name
     * @param name The name to set
     */
    void setName(const QString& name);

    /**
     * @brief Set the chart title
     * @param str Title text
     */
    void setTitle(const QString& str);

    /**
     * @brief Get the maximum Y value currently displayed
     * @return Maximum Y value
     */
    qreal YMax() const;

    // === Axis Access ===

    /**
     * @brief Get the X axis
     * @return Pointer to the X axis
     */
    QPointer<QValueAxis> axisX() const;

    /**
     * @brief Get the Y axis
     * @return Pointer to the Y axis
     */
    QPointer<QValueAxis> axisY() const;

    qreal XMinRange() const;
    qreal XMaxRange() const;
    qreal YMinRange() const;
    qreal YMaxRange() const;

    void setXRange(qreal min, qreal max, bool nice = false);
    void setYRange(qreal min, qreal max, bool nice = false);
    void setXMin(qreal min, bool nice = false);
    void setXMax(qreal max, bool nice = false);
    void setYMin(qreal min, bool nice = false);
    void setYMax(qreal max, bool nice = false);

    // === Configuration ===

    /**
     * @brief Get current chart configuration
     * @return Current configuration as JSON object
     */
    QJsonObject currentChartConfig() const;

    /**
     * @brief Get current font configuration
     * @return Current font settings as JSON object
     */
    QJsonObject currentFontConfig() const;

    /**
     * @brief Update chart configuration
     * @param config New configuration settings
     * @param force Whether to force the update
     */
    void updateChartConfig(const QJsonObject& config, bool force = false);

    /**
     * @brief Set the chart configuration
     * @param config Configuration to apply
     */
    void setChartConfig(const QJsonObject& config);

    /**
     * @brief Force chart configuration update
     * @param config Configuration to apply
     */
    void forceChartConfig(const QJsonObject& config);

    /**
     * @brief Apply font configuration
     * @param config Font configuration
     */
    void setFontConfig(const QJsonObject& config);

    // === Export ===

    /**
     * @brief Add an export setting preset
     * @param name Name of the preset
     * @param description Description of the preset
     * @param settings JsonObject containing the settings
     */
    void addExportSetting(const QString& name, const QString& description, const QJsonObject& settings);

    // === UI Settings ===

    /**
     * @brief Set whether the configuration dialog is modal
     * @param modal Whether dialog should be modal
     */
    void setModal(bool modal);

    /**
     * @brief Set the strategy for automatic scaling of axes
     * @param strategy The auto-scaling strategy to use
     */
    void setAutoScaleStrategy(AutoScaleStrategy strategy);

    /**
     * @brief Enable or disable the vertical tracking line
     * @param enabled Whether the line should be visible
     */
    void setVerticalLineEnabled(bool enabled);

    /**
     * @brief Set the font for the chart
     * @param font Font name to use
     */
    void setFont(const QString& font);

    // === Interaction ===

    /**
     * @brief Get the current mouse position
     * @return Current mouse position
     */
    QPointF currentMousePosition() const;

    /**
     * @brief Add a vertical line at specified position
     * @param positionX X position for the line
     */
    void addVerticalLine(double positionX);

    /**
     * @brief Remove vertical line at specified position
     * @param positionX X position of the line to remove
     * @return True if line was removed successfully
     */
    bool removeVerticalLine(double positionX);

    /**
     * @brief Remove all vertical lines
     */
    void removeAllVerticalLines();

public slots:
    // === Axis Operations ===

    /**
     * @brief Format the axis according to the current data
     */
    void formatAxis();

    /**
     * @brief Apply QtNiceNumbers scaling to axes
     */
    void qtNiceNumbersScale();

    /**
     * @brief Apply space scaling to axes
     */
    void spaceScale();

    /**
     * @brief Set the X axis label
     * @param str Label text
     */
    void setXAxis(const QString& str);

    /**
     * @brief Set the Y axis label
     * @param str Label text
     */
    void setYAxis(const QString& str);

    // === Selection and Zoom ===

    /**
     * @brief Set selection box with given coordinates
     * @param topleft Top left corner in chart coordinates
     * @param bottomright Bottom right corner in chart coordinates
     */
    void setSelectBox(const QPointF& topleft, const QPointF& bottomright);

    /**
     * @brief Handle zoom rectangle selection
     * @param point1 First corner of the rectangle
     * @param point2 Opposite corner of the rectangle
     */
    void zoomRect(const QPointF& point1, const QPointF& point2);

signals:
    void setUpFinished();
    void axisChanged();
    void chartCleared();
    void configurationChanged();
    void lastDirChanged(const QString& dir);
    void pointDoubleClicked(const QPointF& point);
    void zoomChanged();
    void scaleUp();
    void scaleDown();
    void addRect(const QPointF& point1, const QPointF& point2);
    void escapeSelectMode();
    void rightKey();
    void leftKey();
    void exportSettingsFileAdded(const QString& name, const QString& description, const QJsonObject& data);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    // PIMPL pattern - all implementation details hidden
    class ChartViewImpl;
    std::unique_ptr<ChartViewImpl> d;
};