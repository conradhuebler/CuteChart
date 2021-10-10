/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "series.h"

LineSeries::LineSeries()
    : m_dashdot(false)
    , m_size(20)
{
}

void LineSeries::ShowLine(bool state)
{
    setVisible(state);
}

void LineSeries::ShowLine(int state)
{
    if (state == Qt::Unchecked)
        setVisible(false);
    else if (state == Qt::Checked)
        setVisible(true);
}

void LineSeries::setName(const QString& str)
{
    QLineSeries::setName(str);
}

ScatterSeries::ScatterSeries()
{
}

void ScatterSeries::setColor(const QColor& color)
{
    QPen pen = QScatterSeries::pen();
    //      pen.setStyle(Qt::DashDotLine);
    pen.setWidth(2);
    //      pen.setColor(color);
    QScatterSeries::setColor(color);
    setPen(pen);
}

void ScatterSeries::ShowLine(int state)
{
    if (state == Qt::Unchecked)
        setVisible(false);
    else if (state == Qt::Checked)
        setVisible(true);
    emit visibleChanged(state);
}

BoxPlotSeries::BoxPlotSeries(const BoxWhisker& boxwhisker)
    : m_boxwhisker(boxwhisker)
{
    LoadBoxWhisker();
    m_visible = true;
}

void BoxPlotSeries::LoadBoxWhisker()
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
        LoadBoxWhisker();
    else
        clear();
    m_visible = visible;
}

void BoxPlotSeries::setColor(const QColor& color)
{
    QBrush brush;
    brush.setColor(color);
    setBrush(brush);
}

#include "series.moc"
