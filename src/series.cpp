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

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <QtCharts/QBoxPlotSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QVXYModelMapper>
#include <QtCharts/QXYSeries>

#include <QtWidgets/QListWidgetItem>

#include "series.h"

// Factory implementation
std::unique_ptr<SeriesState> SeriesStateFactory::createState(QAbstractSeries* series)
{
    if (qobject_cast<LineSeries*>(series))
        return std::make_unique<LineSeriesState>();
    else if (qobject_cast<ScatterSeries*>(series))
        return std::make_unique<ScatterSeriesState>();
    else if (qobject_cast<BoxPlotSeries*>(series))
        return std::make_unique<BoxPlotSeriesState>();
    else if (qobject_cast<QLineSeries*>(series))
        return std::make_unique<LineSeriesState>();
    else if (qobject_cast<QScatterSeries*>(series))
        return std::make_unique<ScatterSeriesState>();

    return nullptr;
}

// === LineSeries Implementation ===

LineSeries::LineSeries()
{
    // Default values already set in members initialization
}

void LineSeries::setColor(const QColor& color)
{
    m_color = color;
    updatePen();
}

void LineSeries::setDashDotLine(bool dashdot)
{
    m_dashdot = dashdot;
    updatePen();
}

void LineSeries::setLineWidth(double size)
{
    m_size = size;
    updatePen();
}

void LineSeries::updatePen()
{
    QPen pen = QLineSeries::pen();
    pen.setStyle(m_dashdot ? Qt::DashDotLine : Qt::SolidLine);
    pen.setWidth(m_size);
    pen.setColor(m_color);
    setPen(pen);
}

void LineSeries::showLine(bool state)
{
    setVisible(state);
}

void LineSeries::showLine(int state)
{
    setVisible(state == Qt::Checked);
}

void LineSeries::setName(const QString& str)
{
    QLineSeries::setName(str);
}

// Line Series State Implementation
void LineSeriesState::saveState(QAbstractSeries* series)
{
    if (auto lineSeries = qobject_cast<LineSeries*>(series)) {
        m_color = lineSeries->color();
        m_lineWidth = lineSeries->lineWidth();
        m_dashDot = lineSeries->isDashDot();
    } else if (auto baseSeries = qobject_cast<QLineSeries*>(series)) {
        m_color = baseSeries->pen().color();
        m_lineWidth = baseSeries->pen().width();
        m_dashDot = baseSeries->pen().style() == Qt::DashDotLine;
    }
}

void LineSeriesState::restoreState(QAbstractSeries* series)
{
    if (auto lineSeries = qobject_cast<LineSeries*>(series)) {
        lineSeries->setColor(m_color);
        lineSeries->setLineWidth(m_lineWidth);
        lineSeries->setDashDotLine(m_dashDot);
    } else if (auto baseSeries = qobject_cast<QLineSeries*>(series)) {
        QPen pen = baseSeries->pen();
        pen.setColor(m_color);
        pen.setWidth(m_lineWidth);
        pen.setStyle(m_dashDot ? Qt::DashDotLine : Qt::SolidLine);
        baseSeries->setPen(pen);
    }
}

// === ScatterSeries Implementation ===

ScatterSeries::ScatterSeries()
{
    // Default values already set in members initialization
}

void ScatterSeries::setColor(const QColor& color)
{
    QScatterSeries::setColor(color);
    // Retain pen properties when changing color
    QPen pen = this->pen();
    setPen(pen);
}

void ScatterSeries::showLine(int state)
{
    bool visible = (state == Qt::Checked);
    setVisible(visible);
    emit visibilityChanged(state);
}

// Scatter Series State Implementation
void ScatterSeriesState::saveState(QAbstractSeries* series)
{
    auto scatterSeries = qobject_cast<QScatterSeries*>(series);
    if (scatterSeries) {
        m_color = scatterSeries->color();
        m_borderColor = scatterSeries->borderColor();
        m_markerSize = scatterSeries->markerSize();
    }
}

void ScatterSeriesState::restoreState(QAbstractSeries* series)
{
    auto scatterSeries = qobject_cast<QScatterSeries*>(series);
    if (scatterSeries) {
        scatterSeries->setColor(m_color);
        scatterSeries->setBorderColor(m_borderColor);
        scatterSeries->setMarkerSize(m_markerSize);
    }
}

// === BoxPlotSeries Implementation ===

BoxPlotSeries::BoxPlotSeries(const BoxWhisker& boxwhisker)
    : m_boxwhisker(boxwhisker)
{
    loadBoxWhisker();
}

void BoxPlotSeries::loadBoxWhisker()
{
    QBoxSet* box = new QBoxSet;
    box->setValue(QBoxSet::LowerExtreme, m_boxwhisker.lower_whisker);
    box->setValue(QBoxSet::UpperExtreme, m_boxwhisker.upper_whisker);
    box->setValue(QBoxSet::Median, m_boxwhisker.median);
    box->setValue(QBoxSet::LowerQuartile, m_boxwhisker.lower_quantile);
    box->setValue(QBoxSet::UpperQuartile, m_boxwhisker.upper_quantile);
    append(box);
}

void BoxPlotSeries::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    if (visible)
        loadBoxWhisker();
    else
        clear();

    m_visible = visible;
    QBoxPlotSeries::setVisible(visible);
}

void BoxPlotSeries::setColor(const QColor& color)
{
    QBrush brush = this->brush();
    brush.setColor(color);
    setBrush(brush);
}

// Box Plot Series State Implementation
void BoxPlotSeriesState::saveState(QAbstractSeries* series)
{
    auto boxPlotSeries = qobject_cast<BoxPlotSeries*>(series);
    if (boxPlotSeries) {
        m_color = boxPlotSeries->color();
        m_visible = boxPlotSeries->isVisible();
    }
}

void BoxPlotSeriesState::restoreState(QAbstractSeries* series)
{
    auto boxPlotSeries = qobject_cast<BoxPlotSeries*>(series);
    if (boxPlotSeries) {
        boxPlotSeries->setColor(m_color);
        boxPlotSeries->setVisible(m_visible);
    }
}

// Helper function for ListChart
void updateSeriesColor(QListWidgetItem* item, QAbstractSeries* series, const QColor& color)
{
    if (!item || !series || !color.isValid())
        return;

    item->setBackground(color);

    if (auto xySeries = qobject_cast<QXYSeries*>(series)) {
        xySeries->setColor(color);

        if (auto scatterSeries = qobject_cast<QScatterSeries*>(series)) {
            scatterSeries->setBorderColor(color);
        }
    } else if (auto boxPlotSeries = qobject_cast<BoxPlotSeries*>(series)) {
        boxPlotSeries->setColor(color);
    }
}

#include "series.moc"
