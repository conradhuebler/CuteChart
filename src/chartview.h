/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "chartconfig.h"
#include "chartviewprivate.h"
#include "tools.h"

#include <QtWidgets/QGraphicsTextItem>

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QtCore/QDebug>
#include <QtCore/QPointer>

#include <QtWidgets/QScrollArea>

class QGridLayout;
class QPushButton;
class QChart;
class QStackedWidget;

class PeakCallOut;

struct ChartConfig;

class ChartView : public QScrollArea {
    Q_OBJECT

public:
    ChartView();
    ~ChartView() override;

    void setZoomStrategy(ZoomStrategy strategy);
    void setSelectStrategy(SelectStrategy strategy);

    inline ZoomStrategy CurrentZoomStrategy() const { return m_chart_private->CurrentZoomStrategy(); }
    inline SelectStrategy CurrentSelectStrategy() const { return m_chart_private->CurrentSelectStrategy(); }

    void addSeries(QAbstractSeries* series, bool callout = false);

    void setAnimationEnabled(bool animation);

    qreal YMax() const { return m_ymax; }
    inline void removeSeries(QAbstractSeries* series) { m_chart->removeSeries(series); }
    inline QList<QAbstractSeries*> series() const { return m_chart->series(); }

    QLineSeries* addLinearSeries(qreal m, qreal n, qreal min, qreal max);
    void ClearChart();
    inline void setModal(bool modal) { m_chartconfigdialog->setModal(modal); }

    inline void setAutoScaleStrategy(AutoScaleStrategy strategy) { m_autoscalestrategy = strategy; }

    inline qreal YMaxRange() const
    {
        if (m_hasAxis)
            return m_YAxis->max();
        else
            return 0;
    }
    inline qreal YMinRange() const
    {
        if (m_hasAxis)
            return m_YAxis->min();
        else
            return 0;
    }

    inline qreal XMaxRange() const
    {
        if (m_hasAxis)
            return m_XAxis->max();
        else
            return 0;
    }
    inline qreal XMinRange() const
    {
        if (m_hasAxis)
            return m_XAxis->min();
        else
            return 0;
    }

    inline void setXRange(qreal xmin, qreal xmax, bool nice = false)
    {
        if (m_hasAxis) {
            if (nice) {
                m_XAxis->setMin(ChartTools::NiceScalingMin(xmin));
                m_XAxis->setMax(ChartTools::NiceScalingMax(xmax));
            } else {
                m_XAxis->setMin(xmin);
                m_XAxis->setMax(xmax);
            }
            m_XAxis->setTickInterval(ChartTools::ceil(xmax + xmin) / 10.0);
        }
    }

    inline void setXMax(qreal xmax, bool nice = false)
    {
        if (m_hasAxis) {
            if (nice)
                m_XAxis->setMax(ChartTools::NiceScalingMax(xmax));
            else
                m_XAxis->setMax(xmax);
        }
    }

    inline void setXMin(qreal xmin, bool nice = false)
    {
        if (m_hasAxis) {
            if (nice)
                m_XAxis->setMin(ChartTools::NiceScalingMin(xmin));
            else
                m_XAxis->setMin(xmin);
        }
    }

    inline void setYRange(qreal ymin, qreal ymax, bool nice = false)
    {
        if (m_hasAxis) {
            if (nice) {
                m_YAxis->setMin(ChartTools::NiceScalingMin(ymin));
                m_YAxis->setMax(ChartTools::NiceScalingMax(ymax));
            } else {
                m_YAxis->setMin(ymin);
                m_YAxis->setMax(ymax);
            }
            m_YAxis->setTickInterval(ChartTools::ceil(ymax + ymin) / 10.0);
            m_chart_private->UpdateView(ymin, ymax);
        }
    }

    inline void setYMax(qreal ymax, bool nice)
    {
        if (m_hasAxis) {
            if (nice)
                m_YAxis->setMax(ChartTools::NiceScalingMax(ymax));
            else
                m_YAxis->setMax(ymax);
        }
    }

