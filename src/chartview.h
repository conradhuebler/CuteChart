/*
 * CuteCharts - An enhanced chart visualization framework based on Qt Charts
 * Copyright (C) 2016-2023 Conrad Hübler <Conrad.Huebler@gmx.net>
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

// Reduced includes for faster compilation (Claude Generated)
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtWidgets/QScrollArea>

#include <memory>

// Forward declarations to reduce header dependencies (Claude Generated)
class QChart;
class QAbstractSeries;
class QAbstractAxis;
class QLineSeries;
class QValueAxis;
class QGridLayout;
class QPushButton;
class QStackedWidget;
class QResizeEvent;

class ChartViewPrivate;
class ChartConfigDialog;
class PeakCallOut;

// Forward declare enums (defined in chartviewprivate.h)
enum class ZoomStrategy;
enum class SelectStrategy;
enum class AutoScaleStrategy;

struct ChartConfig;

// Default chart configuration settings
const QJsonObject DefaultConfig{
    { "Title", "" },
    { "Legend", false },
    { "ScalingLocked", false },
    { "Annotation", false },
    { "xSize", 600 },
    { "ySize", 400 },
    { "Scaling", 2 },
    { "lineWidth", 4 },
    { "markerSize", 8 },
    { "Theme", 0 },
    { "cropImage", true },
    { "transparentImage", true },
    { "emphasizeAxis", true },
    { "noGrid", true }
};

/**
 * @brief The ChartView class provides a scrollable area containing a chart with enhanced functionality
 *
 * This class extends QScrollArea to include a QtCharts-based chart with additional features such as:
 * - Zooming and selection strategies
 * - Configuration dialog for chart settings
 * - Export capabilities
 * - Custom scaling strategies
 * - Series management
 */
class ChartView : public QScrollArea {
    Q_OBJECT

public:
    /**
     * @brief Constructor for ChartView
     */
    ChartView();

    /**
     * @brief Destructor for ChartView
     */
    ~ChartView() override;

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

    /**
     * @brief Add a series to the chart
     * @param series The series to add
     * @param callout Whether to add a callout annotation for this series
     */
    void addSeries(QAbstractSeries* series, bool callout = false);

    /**
     * @brief Enable or disable animations for the chart
     * @param animation Whether animations should be enabled
     */
    void setAnimationEnabled(bool animation);

    /**
     * @brief Get the maximum Y value currently displayed
     * @return Maximum Y value
     */
    qreal YMax() const { return m_ymax; }

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
     * @brief Get the maximum Y range value
     * @return Maximum Y value of the axis range
     */
    qreal YMaxRange() const;

    /**
     * @brief Get the minimum Y range value
     * @return Minimum Y value of the axis range
     */
    qreal YMinRange() const;

    /**
     * @brief Get the maximum X range value
     * @return Maximum X value of the axis range
     */
    qreal XMaxRange() const;

    /**
     * @brief Get the minimum X range value
     * @return Minimum X value of the axis range
     */
    qreal XMinRange() const;

    /**
     * @brief Set the X axis range
     * @param xmin Minimum X value
     * @param xmax Maximum X value
     * @param nice Whether to use nice numbers for scaling
     */
    void setXRange(qreal xmin, qreal xmax, bool nice = false);

    /**
     * @brief Set the maximum X value
     * @param xmax Maximum X value
     * @param nice Whether to use nice numbers for scaling
     */
    void setXMax(qreal xmax, bool nice = false);

    /**
     * @brief Set the minimum X value
     * @param xmin Minimum X value
     * @param nice Whether to use nice numbers for scaling
     */
    void setXMin(qreal xmin, bool nice = false);

    /**
     * @brief Set the Y axis range
     * @param ymin Minimum Y value
     * @param ymax Maximum Y value
     * @param nice Whether to use nice numbers for scaling
     */
    void setYRange(qreal ymin, qreal ymax, bool nice = false);

    /**
     * @brief Set the maximum Y value
     * @param ymax Maximum Y value
     * @param nice Whether to use nice numbers for scaling
     */
    void setYMax(qreal ymax, bool nice);

    /**
     * @brief Set the minimum Y value
     * @param ymin Minimum Y value
     * @param nice Whether to use nice numbers for scaling
     */
    void setYMin(qreal ymin, bool nice);

    /**
     * @brief Set the chart name
     * @param name The name to set
     */
    void setName(const QString& name);

    /**
     * @brief Get the underlying chart object
     * @return Pointer to the QChart
     */
    QChart* chart() { return m_chart; }

    /**
     * @brief Get the private chart view
     * @return Pointer to the ChartViewPrivate
     */
    ChartViewPrivate* privateView() { return m_chart_private; }

    /**
     * @brief Get the Y axis
     * @return Pointer to the Y axis
     */
    QPointer<QValueAxis> axisY() const { return m_YAxis; }

    /**
     * @brief Get the X axis
     * @return Pointer to the X axis
     */
    QPointer<QValueAxis> axisX() const { return m_XAxis; }

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

