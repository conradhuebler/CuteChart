/*
 * CuteCharts - An enhanced chart visualization framework based on Qt Charts
 * Copyright (C) 2018-2023 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "chartview.h"

#include <QtCharts/QAbstractSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>

#include <QtCore/QModelIndex>

#include <QtGui/QTextDocument>

#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QStyleOptionViewItem>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QWidget>

class QListWidget;
class QListWidgetItem;

/**
 * @brief HTML-enabled list item delegate for the series list
 *
 * This delegate enables HTML formatting in list items
 */
class HTMLListItem : public QStyledItemDelegate {
public:
    /**
     * @brief Constructor
     * @param parent Parent object
     */
    HTMLListItem(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    /**
     * @brief Paint the delegate with HTML support
     * @param painter Painter to use
     * @param option Style options
     * @param index Model index
     */
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);

        painter->save();

        QTextDocument doc;
        doc.setHtml(options.text);

        options.text = "";
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);

        painter->translate(options.rect.left(), options.rect.top());
        QRect clip(0, 0, options.rect.width(), options.rect.height());
        doc.drawContents(painter, clip);

        painter->restore();
    }

    /**
     * @brief Get size hint for the item
     * @param option Style options
     * @param index Model index
     * @return Suggested size
     */
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        return QSize(150, QStyledItemDelegate::sizeHint(option, index).height() * 1.5);
    }
};

/**
 * @brief The ListChart class provides a chart with a list of series for easy toggling
 *
 * This widget combines a ChartView with a list of series that allows users to:
 * - Toggle series visibility
 * - Rename series
 * - Change series colors
 */
class ListChart : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     */
    ListChart();

    /**
     * @brief Destructor
     */
    ~ListChart();

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

    /**
     * @brief Set the chart title
     * @param str Title text
     */
    inline void setTitle(const QString& str)
    {
        m_name = str;
        m_chartview->setTitle(str);
    }

    /**
     * @brief Get the maximum Y value
     * @return Maximum Y value
     */
    inline qreal YMax() const { return m_chartview->YMax(); }

    /**
     * @brief Add a series to the chart with custom properties
     * @param series The series to add
     * @param index Index for the series
     * @param color Color for the series
     * @param name Optional name (defaults to series name)
     * @param callout Whether to add a callout annotation
     */
    void addSeries(QAbstractSeries* series, int index, const QColor& color, QString name = QString(), bool callout = false);

    /**
     * @brief Clear all series from the chart
     */
    void clear();

    /**
     * @brief Add a linear series defined by slope and intercept
     * @param m Slope
     * @param n Y-intercept
     * @param min Minimum X value
     * @param max Maximum X value
     * @param index Index for the series
     * @return Pointer to the created LineSeries
     */
    QLineSeries* addLinearSeries(qreal m, qreal n, qreal min, qreal max, int index);

    /**
     * @brief Set animation options for the chart
     * @param option Animation options
     */
    inline void setAnimationOptions(QChart::AnimationOption option)
    {
        m_chartview->chart()->setAnimationOptions(option);
    }

    /**
     * @brief Set the chart theme
     * @param theme Chart theme to apply
     */
    inline void setTheme(QChart::ChartTheme theme)
    {
        m_chartview->chart()->setTheme(theme);
    }

    /**
     * @brief Set the chart name
     * @param name Name to set
     */
    inline void setName(const QString& name)
    {
        m_chartview->setName(name);
    }

    /**
     * @brief Get the underlying ChartView
     * @return ChartView pointer
     */
    inline ChartView* chart()
    {
        return m_chartview;
    }

    /**
     * @brief Set the automatic scale strategy
     * @param strategy Strategy to use
     */
    inline void setAutoScaleStrategy(AutoScaleStrategy strategy) { m_chartview->setAutoScaleStrategy(strategy); }

    /**
     * @brief Get current chart configuration
     * @return JsonObject with configuration
     */
    inline QJsonObject currentChartConfig() const { return m_chartview->currentChartConfig(); }

    /**
     * @brief Get current font configuration
     * @return JsonObject with font settings
     */
    inline QJsonObject currentFontConfig() const { return m_chartview->currentFontConfig(); }

    /**
     * @brief Update chart configuration
     * @param config New configuration
     * @param force Whether to force the update
     */
    inline void updateChartConfig(const QJsonObject& config, bool force = false)
    {
        m_chartview->updateChartConfig(config, force);
    }

    /**
     * @brief Apply font configuration
     * @param chartconfig Font configuration
     */
    inline void setFontConfig(const QJsonObject& chartconfig)
    {
        m_chartview->setFontConfig(chartconfig);
    }

    /**
     * @brief Add an export setting preset
     * @param name Name of the preset
     * @param description Description of the preset
     * @param settings JsonObject containing the settings
     */
    inline void addExportSetting(const QString& name, const QString& description, const QJsonObject& settings)
    {
        m_chartview->addExportSetting(name, description, settings);
    }

public slots:
    /**
     * @brief Format the chart axis
     */
    inline void formatAxis() { m_chartview->formatAxis(); }

    /**
     * @brief Set color for a series by index
     * @param index Series index
     * @param color New color
     */
    void setColor(int index, const QColor& color);

    /**
     * @brief Toggle visibility of a series
     * @param index Series index
     */
    void hideSeries(int index);

private:
    QListWidget *m_list, *m_names_list;
    ChartView* m_chartview;
    QMultiHash<int, QAbstractSeries*> m_series;
    QHash<int, bool> m_hidden;
    QString m_name;

private slots:
    /**
     * @brief Handle click on series list item
     * @param item Clicked item
     */
    inline void seriesListClicked(QListWidgetItem* item) { hideSeries(item->data(Qt::UserRole).toInt()); }

    /**
     * @brief Handle click on names list item
     * @param item Clicked item
     */
    void namesListClicked(QListWidgetItem* item);

    /**
     * @brief Show context menu
     * @param pos Menu position
     */
    void contextMenu(const QPoint& pos);

    /**
     * @brief Rename a series
     */
    void renameSeries();

    /**
     * @brief Change series color
     */
    void changeColor();

signals:
    /**
     * @brief Signal emitted when an item is double-clicked
     * @param item The clicked item
     */
    void itemDoubleClicked(QListWidgetItem* item);

    /**
     * @brief Signal emitted when last directory changes
     * @param dir New directory
     */
    void lastDirChanged(const QString& dir);

    /**
     * @brief Signal emitted when configuration changes
     */
    void configurationChanged();

    /**
     * @brief Signal emitted when setup is finished
     */
    void setUpFinished();

    /**
     * @brief Signal emitted when export settings file is added
     * @param name Name of the settings
     * @param description Description of the settings
     * @param data Settings data
     */
    void exportSettingsFileAdded(const QString& name, const QString& description, const QJsonObject& data);
};
