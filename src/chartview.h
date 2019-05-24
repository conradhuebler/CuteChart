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

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QtCore/QPointer>

#include <QtWidgets/QScrollArea>

class QGridLayout;
class QPushButton;
class QChart;

class PeakCallOut;

struct ChartConfig;
struct PgfPlotConfig {
    QString colordefinition;
    QString plots;
    QStringList table;
};

class ChartViewPrivate : public QtCharts::QChartView {
    Q_OBJECT
public:
    inline ChartViewPrivate(QtCharts::QChart* chart, QWidget* parent = Q_NULLPTR)
        : QtCharts::QChartView(parent)
        , m_vertical_line_visible(false)
    {
        setChart(chart);
        setAcceptDrops(true);
        setRenderHint(QPainter::Antialiasing, true);
        setRubberBand(QChartView::RectangleRubberBand);
        addVerticalSeries();
        /*QFont font = chart->titleFont();
        font.setBold(true);
        font.setPointSize(12);
        chart->setTitleFont(font);*/
    }

    inline ~ChartViewPrivate() override {}

public slots:
    inline void UpdateVerticalLine(double x)
    {
        m_vertical_series->replace(0, QPointF(x, m_min));
        m_vertical_series->replace(1, QPointF(x, m_max));
        m_vertical_series->setName(QString::number(x, 'f', 4));
    }

    inline void UpdateView(double min, double max)
    {
        m_min = min;
        m_max = max;
    }

    void UpdateSelectionChart(const QPointF& point);

    inline void setVerticalLineEnabled(bool enabled)
    {
        m_vertical_series->setVisible(enabled);
        m_vertical_line_visible = enabled;
    }

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    inline void addVerticalSeries()
    {
        m_vertical_series = new QtCharts::QLineSeries;
        QPen pen = m_vertical_series->pen();
        pen.setWidth(1);
        pen.setColor(Qt::gray);
        m_vertical_series->setPen(pen);
        QPointF start = QPointF(0, -1);
        QPointF end = QPointF(0, 10);

        m_vertical_series->append(start);
        m_vertical_series->append(end);
        chart()->addSeries(m_vertical_series);
        m_vertical_series->setVisible(m_vertical_line_visible);
        //chart()->legend()->markers(m_vertical_series).first()->setVisible(false);

        m_upper = new QtCharts::QLineSeries;
        m_upper->append(0, 0);
        m_upper->append(0, 0);
        m_lower = new QtCharts::QLineSeries;
        m_lower->append(0, 0);
        m_lower->append(0, 0);

        m_area = new QtCharts::QAreaSeries(m_upper, m_lower);

        chart()->addSeries(m_area);
        m_area->hide();
    }

    void handleMouseMoved(const QPointF& point);

    QtCharts::QLineSeries *m_vertical_series, *m_upper, *m_lower;
    QtCharts::QAreaSeries* m_area;

    QPointF rect_start;
    double m_min, m_max;
    bool m_double_clicked, m_vertical_line_visible = false;

signals:
    void LockZoom();
    void UnLockZoom();
    void ZoomChanged();
    void scaleUp();
    void scaleDown();
    void AddRect(const QPointF& point1, const QPointF& point2);
    void PointDoubleClicked(const QPointF& point);
};

class ChartView : public QScrollArea {
    Q_OBJECT
public:
    ChartView();
    ~ChartView() override;

    void setSelectionStrategie(int strategy);

    void addSeries(QtCharts::QAbstractSeries* series, bool callout = false);

    void setAnimationEnabled(bool animation);

    qreal YMax() const { return m_ymax; }
    inline void removeSeries(QtCharts::QAbstractSeries* series) { m_chart->removeSeries(series); }
    inline QList<QtCharts::QAbstractSeries*> series() const { return m_chart->series(); }

    QtCharts::QLineSeries* addLinearSeries(qreal m, qreal n, qreal min, qreal max);
    void ClearChart();
    inline void setModal(bool modal) { m_chartconfigdialog->setModal(modal); }

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

    inline void setXRange(qreal xmin, qreal xmax)
    {
        if (m_hasAxis) {
            m_XAxis->setMin(xmin);
            m_XAxis->setMax(xmax);
        }
    }

    inline void setXMax(qreal xmax)
    {
        if (m_hasAxis) {
            m_XAxis->setMax(xmax);
        }
    }

    inline void setXMin(qreal xmin)
    {
        if (m_hasAxis) {
            m_XAxis->setMin(xmin);
        }
    }

    inline void setYRange(qreal ymin, qreal ymax)
    {
        if (m_hasAxis) {
            m_YAxis->setMin(ymin);
            m_YAxis->setMax(ymax);
        }
    }

    inline void setYMax(qreal ymax)
    {
        if (m_hasAxis) {
            m_YAxis->setMax(ymax);
        }
    }

    inline void setYMin(qreal ymin)
    {
        if (m_hasAxis) {
            m_YAxis->setMin(ymin);
        }
    }

    inline void setName(const QString& name)
    {
        m_name = name;
        ReadSettings();
    }

    QtCharts::QChart* Chart() { return m_chart; }
    QPointer<QtCharts::QValueAxis> axisY() const { return m_YAxis; }
    QPointer<QtCharts::QValueAxis> axisX() const { return m_XAxis; }

    inline void setVerticalLineEnabled(bool enabled)
    {
        m_chart_private->setVerticalLineEnabled(enabled);
    }

public slots:
    void formatAxis();

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

private:
    QWidget* mCentralHolder;

    QAction* m_lock_action;
    ChartViewPrivate* m_chart_private;
    QPointer<QtCharts::QChart> m_chart;
    QPushButton* m_config;
    void setUi();
    bool has_legend, connected, m_hasAxis = false;
    QString m_x_axis, m_y_axis;
    ChartConfig getChartConfig() const;
    PgfPlotConfig getScatterTable() const;
    PgfPlotConfig getLineTable() const;
    QString Color2RGB(const QColor& color) const;
    void WriteTable(const QString& str);
    ChartConfigDialog* m_chartconfigdialog;
    bool m_pending, m_lock_scaling, m_modal = true;
    qreal m_ymax, m_ymin, m_xmin, m_xmax;
    QVector<QPointer<QtCharts::QAbstractSeries>> m_series;
    QVector<QPointer<PeakCallOut>> m_peak_anno;
    ChartConfig ReadSettings();

    ChartConfig m_last_config;
    void WriteSettings(const ChartConfig& chartconfig);
    QString m_name;
    QPointer<QtCharts::QValueAxis> m_XAxis, m_YAxis;

    void ScaleAxis(QPointer<QtCharts::QValueAxis> axis, qreal& min, qreal& max);
    QGridLayout* mCentralLayout;

    int m_x_size = 0, m_y_size = 0, m_scaling = 0;
    qreal m_lineWidth = 4, m_markerSize = 8;

private slots:
    void PlotSettings();
    void ExportPNG();
    void setChartConfig(const ChartConfig& chartconfig);
    void forceformatAxis();
    void ResetFontConfig();

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

protected:
    virtual void resizeEvent(QResizeEvent* event) override;
};
