/*
 * CuteCharts - Axis management component (Claude Generated)
 * Copyright (C) 2023 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "chartaxismanager.h"
#include "chartviewprivate.h"

#include <QtCharts/QChart>
#include <QtCharts/QXYSeries>

#include <QtCore/QDebug>

#include <cmath>

#ifdef DEBUG_ON
#include <QtCore/QDebug>
#endif

ChartAxisManager::ChartAxisManager(QChart* chart, QObject* parent)
    : QObject(parent)
    , m_chart(chart)
    , m_xLabel("X")
    , m_yLabel("Y")
{
#ifdef DEBUG_ON
    qDebug() << "ChartAxisManager: Initialized";
#endif
}

void ChartAxisManager::initializeAxes()
{
    if (!m_chart) {
#ifdef DEBUG_ON
        qDebug() << "ChartAxisManager: Cannot initialize axes - chart is null";
#endif
        return;
    }

    // Create axes if they don't exist
    if (!m_xAxis) {
        m_xAxis = new QValueAxis(this);
        m_xAxis->setTitleText(m_xLabel);
        m_chart->addAxis(m_xAxis, Qt::AlignBottom);
    }

    if (!m_yAxis) {
        m_yAxis = new QValueAxis(this);
        m_yAxis->setTitleText(m_yLabel);
        m_chart->addAxis(m_yAxis, Qt::AlignLeft);
    }

    // Attach axes to all existing series
    const auto series = m_chart->series();
    for (auto* s : series) {
        if (auto* xySeries = qobject_cast<QXYSeries*>(s)) {
            xySeries->attachAxis(m_xAxis);
            xySeries->attachAxis(m_yAxis);
        }
    }

#ifdef DEBUG_ON
    qDebug() << "ChartAxisManager: Axes initialized";
#endif
}

void ChartAxisManager::setXRange(qreal min, qreal max, bool nice)
{
    if (!m_xAxis) {
        initializeAxes();
    }

    if (m_xAxis) {
        if (nice) {
            m_xAxis->setMin(ChartTools::NiceScalingMin(min));
            m_xAxis->setMax(ChartTools::NiceScalingMax(max));
        } else {
            m_xAxis->setMin(min);
            m_xAxis->setMax(max);
        }
        m_xAxis->setTickInterval(ChartTools::CustomCeil(max + min) / 10.0);

        m_xMin = min;
        m_xMax = max;

        emit axisRangeChanged();

#ifdef DEBUG_ON
        qDebug() << "ChartAxisManager: X range set to" << min << "-" << max << "nice:" << nice;
#endif
    }
}

void ChartAxisManager::setYRange(qreal min, qreal max, bool nice)
{
    if (!m_yAxis) {
        initializeAxes();
    }

    if (m_yAxis) {
        if (nice) {
            m_yAxis->setMin(ChartTools::NiceScalingMin(min));
            m_yAxis->setMax(ChartTools::NiceScalingMax(max));
        } else {
            m_yAxis->setMin(min);
            m_yAxis->setMax(max);
        }
        m_yAxis->setTickInterval(ChartTools::CustomCeil(max + min) / 10.0);

        m_yMin = min;
        m_yMax = max;

        emit axisRangeChanged();

#ifdef DEBUG_ON
        qDebug() << "ChartAxisManager: Y range set to" << min << "-" << max << "nice:" << nice;
#endif
    }
}

void ChartAxisManager::setXMin(qreal min, bool nice)
{
    if (m_xAxis) {
        qreal max = m_xAxis->max();
        setXRange(min, max, nice);
    }
}

void ChartAxisManager::setXMax(qreal max, bool nice)
{
    if (m_xAxis) {
        qreal min = m_xAxis->min();
        setXRange(min, max, nice);
    }
}

void ChartAxisManager::setYMin(qreal min, bool nice)
{
    if (m_yAxis) {
        qreal max = m_yAxis->max();
        setYRange(min, max, nice);
    }
}

void ChartAxisManager::setYMax(qreal max, bool nice)
{
    if (m_yAxis) {
        qreal min = m_yAxis->min();
        setYRange(min, max, nice);
    }
}

QPair<qreal, qreal> ChartAxisManager::getXRange() const
{
    if (m_xAxis) {
        return qMakePair(m_xAxis->min(), m_xAxis->max());
    }
    return qMakePair(m_xMin, m_xMax);
}

QPair<qreal, qreal> ChartAxisManager::getYRange() const
{
    if (m_yAxis) {
        return qMakePair(m_yAxis->min(), m_yAxis->max());
    }
    return qMakePair(m_yMin, m_yMax);
}

void ChartAxisManager::setAxisLabels(const QString& xLabel, const QString& yLabel)
{
    m_xLabel = xLabel;
    m_yLabel = yLabel;

    if (m_xAxis) {
        m_xAxis->setTitleText(xLabel);
    }
    if (m_yAxis) {
        m_yAxis->setTitleText(yLabel);
    }

    emit axisLabelsChanged();

#ifdef DEBUG_ON
    qDebug() << "ChartAxisManager: Axis labels set to" << xLabel << "," << yLabel;
#endif
}

QPair<QString, QString> ChartAxisManager::getAxisLabels() const
{
    return qMakePair(m_xLabel, m_yLabel);
}

void ChartAxisManager::formatAxis()
{
    if (!m_chart || m_chart->series().isEmpty()) {
        return;
    }
    forceFormatAxis();
}

void ChartAxisManager::forceFormatAxis()
{
    if (!m_chart || m_chart->series().size() == 0) {
        return;
    }

    if (m_autoScaleStrategy == AutoScaleStrategy::QtNiceNumbers) {
        applyQtNiceNumbersScale();
    } else if (m_autoScaleStrategy == AutoScaleStrategy::SpaceScale) {
        applySpaceScale();
    }

#ifdef DEBUG_ON
    qDebug() << "ChartAxisManager: Force format axis completed";
#endif
}

void ChartAxisManager::applyQtNiceNumbersScale()
{
    if (!hasAxes()) {
        initializeAxes();
    }

    qreal x_min = 1e12;
    qreal x_max = -1 * 1e12;
    qreal y_max = -1 * 1e12;
    qreal y_min = 1 * 1e12;
    bool start = false;

    // Find data bounds from all visible series
    for (const auto* series : m_chart->series()) {
        const auto* xySeries = qobject_cast<const QXYSeries*>(series);
        if (!xySeries || !xySeries->isVisible()) {
            continue;
        }

        const QVector<QPointF> points = xySeries->points();
        if (!start && !points.isEmpty()) {
            const QPointF& firstPoint = points.first();
            y_min = y_max = firstPoint.y();
            x_min = x_max = firstPoint.x();
            start = true;
        }

        for (const QPointF& point : points) {
            y_min = qMin(y_min, point.y());
            y_max = qMax(y_max, point.y());
            x_min = qMin(x_min, point.x());
            x_max = qMax(x_max, point.x());
        }
    }

    if (start) {
        // Apply nice number scaling
        x_min = ChartTools::NiceScalingMin(x_min);
        x_max = ChartTools::NiceScalingMax(x_max);
        y_min = ChartTools::NiceScalingMin(y_min);
        y_max = ChartTools::NiceScalingMax(y_max);

        setXRange(x_min, x_max);
        setYRange(y_min, y_max);

#ifdef DEBUG_ON
        qDebug() << "ChartAxisManager: Qt nice numbers scaling applied";
#endif
    }
}

void ChartAxisManager::applySpaceScale()
{
    if (!hasAxes()) {
        initializeAxes();
    }

    qreal x_min = 0;
    qreal x_max = 0;
    qreal y_max = 0;
    qreal y_min = 0;
    bool start = false;

    // Find data bounds from all visible series
    for (const auto* series : m_chart->series()) {
        const auto* xySeries = qobject_cast<const QXYSeries*>(series);
        if (!xySeries || !xySeries->isVisible()) {
            continue;
        }

        const QVector<QPointF> points = xySeries->points();
        if (!start && !points.isEmpty()) {
            const QPointF& firstPoint = points.first();
            y_min = y_max = firstPoint.y();
            x_min = x_max = firstPoint.x();
            start = true;
        }

        for (const QPointF& point : points) {
            y_min = qMin(y_min, point.y());
            y_max = qMax(y_max, point.y());
            x_min = qMin(x_min, point.x());
            x_max = qMax(x_max, point.x());
        }
    }

    if (start) {
        scaleAxis(m_xAxis, x_min, x_max);
        scaleAxis(m_yAxis, y_min, y_max);

        m_xMin = x_min;
        m_xMax = x_max;
        m_yMin = y_min;
        m_yMax = y_max;

        emit axisRangeChanged();

#ifdef DEBUG_ON
        qDebug() << "ChartAxisManager: Space scaling applied";
#endif
    }
}

void ChartAxisManager::zoomToRect(const QPointF& point1, const QPointF& point2)
{
    qreal x_max = qMax(point1.x(), point2.x());
    qreal x_min = qMin(point1.x(), point2.x());
    qreal y_min = qMin(point1.y(), point2.y());
    qreal y_max = qMax(point1.y(), point2.y());

    setXRange(x_min, x_max);
    setYRange(y_min, y_max);

#ifdef DEBUG_ON
    qDebug() << "ChartAxisManager: Zoomed to rect" << x_min << x_max << y_min << y_max;
#endif
}

void ChartAxisManager::autoScale()
{
    forceFormatAxis();
}

void ChartAxisManager::scaleAxis(QPointer<QValueAxis> axis, qreal& min, qreal& max)
{
    if (!axis) {
        return;
    }

    // Improved scaling algorithm based on original ChartView implementation
    int mean = (max + min) / 2;

    if (1 < mean && mean < 10) {
        max = std::ceil(max);
        min = std::floor(min);
    } else {
        max = ChartTools::CustomCeil(max - mean) + mean;
        if (min && !(0 < min && min < 1)) {
            min = ChartTools::CustomFloor(min - mean) + mean;
        } else {
            min = 0;
        }
    }

    int ticks = ChartTools::ScaleToNormalizedRange(max - min) / int(ChartTools::ScaleToNormalizedRange(max - min) / 5) + 1;
    axis->setTickCount(ticks);
    axis->setRange(min, max);

#ifdef DEBUG_ON
    qDebug() << "ChartAxisManager: Axis scaled, range:" << min << "-" << max << "ticks:" << ticks;
#endif
}

qreal ChartAxisManager::calculateTickInterval(qreal min, qreal max) const
{
    qreal range = max - min;
    if (range <= 0) {
        return 1.0;
    }

    // Use ChartTools for consistent tick intervals
    return ChartTools::CustomCeil(range) / 10.0;
}

void ChartAxisManager::updateAxisAppearance()
{
    if (m_xAxis) {
        m_xAxis->setTitleText(m_xLabel);
    }
    if (m_yAxis) {
        m_yAxis->setTitleText(m_yLabel);
    }
}