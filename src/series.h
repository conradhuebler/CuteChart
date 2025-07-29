/*
 * CuteCharts - An enhanced chart visualization framework based on Qt Charts
 * Copyright (C) 2019-2023 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QDebug>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <QtCharts>

#include <QtCharts/QBoxPlotSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QVXYModelMapper>
#include <QtCharts/QXYSeries>

#include "boxwhisker.h"

#include <memory>

/**
 * @brief Series state interface for saving/restoring series properties
 *
 * Abstract class defining the interface for the state pattern to store
 * and restore series properties during operations like exporting.
 */
class SeriesState {
public:
    virtual ~SeriesState() = default;

    /**
     * @brief Save the current state of the series
     * @param series The series to save state from
     */
    virtual void saveState(QAbstractSeries* series) = 0;

    /**
     * @brief Restore the saved state to the series
     * @param series The series to restore state to
     */
    virtual void restoreState(QAbstractSeries* series) = 0;
};

/**
 * @brief Factory for creating appropriate SeriesState objects
 */
class SeriesStateFactory {
public:
    /**
     * @brief Create an appropriate SeriesState for a given series
     * @param series The series to create a state object for
     * @return A unique_ptr to the created state object, or nullptr if not supported
     */
    static std::unique_ptr<SeriesState> createState(QAbstractSeries* series);
};

/**
 * @brief Enhanced line series with additional styling options
 */
class LineSeries : public QLineSeries {
    Q_OBJECT

public:
    LineSeries();
    ~LineSeries() override = default;

    /**
     * @brief Check if series should appear in legend
     * @return True if series should appear in legend
     */
    bool showInLegend() const { return m_show_in_legend; }

    /**
     * @brief Set whether the series should appear in legend
     * @param legend True to show in legend
     */
    void setShowInLegend(bool legend)
    {
        m_show_in_legend = legend;
        emit legendChanged(legend);
    }

    /**
     * @brief Get the current line width
     * @return Line width
     */
    qreal lineWidth() const { return m_size; }

    /**
     * @brief Get whether line uses dash-dot style
     * @return True if dash-dot style is active
     */
    bool isDashDot() const { return m_dashdot; }

    /**
     * @brief Get the line color
     * @return Line color
     */
    QColor color() const { return m_color; }

public slots:
    void setColor(const QColor& color) override;

    /**
     * @brief Set whether to use dash-dot line style
     * @param dashdot True for dash-dot style, false for solid
     */
    void setDashDotLine(bool dashdot);

    /**
     * @brief Set line width
     * @param size Line width in pixels
     */
    void setLineWidth(double size);

    /**
     * @brief Set line visibility based on boolean
     * @param state True to show line
     */
    void showLine(bool state);

    /**
     * @brief Set line visibility based on checkbox state
     * @param state Qt::CheckState
     */
    void showLine(int state);

    /**
     * @brief Set the series name
     * @param name New name for the series
     */
    void setName(const QString& name);

private:
    /**
     * @brief Update pen appearance based on current settings
     */
    void updatePen();

    bool m_dashdot = false;
    bool m_show_in_legend = false;
    double m_size = 2;
    QColor m_color;

signals:
    /**
     * @brief Signal emitted when legend visibility changes
     * @param legend Whether series should show in legend
     */
    void legendChanged(bool legend);
};

/**
 * @brief State implementation for LineSeries
 */
class LineSeriesState : public SeriesState {
public:
    void saveState(QAbstractSeries* series) override;
    void restoreState(QAbstractSeries* series) override;

private:
    QColor m_color;
    double m_lineWidth = 2;
    bool m_dashDot = false;
};

/**
 * @brief Enhanced scatter series with additional features
 */
class ScatterSeries : public QScatterSeries {
    Q_OBJECT

public:
    ScatterSeries();
    ~ScatterSeries() override = default;

    /**
     * @brief Check if series should appear in legend
     * @return True if series should appear in legend
     */
    bool showInLegend() const { return m_show_in_legend; }

    /**
     * @brief Set whether the series should appear in legend
     * @param legend True to show in legend
     */
    void setShowInLegend(bool legend)
    {
        m_show_in_legend = legend;
        emit legendChanged(legend);
    }

public slots:
    void setColor(const QColor& color) override;

    /**
     * @brief Set series visibility based on checkbox state
     * @param state Qt::CheckState
     */
    void showLine(int state);

private:
    bool m_show_in_legend = false;

signals:
    /**
     * @brief Signal emitted when name changes
     * @param str New name
     */
    void nameChanged(const QString& str);

    /**
     * @brief Signal emitted when visibility changes
     * @param state New visibility state
     */
    void visibilityChanged(int state);

    /**
     * @brief Signal emitted when legend visibility changes
     * @param legend Whether series should show in legend
     */
    void legendChanged(bool legend);
};

/**
 * @brief State implementation for ScatterSeries
 */
class ScatterSeriesState : public SeriesState {
public:
    void saveState(QAbstractSeries* series) override;
    void restoreState(QAbstractSeries* series) override;

private:
    QColor m_color;
    QColor m_borderColor;
    double m_markerSize = 8;
};

/**
 * @brief Enhanced box plot series for statistical visualizations
 */
class BoxPlotSeries : public QBoxPlotSeries {
    Q_OBJECT

public:
    /**
     * @brief Constructor with boxwhisker data
     * @param boxwhisker The boxwhisker data to display
     */
    explicit BoxPlotSeries(const BoxWhisker& boxwhisker);

    /**
     * @brief Get the series color
     * @return Current color
     */
    QColor color() const { return brush().color(); }

public slots:
    /**
     * @brief Set the series color
     * @param color New color
     */
    void setColor(const QColor& color);

    /**
     * @brief Set whether the series should be visible
     * @param visible Whether series should be visible
     */
    void setVisible(bool visible);

private:
    /**
     * @brief Load boxwhisker data into series
     */
    void loadBoxWhisker();

    BoxWhisker m_boxwhisker;
    bool m_visible = true;
};

/**
 * @brief State implementation for BoxPlotSeries
 */
class BoxPlotSeriesState : public SeriesState {
public:
    void saveState(QAbstractSeries* series) override;
    void restoreState(QAbstractSeries* series) override;

private:
    QColor m_color;
    bool m_visible = true;
};

/**
 * @brief Update series and list item color in ListChart
 *
 * @param item List widget item to update
 * @param series Series to update
 * @param color New color to apply
 */
void updateSeriesColor(QListWidgetItem* item, QAbstractSeries* series, const QColor& color);
