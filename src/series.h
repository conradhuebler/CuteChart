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

#pragma once

#include <QtCore/QDebug>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <QtCharts/QBoxPlotSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QVXYModelMapper>
#include <QtCharts/QXYSeries>

#include "boxwhisker.h"

class LineSeries : public QtCharts::QLineSeries {
    Q_OBJECT

public:
    LineSeries();
    virtual inline ~LineSeries() override {}

public slots:
    virtual void setColor(const QColor& color) override
    {
        m_color = color;
        QPen pen = QtCharts::QLineSeries::pen();
        pen.setColor(color);
        setPen(pen);
    }
    inline void setDashDotLine(bool dashdot)
    {
        m_dashdot = dashdot;
        QPen pen = QtCharts::QLineSeries::pen();
        if (m_dashdot)
            pen.setStyle(Qt::DashDotLine);
        else
            pen.setStyle(Qt::SolidLine);
        setPen(pen);
    }
    inline void setSize(int size)
    {
        m_size = size;
        QPen pen = QtCharts::QLineSeries::pen();
        pen.setWidth(m_size);
        setPen(pen);
    }

    inline qreal LineWidth() const { return m_size; }

    void ShowLine(int state);
    void ShowLine(bool state);
    virtual void setName(const QString& name);

    inline QColor Color() const { return m_color; }

private:
    inline void Update()
    {
        QPen pen = QtCharts::QLineSeries::pen();

        if (m_dashdot)
            pen.setStyle(Qt::DashDotLine);
        else
            pen.setStyle(Qt::SolidLine);

        pen.setWidth(m_size);
        pen.setColor(m_color);
        setPen(pen);
    }
    bool m_dashdot;
    int m_size;
    QColor m_color;
};

class ScatterSeries : public QtCharts::QScatterSeries {
    Q_OBJECT

public:
    ScatterSeries();
    virtual inline ~ScatterSeries() override {}

public slots:
    virtual void setColor(const QColor& color) override;
    void ShowLine(int state);

signals:
    void NameChanged(const QString& str);
    void visibleChanged(int state);
};

class BoxPlotSeries : public QtCharts::QBoxPlotSeries {
    Q_OBJECT

public:
    BoxPlotSeries(const BoxWhisker& boxwhisker);
    inline QColor color() const { return brush().color(); }

public slots:
    void setColor(const QColor& color);
    virtual void setVisible(bool visible);

private:
    void LoadBoxWhisker();
    BoxWhisker m_boxwhisker;
    bool m_visible;
};