    inline void setYMin(qreal ymin, bool nice)
    {
        if (m_hasAxis) {
            if (nice)
                m_YAxis->setMin(ChartTools::NiceScalingMin(ymin));
            else
                m_YAxis->setMin(ymin);
        }
    }

    inline void setName(const QString& name)
    {
        m_name = name;
    }

    QChart* Chart() { return m_chart; }
    ChartViewPrivate* PrivateView() { return m_chart_private; }

    QPointer<QValueAxis> axisY() const { return m_YAxis; }
    QPointer<QValueAxis> axisX() const { return m_XAxis; }

    inline void setVerticalLineEnabled(bool enabled)
    {
        m_chart_private->setVerticalLineEnabled(enabled);
    }

    inline void setFont(const QString& font) { m_font = font; }
    QPointF currentMousePosition() const { return m_chart_private->currentMousePosition(); }

public slots:
    void setSelectBox(const QPointF& topleft, const QPointF& bottomright) { m_chart_private->setSelectBox(topleft, bottomright); }

    void formatAxis();
    void QtNiceNumbersScale();
    void SpaceScale();

    void setXAxis(const QString& str)
    {
        m_x_axis = str;
        emit AxisChanged();
    }
    void setYAxis(const QString& str)
    {
        m_y_axis = str;
        emit AxisChanged();
    }
    void setTitle(const QString& str);

    void ApplyConfigurationChange(const QString& str = "noname");

    void ZoomRect(const QPointF& point1, const QPointF& point2);

private:
    QStackedWidget* m_centralWidget;
    QWidget* m_configure;

    QAction* m_lock_action;
    ChartViewPrivate* m_chart_private;
    QPointer<QChart> m_chart;
    QPushButton* m_config;
    void setUi();
    bool has_legend, connected, m_hasAxis = false, m_manual_zoom = false;
    QString m_x_axis, m_y_axis;
    QJsonObject getChartConfig() const;
    QString Color2RGB(const QColor& color) const;
    void WriteTable(const QString& str);
    ChartConfigDialog* m_chartconfigdialog;
    bool m_pending, m_lock_scaling, m_modal = true;
    qreal m_ymax, m_ymin, m_xmin, m_xmax;
    QVector<QPointer<QAbstractSeries>> m_series;
    QVector<QPointer<PeakCallOut>> m_peak_anno;
    // ChartConfig ReadSettings();

    QAction *m_configure_series, *m_select_none, *m_select_horizonal, *m_select_vertical, *m_select_rectangular, *m_zoom_none, *m_zoom_horizonal, *m_zoom_vertical, *m_zoom_rectangular;
    QMenu *m_select_strategy, *m_zoom_strategy;

    // ChartConfig m_last_config;
    // void WriteSettings(const ChartConfig& chartconfig);

    QJsonObject m_currentChartConfig;
    // void WriteSettings(const QJsonObject& chartconfig);

    QString m_name;
    QPointer<QValueAxis> m_XAxis, m_YAxis;

    void ScaleAxis(QPointer<QValueAxis> axis, qreal& min, qreal& max);
    QGridLayout* mCentralLayout;

    int m_x_size = 0, m_y_size = 0, m_scaling = 0;
    qreal m_lineWidth = 4, m_markerSize = 8;

    QString m_font;
    AutoScaleStrategy m_autoscalestrategy;

private slots:
    void PlotSettings();
    void SaveFontConfig();
    void LoadFontConfig();
    void ExportPNG();
    void setChartConfig(const QJsonObject& chartconfig);
    void setFontConfig(const QJsonObject& chartconfig);

    void forceformatAxis();
    void ResetFontConfig();
    void Configure();

signals:
    void AxisChanged();
    void ChartCleared();
    void ConfigurationChanged();
    void LastDirChanged(const QString& dir);
    void PointDoubleClicked(const QPointF& point);
    void ZoomChanged();
    void scaleUp();
    void scaleDown();
    void AddRect(const QPointF& point1, const QPointF& point2);
    void EscapeSelectMode();
    void RightKey();
    void LeftKey();

protected:
    virtual void resizeEvent(QResizeEvent* event) override;
};