    /**
     * @brief Get the current mouse position
     * @return Current mouse position
     */
    QPointF currentMousePosition() const;

    /**
     * @brief Get the current chart configuration
     * @return JsonObject containing the configuration
     */
    QJsonObject currentChartConfig() const { return m_currentChartConfig; }

    /**
     * @brief Get the chart configuration
     * @return JsonObject containing the configuration
     */
    QJsonObject getChartConfig() const;

    /**
     * @brief Set the chart configuration
     * @param config Configuration to apply
     */
    void setChartConfig(const QJsonObject& config);

    /**
     * @brief Get the current font configuration
     * @return JsonObject containing font settings
     */
    QJsonObject currentFontConfig() const;

    /**
     * @brief Update the chart configuration
     * @param config New configuration settings
     * @param force Whether to force the update
     */
    void updateChartConfig(const QJsonObject& config, bool force = false);

    /**
     * @brief Force update of chart configuration
     * @param config Configuration to apply
     */
    void forceChartConfig(const QJsonObject& config);

    /**
     * @brief Add an export setting preset
     * @param name Name of the preset
     * @param description Description of the preset
     * @param settings JsonObject containing the settings
     */
    void addExportSetting(const QString& name, const QString& description, const QJsonObject& settings);

    /**
     * @brief Add a vertical line at specified position
     * @param position_x X position for the line
     */
    void addVerticalLine(double position_x);

    /**
     * @brief Remove vertical line at specified position
     * @param position_x X position of the line to remove
     * @return True if line was removed successfully
     */
    bool removeVerticalLine(double position_x);

    /**
     * @brief Remove all vertical lines
     */
    void removeAllVerticalLines();

public slots:
    /**
     * @brief Set selection box with given coordinates
     * @param topleft Top left corner in chart coordinates
     * @param bottomright Bottom right corner in chart coordinates
     */
    void setSelectBox(const QPointF& topleft, const QPointF& bottomright);

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
    void setXAxis(const QString& str)
    {
        m_x_axis = str;
        emit axisChanged();
    }

    /**
     * @brief Set the Y axis label
     * @param str Label text
     */
    void setYAxis(const QString& str)
    {
        m_y_axis = str;
        emit axisChanged();
    }

    /**
     * @brief Set the chart title
     * @param str Title text
     */
    void setTitle(const QString& str);

    /**
     * @brief Handle zoom rectangle selection
     * @param point1 First corner of the rectangle
     * @param point2 Opposite corner of the rectangle
     */
    void zoomRect(const QPointF& point1, const QPointF& point2);

    /**
     * @brief Apply font configuration
     * @param chartconfig The configuration to apply
     */
    void setFontConfig(const QJsonObject& chartconfig);

private:
    void connectLegendCallbacks(QAbstractSeries* series, bool initialShowState);

    QStackedWidget* m_centralWidget;
    QWidget* m_configure;

    QAction* m_lock_action;
    ChartViewPrivate* m_chart_private;
    QPointer<QChart> m_chart;
    QPushButton *m_config, *m_action_button, *m_ignore;
    void setUi();
    bool has_legend, connected, m_hasAxis = false, m_manual_zoom = false;
    QString m_x_axis, m_y_axis;
    QString color2RGB(const QColor& color) const;
    void writeTable(const QString& str);
    ChartConfigDialog* m_chartconfigdialog;
    bool m_pending, m_lock_scaling, m_modal = true, m_prevent_notification = false;
    qreal m_ymax, m_ymin, m_xmin, m_xmax;
    QVector<QPointer<QAbstractSeries>> m_series;
    QVector<QPointer<PeakCallOut>> m_peak_anno;

    QAction *m_configure_series, *m_select_none, *m_select_horizonal, *m_select_vertical, *m_select_rectangular, *m_zoom_none, *m_zoom_horizonal, *m_zoom_vertical, *m_zoom_rectangular;
    QMenu *m_select_strategy, *m_zoom_strategy;

    QJsonObject m_currentChartConfig, m_pendingChartConfig, m_lastChartConfig;

    QString m_name, m_last_filename;
    QPointer<QValueAxis> m_XAxis, m_YAxis;

    void scaleAxis(QPointer<QValueAxis> axis, qreal& min, qreal& max);
    QGridLayout* mCentralLayout;

    // -1: button activated to revert
    // +1: button activated to apply
    int m_apply_action = 0;
    int m_x_size = 600, m_y_size = 400, m_scaling = 2;
    qreal m_lineWidth = 2, m_markerSize = 8;

    QString m_font;
    AutoScaleStrategy m_autoscalestrategy;

    void updateAxisConfig(const QJsonObject& config, QAbstractAxis* axis);
    QJsonObject getAxisConfig(const QAbstractAxis* axis) const;
    void applyConfigAction();

    QHash<QString, QPair<QString, QJsonObject>> m_stored_exportsettings;
    QMenu* m_exportMenu;

private slots:
    void plotSettings();
    void saveFontConfig();
    void loadFontConfig();
    void exportPNG();

    void forceFormatAxis();
    void configure();

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
};
