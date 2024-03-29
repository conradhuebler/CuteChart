/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "chartconfig.h"
#include "chartviewprivate.h"
#include "peakcallout.h"
#include "series.h"
#include "tools.h"

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QXYSeries>

#include <QtCore/QBuffer>
#include <QtCore/QDebug>
#include <QtCore/QMimeData>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>

#include <QtGui/QCursor>
#include <QtGui/QDrag>

#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>

#include <cmath>
#include <iostream>

#include "chartview.h"


ChartView::ChartView()
    : has_legend(false)
    , connected(false)
    , m_x_axis(QString())
    , m_y_axis(QString())
    , m_pending(false)
    , m_lock_scaling(false)
{
    m_currentChartConfig = DefaultConfig;
    m_chart = new QChart();
    m_chart_private = new ChartViewPrivate(m_chart, this);

    connect(m_chart_private, SIGNAL(ZoomChanged()), this, SIGNAL(ZoomChanged()));
    connect(m_chart_private, &ChartViewPrivate::ZoomRect, this, &ChartView::ZoomRect);
    connect(m_chart_private, SIGNAL(scaleDown()), this, SIGNAL(scaleDown()));
    connect(m_chart_private, SIGNAL(scaleUp()), this, SIGNAL(scaleUp()));
    connect(m_chart_private, SIGNAL(AddRect(const QPointF&, const QPointF&)), this, SIGNAL(AddRect(const QPointF&, const QPointF&)));
    connect(m_chart_private, &ChartViewPrivate::PointDoubleClicked, this, &ChartView::PointDoubleClicked);
    connect(m_chart_private, &ChartViewPrivate::EscapeSelectMode, this, &ChartView::EscapeSelectMode);
    connect(m_chart_private, &ChartViewPrivate::RightKey, this, &ChartView::RightKey);
    connect(m_chart_private, &ChartViewPrivate::LeftKey, this, &ChartView::LeftKey);

    m_chart->legend()->setVisible(false);
    m_chart->legend()->setAlignment(Qt::AlignRight);
    setUi();
    setZoomStrategy(ZoomStrategy::Z_Rectangular);
    setSelectStrategy(S_None);
    setAutoScaleStrategy(AutoScaleStrategy::SpaceScale);
    m_chart_private->setVerticalLineEnabled(false);
}

ChartView::~ChartView()
{
    qDeleteAll(m_peak_anno);
}

void ChartView::setAnimationEnabled(bool animation)
{
    if (!animation)
        m_chart->setAnimationOptions(QChart::NoAnimation);
    else
        m_chart->setAnimationOptions(QChart::SeriesAnimations);
}

void ChartView::setUi()
{
    m_name = "chart";
    mCentralLayout = new QGridLayout;
    QMenu* menu = new QMenu(this);

    m_configure_series = new QAction(this);
    m_configure_series->setText(tr("Configure"));
    connect(m_configure_series, SIGNAL(triggered()), this, SLOT(Configure()));

    QAction* plotsettings = new QAction(this);
    plotsettings->setText(tr("Plot Settings"));
    connect(plotsettings, SIGNAL(triggered()), this, SLOT(PlotSettings()));
    menu->addAction(plotsettings);

    m_lock_action = new QAction(this);
    m_lock_action->setText(tr("Lock Scaling"));
    m_lock_action->setCheckable(true);
    connect(m_lock_action, &QAction::triggered, this, [this]() {
        m_lock_scaling = m_lock_action->isChecked();
    });
    menu->addAction(m_lock_action);

    QAction* scaleAction = new QAction(this);
    scaleAction->setText(tr("Rescale Axis"));
    connect(scaleAction, &QAction::triggered, this, [this]() {
        AutoScaleStrategy strategy = m_autoscalestrategy;
        m_autoscalestrategy = AutoScaleStrategy::SpaceScale;
        forceformatAxis();
        m_autoscalestrategy = strategy;
    });

    menu->addAction(scaleAction);

    QAction* MinMaxscaleAction = new QAction(this);
    MinMaxscaleAction->setText(tr("Autoscale Min/Max"));
    connect(MinMaxscaleAction, &QAction::triggered, this, [this]() {
        AutoScaleStrategy strategy = m_autoscalestrategy;
        m_autoscalestrategy = AutoScaleStrategy::QtNiceNumbers;
        forceformatAxis();
        m_autoscalestrategy = strategy;
    });
    menu->addAction(MinMaxscaleAction);

    QAction* exportpng = new QAction(this);
    exportpng->setText(tr("Export Diagram (PNG)"));
    connect(exportpng, SIGNAL(triggered()), this, SLOT(ExportPNG()));
    menu->addAction(exportpng);

    m_exportMenu = new QMenu(tr("Export Style"));
    menu->addMenu(m_exportMenu);

    QAction* defaultAction = new QAction(tr("Default"));
    defaultAction->setData(DefaultConfig);
    m_exportMenu->addAction(defaultAction);

    connect(m_exportMenu, &QMenu::triggered, m_exportMenu, [this](QAction* action) {
        this->setFontConfig(action->data().toJsonObject());
    });

    QAction* saveConfig = new QAction(this);
    saveConfig->setText(tr("Save Font Config (Json)"));
    connect(saveConfig, SIGNAL(triggered()), this, SLOT(SaveFontConfig()));
    menu->addAction(saveConfig);

    QAction* loadConfig = new QAction(this);
    loadConfig->setText(tr("Load Font Config (Json)"));
    connect(loadConfig, SIGNAL(triggered()), this, SLOT(LoadFontConfig()));
    menu->addAction(loadConfig);

    m_select_strategy = new QMenu(tr("Select Strategy"));

    m_select_none = new QAction(tr("None"));
    m_select_none->setData(SelectStrategy::S_None);
    m_select_none->setCheckable(true);

    m_select_horizonal = new QAction(tr("Horizontal"));
    m_select_horizonal->setData(SelectStrategy::S_Horizontal);
    m_select_horizonal->setCheckable(true);

    m_select_vertical = new QAction(tr("Vertical"));
    m_select_vertical->setData(SelectStrategy::S_Vertical);
    m_select_vertical->setCheckable(true);

    m_select_rectangular = new QAction(tr("Rectangular"));
    m_select_rectangular->setData(SelectStrategy::S_Rectangular);
    m_select_rectangular->setCheckable(true);

    m_select_strategy->addAction(m_select_none);
    m_select_strategy->addAction(m_select_horizonal);
    m_select_strategy->addAction(m_select_vertical);
    m_select_strategy->addAction(m_select_rectangular);

    menu->addMenu(m_select_strategy);
    connect(m_select_strategy, &QMenu::triggered, m_select_strategy, [this](QAction* action) {
        SelectStrategy select = static_cast<SelectStrategy>(action->data().toInt());
        this->m_chart_private->setSelectStrategy(select);
        m_select_none->setChecked(select == SelectStrategy::S_None);
        m_select_horizonal->setChecked(select == SelectStrategy::S_Horizontal);
        m_select_vertical->setChecked(select == SelectStrategy::S_Vertical);
        m_select_rectangular->setChecked(select == SelectStrategy::S_Rectangular);
    });

    m_zoom_strategy = new QMenu(tr("Zoom Strategy"));

    m_zoom_none = new QAction(tr("None"));
    m_zoom_none->setData(ZoomStrategy::Z_None);
    m_zoom_none->setCheckable(true);

    m_zoom_horizonal = new QAction(tr("Horizontal"));
    m_zoom_horizonal->setData(ZoomStrategy::Z_Horizontal);
    m_zoom_horizonal->setCheckable(true);

    m_zoom_vertical = new QAction(tr("Vertical"));
    m_zoom_vertical->setData(ZoomStrategy::Z_Vertical);
    m_zoom_vertical->setCheckable(true);

    m_zoom_rectangular = new QAction(tr("Rectangular"));
    m_zoom_rectangular->setData(ZoomStrategy::Z_Rectangular);
    m_zoom_rectangular->setCheckable(true);

    m_zoom_strategy->addAction(m_zoom_none);
    m_zoom_strategy->addAction(m_zoom_horizonal);
    m_zoom_strategy->addAction(m_zoom_vertical);
    m_zoom_strategy->addAction(m_zoom_rectangular);

    menu->addMenu(m_zoom_strategy);
    connect(m_zoom_strategy, &QMenu::triggered, m_zoom_strategy, [this](QAction* action) {
        ZoomStrategy select = static_cast<ZoomStrategy>(action->data().toInt());
        this->m_chart_private->setZoomStrategy(select);
        m_zoom_none->setChecked(select == ZoomStrategy::Z_None);
        m_zoom_horizonal->setChecked(select == ZoomStrategy::Z_Horizontal);
        m_zoom_vertical->setChecked(select == ZoomStrategy::Z_Vertical);
        m_zoom_rectangular->setChecked(select == ZoomStrategy::Z_Rectangular);
    });

    m_ignore = new QPushButton(tr("Ignore"));
    m_ignore->setMaximumWidth(100);

    m_action_button = new QPushButton;
    // m_action_button->setFlat(true);
    m_action_button->setMaximumWidth(100);
    connect(m_action_button, &QPushButton::clicked, this, &ChartView::ApplyConfigAction);

    m_config = new QPushButton(tr("Tools"));
    m_config->setFlat(true);
    m_config->setIcon(QIcon::fromTheme("applications-system"));
    m_config->setMaximumWidth(100);
    m_config->setStyleSheet("QPushButton {background-color: #A3C1DA; color: black;}");
    m_config->setMenu(menu);

    mCentralLayout->addWidget(m_chart_private, 0, 0, 1, 5);
    mCentralLayout->addWidget(m_action_button, 0, 2, Qt::AlignTop);
    mCentralLayout->addWidget(m_ignore, 0, 3, Qt::AlignTop);
    mCentralLayout->addWidget(m_config, 0, 4, Qt::AlignTop);

    m_action_button->setHidden(true);
    m_ignore->setHidden(true);
    connect(m_ignore, &QPushButton::clicked, this, [this]() {
        m_action_button->setHidden(true);
        m_ignore->setHidden(true);
        m_apply_action = 0;
    });

    QWidget* firstPageWidget = new QWidget;
    m_configure = new QWidget;

    m_centralWidget = new QStackedWidget;
    m_centralWidget->addWidget(firstPageWidget);
    m_centralWidget->addWidget(m_configure);

    firstPageWidget->setLayout(mCentralLayout);

    setWidget(m_centralWidget);

    m_chartconfigdialog = new ChartConfigDialog(this);

    connect(m_chartconfigdialog, &ChartConfigDialog::ConfigChanged, this, [this](const QJsonObject& config) {
        this->ForceChartConfig(config);
        emit ConfigurationChanged();
    });
    connect(m_chartconfigdialog, SIGNAL(ScaleAxis()), this, SLOT(forceformatAxis()));
    connect(m_chartconfigdialog, SIGNAL(ResetFontConfig()), this, SLOT(ResetFontConfig()));

    connect(m_chart_private, &ChartViewPrivate::LockZoom, this, [this]() {
        this->m_lock_scaling = true;
        this->m_lock_action->setChecked(true);
    });

    connect(m_chart_private, &ChartViewPrivate::UnLockZoom, this, [this]() {
        this->m_lock_scaling = false;
        this->m_lock_action->setChecked(false);
    });
    m_config->setEnabled(m_series.size());
}

void ChartView::AddExportSetting(const QString& name, const QString& description, const QJsonObject& settings)
{
    if (m_stored_exportsettings.contains(name))
        return;
    m_stored_exportsettings.insert(name, QPair<QString, QJsonObject>(description, settings));
    m_exportMenu->clear();
    QAction* defaultAction = new QAction(tr("Default"));
    defaultAction->setData(DefaultConfig);
    m_exportMenu->addAction(defaultAction);
    QHash<QString, QPair<QString, QJsonObject>>::const_iterator i = m_stored_exportsettings.constBegin();
    while (i != m_stored_exportsettings.constEnd()) {
        QAction* action = new QAction(m_exportMenu);
        action->setText(i.key());
        action->setToolTip(i.value().first);
        action->setData(i.value().second);
        ++i;
        m_exportMenu->addAction(action);
    }
}

void ChartView::Configure()
{
    if (m_centralWidget->currentIndex() == 0)
        m_centralWidget->setCurrentIndex(1);
    else
        m_centralWidget->setCurrentIndex(0);
}

void ChartView::setZoomStrategy(ZoomStrategy strategy)
{
    m_chart_private->setZoomStrategy(strategy);
    m_zoom_none->setChecked(strategy == ZoomStrategy::Z_None);
    m_zoom_horizonal->setChecked(strategy == ZoomStrategy::Z_Horizontal);
    m_zoom_vertical->setChecked(strategy == ZoomStrategy::Z_Vertical);
    m_zoom_rectangular->setChecked(strategy == ZoomStrategy::Z_Rectangular);
}

void ChartView::setSelectStrategy(SelectStrategy strategy)
{
    m_chart_private->setSelectStrategy(strategy);
    m_select_none->setChecked(strategy == SelectStrategy::S_None);
    m_select_horizonal->setChecked(strategy == SelectStrategy::S_Horizontal);
    m_select_vertical->setChecked(strategy == SelectStrategy::S_Vertical);
    m_select_rectangular->setChecked(strategy == SelectStrategy::S_Rectangular);
}

QLineSeries* ChartView::addLinearSeries(qreal m, qreal n, qreal min, qreal max)
{
    qreal y_min = m * min + n;
    qreal y_max = m * max + n;
    QLineSeries* series = new QLineSeries(this);
    series->append(min, y_min);
    series->append(max, y_max);
    addSeries(series);
    m_config->setEnabled(m_series.size());
    return series;
}

void ChartView::addSeries(QAbstractSeries* series, bool callout)
{
    if (!m_chart->series().contains(series) || !series) {
        QPointer<QXYSeries> serie = qobject_cast<QXYSeries*>(series);
        if (serie) {
            if (serie->points().size() > 5e3)
                serie->setUseOpenGL(true);
            if (callout) {
                qreal x = 0;
                for (const QPointF& point : serie->points())
                    x += point.x();
                x /= double(serie->points().size());
                QPointF point(x, 1.5);

                QPointer<PeakCallOut> annotation = new PeakCallOut(m_chart);
                annotation->setSeries(series);
                annotation->setText(series->name(), point);
                annotation->setAnchor(point);
                annotation->setZValue(11);
                //annotation->updateGeometry();
                annotation->show();
                connect(series, &QAbstractSeries::visibleChanged, series, [series, annotation]() {
                    annotation->setVisible(series->isVisible());
                });
                connect(serie, &QXYSeries::colorChanged, serie, [serie, annotation]() {
                    annotation->setColor(serie->color());
                });
                connect(serie, &QXYSeries::nameChanged, serie, [serie, annotation, point]() {
                    annotation->setText(serie->name(), point);
                });
                annotation->setColor(serie->color());
                m_peak_anno.append(annotation);
            }
        }
        m_chart->addSeries(series);
        if (!m_hasAxis) {
            m_chart->createDefaultAxes();
            m_XAxis = qobject_cast<QValueAxis*>(m_chart->axes(Qt::Horizontal).first());
            m_YAxis = qobject_cast<QValueAxis*>(m_chart->axes(Qt::Vertical).first());

            m_XAxis->setLabelFormat("%2.2f");
            m_YAxis->setLabelFormat("%2.2f");

            m_hasAxis = true;
        } else {
            series->attachAxis(m_XAxis);
            series->attachAxis(m_YAxis);
        }
        m_series << series;
    }
    connect(series, &QAbstractSeries::nameChanged, series, [this, series]() {
        if (series) {
            bool show = false;
            QPointer<LineSeries> line = qobject_cast<LineSeries*>(series);
            QPointer<ScatterSeries> scatter = qobject_cast<ScatterSeries*>(series);
            if (line) {
                show = line->ShowInLegend();
            } else if (scatter)

            {
                show = scatter->ShowInLegend();
            }
            this->m_chart->legend()->markers(series).first()->setVisible(show);
        }
    });
    connect(series, &QAbstractSeries::visibleChanged, series, [this, series]() {
        if (series) {
            bool show = false;
            QPointer<LineSeries> line = qobject_cast<LineSeries*>(series);
            QPointer<ScatterSeries> scatter = qobject_cast<ScatterSeries*>(series);
            if (line) {
                show = line->ShowInLegend();
            } else if (scatter)

            {
                show = scatter->ShowInLegend();
            }
            if (series->isVisible())
                this->m_chart->legend()->markers(series).first()->setVisible(!show);
        }
    });
    bool show = false;
    QPointer<LineSeries> line = qobject_cast<LineSeries*>(series);
    QPointer<ScatterSeries> scatter = qobject_cast<ScatterSeries*>(series);
    if (line) {
        connect(line, &LineSeries::legendChanged, series, [this, series](bool legend) {
            bool vis = m_chart->legend()->isVisible();
            m_chart->legend()->setVisible(false);
            m_chart->legend()->markers(series).first()->setVisible(legend);
            m_chart->legend()->setVisible(vis);
        });

        show = line->ShowInLegend();
    } else if (scatter) {
        connect(scatter, &ScatterSeries::legendChanged, series, [this, series](bool legend) {
            bool vis = m_chart->legend()->isVisible();
            m_chart->legend()->setVisible(false);
            m_chart->legend()->markers(series).first()->setVisible(legend);
            m_chart->legend()->setVisible(vis);
        });

        show = scatter->ShowInLegend();
    }
    m_chart->legend()->markers(series).first()->setVisible(show);
    connect(series, SIGNAL(visibleChanged()), this, SLOT(forceformatAxis()));
    if (!connected)
        if (connect(this, SIGNAL(AxisChanged()), this, SLOT(forceformatAxis())))
            connected = true;
    forceformatAxis();
    m_config->setEnabled(m_series.size());
    emit setUpFinished();
}

void ChartView::ClearChart()
{
    m_chart->removeAllSeries();
    emit ChartCleared();
}

void ChartView::formatAxis()
{
    if (m_pending || m_chart->series().isEmpty())
        return;
    forceformatAxis();
}

void ChartView::ZoomRect(const QPointF& point1, const QPointF& point2)
{
    if (m_manual_zoom == true)
        return;
    /*
    qreal max = qMax(point1.x(), point2.x());
    qreal min = qMin(point1.x(), point2.x());
    m_XAxis->setRange(min, max);

    min = qMin(point1.y(), point2.y());
    max = qMax(point1.y(), point2.y());
    m_YAxis->setRange(min, max);
    */
    m_chart_private->UpdateZoom();
}

void ChartView::ScaleAxis(QPointer<QValueAxis> axis, qreal& min, qreal& max)
{
    /*
    min  = ChartTools::NiceFloor(min);
    max = ChartTools::NiceCeil(max);

    qreal start = 0;
    qreal step = 0;

    ChartTools::IdealInterval(min, max, start, step);

    axis->setRange(min, max);
    axis->setTickAnchor(start);
    axis->setTickInterval(step);



    return; */
    int mean = (max + min) / 2;

    if (1 < mean && mean < 10) {
        max = std::ceil(max);
        min = std::floor(min);
    } else {
        max = ChartTools::ceil(max - mean) + mean;
        if (min && !(0 < min && min < 1))
            min = ChartTools::floor(min - mean) + mean;
        else
            min = 0;
    }

    int ticks = ChartTools::scale(max - min) / int(ChartTools::scale(max - min) / 5) + 1;
    //ticks = 2*(max-min)-1;
    //if (ticks < 10) {
    axis->setTickCount(ticks);
    axis->setRange(min, max);
    //} else
    //    axis->applyNiceNumbers();
}

void ChartView::forceformatAxis()
{
    if (m_lock_scaling || m_chart->series().size() == 0)
        return;
    m_pending = true;

    if (m_autoscalestrategy == AutoScaleStrategy::QtNiceNumbers)
        QtNiceNumbersScale();
    else if (m_autoscalestrategy == AutoScaleStrategy::SpaceScale)
        SpaceScale();

    if (connected)
        m_chartconfigdialog->setChartConfig(getChartConfig());

    m_chart_private->UpdateZoom();
}

void ChartView::SpaceScale()
{
    qreal x_min = 0;
    qreal x_max = 0;
    qreal y_max = 0;
    qreal y_min = 0;
    int start = 0;
    for (QAbstractSeries* series : m_chart->series()) {
        QPointer<QXYSeries> serie = qobject_cast<QXYSeries*>(series);
        if (!serie)
            continue;
        if (!serie->isVisible())
            continue;

        QVector<QPointF> points = serie->points();
        if (start == 0 && points.size()) {
            y_min = points.first().y();
            y_max = points.first().y();

            x_min = points.first().x();
            x_max = points.first().x();
            start = 1;
        }
        for (int i = 0; i < points.size(); ++i) {
            y_min = qMin(y_min, points[i].y());
            y_max = qMax(y_max, points[i].y());

            x_min = qMin(x_min, points[i].x());
            x_max = qMax(x_max, points[i].x());
        }
    }

    ScaleAxis(m_XAxis, x_min, x_max);
    ScaleAxis(m_YAxis, y_min, y_max);

    m_XAxis->setTitleText(m_x_axis);
    m_YAxis->setTitleText(m_y_axis);

    m_pending = false;
    m_ymax = y_max;
    m_ymin = y_min;
    m_xmin = x_min;
    m_xmax = x_max;
}

void ChartView::QtNiceNumbersScale()
{
    qreal x_min = 1e12;
    qreal x_max = -1 * 1e12;
    qreal y_max = -1 * 1e12;
    qreal y_min = 1 * 1e12;
    int start = 0;

    for (QAbstractSeries* series : m_chart->series()) {
        QPointer<QXYSeries> serie = qobject_cast<QXYSeries*>(series);
        if (!serie)
            continue;
        if (!serie->isVisible())
            continue;

        QVector<QPointF> points = serie->points();
        if (start == 0 && points.size()) {
            y_min = points.first().y();
            y_max = points.first().y();

            x_min = points.first().x();
            x_max = points.first().x();
            start = 1;
        }
        for (int i = 0; i < points.size(); ++i) {
            y_min = qMin(y_min, points[i].y());
            y_max = qMax(y_max, points[i].y());

            x_min = qMin(x_min, points[i].x());
            x_max = qMax(x_max, points[i].x());
        }
    }

    m_XAxis->setRange(x_min, x_max);
    m_XAxis->applyNiceNumbers();
    m_YAxis->setRange(y_min, y_max);
    m_YAxis->applyNiceNumbers();

    m_XAxis->setTitleText(m_x_axis);
    m_YAxis->setTitleText(m_y_axis);

    m_ymax = y_max;
    m_ymin = y_min;
    m_xmin = x_min;
    m_xmax = x_max;
}

void ChartView::PlotSettings()
{
    if (!connected)
        return;
    m_currentChartConfig = getChartConfig();
    m_chartconfigdialog->setChartConfig(m_currentChartConfig);
    m_chartconfigdialog->show();
    m_chartconfigdialog->raise();
    m_chartconfigdialog->activateWindow();
}

void ChartView::UpdateAxisConfig(const QJsonObject& config, QAbstractAxis* axis)
{
    axis->setTitleText(config["Title"].toString());

    axis->setMin(config["Min"].toDouble());
    axis->setMax(config["Max"].toDouble());
    axis->setVisible(config["showAxis"].toBool());
    QPointer<QValueAxis> valueaxis = qobject_cast<QValueAxis*>(axis);
    if (valueaxis) {
        valueaxis->setTickType(config["TickType"].toInt() == 0 ? QValueAxis::TicksDynamic : QValueAxis::TicksFixed);
        valueaxis->setTickAnchor(config["TickAnchor"].toDouble());
        valueaxis->setLabelFormat(config["TickFormat"].toString());
        valueaxis->setTickInterval(config["TickInterval"].toDouble());
        valueaxis->setTickCount(config["TickCount"].toInt());
        valueaxis->setMinorTickCount(config["MinorTickCount"].toInt());
        valueaxis->setMinorGridLineVisible(config["MinorVisible"].toBool());
    }
}

void ChartView::ForceChartConfig(const QJsonObject& config)
{
    QJsonObject tmp = ChartTools::MergeJsonObject(getChartConfig(), config);

    setChartConfig(tmp);
    m_apply_action = -1;
    m_action_button->setText(tr("Revert"));
    m_action_button->setStyleSheet("QPushButton {background-color: #BF593E; color: black;}");
    m_action_button->setHidden(false);
    m_ignore->setHidden(false);
}

void ChartView::UpdateChartConfig(const QJsonObject& config, bool force)
{
    if (m_prevent_notification) {
        m_prevent_notification = false;
        return;
    }

    QJsonObject tmp = ChartTools::MergeJsonObject(getChartConfig(), config);
    if (force) {
        setChartConfig(tmp);
        m_apply_action = -1;
        m_action_button->setText(tr("Revert"));
        m_action_button->setStyleSheet("QPushButton {background-color: #BF593E; color: black;}");
    } else {
        m_pendingChartConfig = tmp;
        m_apply_action = 1;
        m_action_button->setText(tr("Apply"));
        m_action_button->setStyleSheet("QPushButton {background-color: #00CC00; color: black;}");
    }
    m_action_button->setHidden(false);
    m_ignore->setHidden(false);
}

void ChartView::setChartConfig(const QJsonObject& chartconfig)
{
    // It seems in SupraFit is a leak, resulting in incomplete ChartView object - this fixes it
    // qDebug() << 1;
    if(!m_XAxis || !m_YAxis)
        return;
    // qDebug() << 2;

    /* Something very strange is going on here, If I did not copy the const QJsonObject, the config get modifed (although const) - GCC and Clang Qt 6.2.3, Manjaro Linux */
    QJsonObject config = chartconfig;
    m_lastChartConfig = m_currentChartConfig;
    m_currentChartConfig = chartconfig;

    m_lock_scaling = config["ScalingLocked"].toBool();
    m_x_size = config["xSize"].toDouble();
    m_y_size = config["ySize"].toDouble();
    m_scaling = config["Scaling"].toDouble();

    m_markerSize = config["markerSize"].toDouble();
    m_lineWidth = config["lineWidth"].toDouble();

    UpdateAxisConfig(config["xAxis"].toObject(), m_XAxis);
    UpdateAxisConfig(config["yAxis"].toObject(), m_YAxis);

    QFont keyFont;
    keyFont.fromString(config["KeyFont"].toString());
    m_chart->legend()->setFont(keyFont);

    if (config["Legend"].toBool()) {
        m_chart->legend()->setVisible(true);
        if (config["Alignment"].toInt() == Qt::AlignTop
            || config["Alignment"].toInt() == Qt::AlignBottom
            || config["Alignment"].toInt() == Qt::AlignLeft
            || config["Alignment"].toInt() == Qt::AlignRight)
            m_chart->legend()->setAlignment(Qt::Alignment(config["Alignment"].toInt()));
        else
            m_chart->legend()->setAlignment(Qt::AlignRight);
        for (PeakCallOut* call : m_peak_anno)
            call->setFont(keyFont);
    } else {
        m_chart->legend()->setVisible(false);
    }
    setTitle(config["Title"].toString());

    int Theme = config["Theme"].toInt();
    if (Theme < 8)
        m_chart->setTheme(static_cast<QChart::ChartTheme>(Theme));
    else {
        for (int i = 0; i < m_series.size(); ++i) {
            if (!m_series[i])
                continue;

            if (qobject_cast<QXYSeries*>(m_series[i])) {
                QXYSeries* series = qobject_cast<QXYSeries*>(m_series[i]);
                series->setColor(QColor("black"));
                if (qobject_cast<QScatterSeries*>(series)) {
                    qobject_cast<QScatterSeries*>(series)->setBorderColor(QColor("black"));
                }
            } else if (qobject_cast<QAreaSeries*>(m_series[i])) {
                QAreaSeries* series = qobject_cast<QAreaSeries*>(m_series[i]);
                QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
                gradient.setColorAt(0.0, QColor("darkGray"));
                gradient.setColorAt(1.0, QColor("lightGray"));
                gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
                series->setBrush(gradient);
                series->setOpacity(0.4);
                QPen pen(QColor("darkGray"));
                pen.setWidth(3);
                series->setPen(pen);
            }
            QBrush brush;
            brush.setColor(Qt::transparent);
            m_chart->setBackgroundBrush(brush);
            m_chart->setTitleBrush(QBrush(Qt::black));
            m_YAxis->setTitleBrush(QBrush(Qt::black));
            m_XAxis->setTitleBrush(QBrush(Qt::black));
            m_XAxis->setLabelsBrush(QBrush(Qt::black));
            m_YAxis->setLabelsBrush(QBrush(Qt::black));
        }
    }
    for (QPointer<PeakCallOut>& call : m_peak_anno) {
        if (!call)
            continue;
        call->setVisible(config["Annotation"].toBool());
        call->setFont(config["KeyFont"].toString());
    }
    setFontConfig(config);
    m_apply_action = 1;
    m_action_button->setHidden(false);
    m_prevent_notification = true;

    QSignalBlocker block(m_chartconfigdialog);
    m_chartconfigdialog->setChartConfig(m_currentChartConfig);
}

void ChartView::ApplyConfigAction()
{
    if (m_apply_action == -1) {
        setChartConfig(m_lastChartConfig);
    } else if (m_apply_action == 1) {
        setChartConfig(ChartTools::MergeJsonObject(getChartConfig(), m_pendingChartConfig));
    }
    m_action_button->setHidden(true);
    m_ignore->setHidden(true);
    m_prevent_notification = false;
}

void ChartView::setFontConfig(const QJsonObject& chartconfig)
{
    if (m_XAxis) {
        QJsonObject axis = chartconfig["xAxis"].toObject();

        QFont font;
        font.fromString(axis["TitleFont"].toString());
        m_XAxis->setTitleFont(font);

        font.fromString(axis["TicksFont"].toString());
        m_XAxis->setLabelsFont(font);
    }

    if (m_YAxis) {
        QJsonObject axis = chartconfig["yAxis"].toObject();

        QFont font;
        font.fromString(axis["TitleFont"].toString());
        m_YAxis->setTitleFont(font);
        font.fromString(axis["TicksFont"].toString());
        m_YAxis->setLabelsFont(font);
    }

    QFont font;
    font.fromString(chartconfig["TitleFont"].toString());
    m_chart->setTitleFont(font);

    QFont keyFont;
    keyFont.fromString(chartconfig["KeyFont"].toString());
    m_chart->legend()->setFont(keyFont);
}

void ChartView::setTitle(const QString& str)
{
    m_chart->setTitle(str);
}

QJsonObject ChartView::getAxisConfig(const QAbstractAxis* axis) const
{
    QJsonObject config;
    config["Title"] = axis->titleText();

    config["showAxis"] = axis->isVisible();

    QPointer<const QValueAxis> valueaxis = qobject_cast<const QValueAxis*>(axis);
    if (valueaxis) {
        config["TickType"] = valueaxis->tickType() == QValueAxis::TicksDynamic ? 0 : 1;
        config["TickAnchor"] = valueaxis->tickAnchor();
        config["TickFormat"] = valueaxis->labelFormat();
        config["TickInterval"] = valueaxis->tickInterval();
        config["TickCount"] = valueaxis->tickCount();
        config["MinorTickCount"] = valueaxis->minorTickCount();
        config["MinorVisible"] = valueaxis->isMinorGridLineVisible();
        config["Min"] = valueaxis->min();
        config["Max"] = valueaxis->max();
        config["TitleFont"] = valueaxis->titleFont().toString();
        config["TicksFont"] = valueaxis->labelsFont().toString();
    }
    return config;
}

QJsonObject ChartView::getChartConfig() const
{
    QJsonObject chartconfig = m_currentChartConfig;

    if (m_hasAxis) {
        chartconfig["xAxis"] = getAxisConfig(m_XAxis);
        chartconfig["yAxis"] = getAxisConfig(m_YAxis);
    }
    chartconfig["Legend"] = m_chart->legend()->isVisible();
    chartconfig["ScalingLocked"] = m_lock_scaling;
    chartconfig["xSize"] = m_x_size;
    chartconfig["ySize"] = m_y_size;
    chartconfig["Scaling"] = m_scaling;
    chartconfig["lineWidth"] = m_lineWidth;
    chartconfig["markerSize"] = m_markerSize;

    chartconfig["KeyFont"] = m_chart->legend()->font().toString();
    chartconfig["Alignment"] = static_cast<int>(m_chart->legend()->alignment());
    chartconfig["Title"] = m_chart->title();
    chartconfig["TitleFont"] = m_chart->titleFont().toString();

    return chartconfig;
}

QJsonObject ChartView::CurrentFontConfig() const
{
    QJsonObject font;
    font["KeyFont"] = m_chart->legend()->font().toString();
    font["TitleFont"] = m_chart->titleFont().toString();
    QJsonObject tmp = getAxisConfig(m_XAxis);
    QJsonObject axis;
    axis["TitleFont"] = tmp["TitleFont"].toString();
    axis["TicksFont"] = tmp["TicksFont"].toString();
    font["xAxis"] = axis;
    tmp = getAxisConfig(m_YAxis);
    axis["TitleFont"] = tmp["TitleFont"].toString();
    axis["TicksFont"] = tmp["TicksFont"].toString();
    font["yAxis"] = axis;
    return font;
}

QString ChartView::Color2RGB(const QColor& color) const
{
    QString result;
    result = QString::number(color.toRgb().red()) + "," + QString::number(color.toRgb().green()) + "," + QString::number(color.toRgb().blue());
    return result;
}

void ChartView::ExportPNG()
{
    const QString str = QFileDialog::getSaveFileName(this, tr("Save File"),
        qApp->instance()->property("lastDir").toString() + m_last_filename,
        tr("Images (*.png)"));
    if (str.isEmpty() || str.isNull())
        return;
    emit LastDirChanged(str);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    bool verticalline = m_chart_private->isVerticalLineEnabled();
    m_chart_private->setVerticalLineEnabled(false);

    QChart::AnimationOptions animation = m_chart->animationOptions();

    m_chart->setAnimationOptions(QChart::NoAnimation);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    // first cache the original size, within SupraFit
    QSize widgetSize = m_centralWidget->size();

    // and resize as set in settings
    m_chart->resize(m_x_size, m_y_size);
    m_centralWidget->resize(m_x_size, m_y_size);

    for (PeakCallOut* call : m_peak_anno) {
        call->update();
    }
    m_chart->scene()->update();
    QApplication::processEvents();

    int w = m_chart->rect().size().width();
    int h = m_chart->rect().size().height();
    // scaling is important for good resolution
    QImage image(QSize(m_scaling * w, m_scaling * h), QImage::Format_ARGB32);

    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    bool xGrid = m_XAxis->isGridLineVisible();
    bool yGrid = m_YAxis->isGridLineVisible();

    // hide grid lines
    if (m_currentChartConfig["noGrid"].toBool() == true) {
        m_XAxis->setGridLineVisible(false);
        m_YAxis->setGridLineVisible(false);
    }

    QPen xPen = m_XAxis->linePen();
    QPen yPen = m_YAxis->linePen();

    QPen pen = m_XAxis->linePen();
    pen.setColor(Qt::black);
    pen.setWidth(2);

    // make axis stronger
    if (m_currentChartConfig["emphasizeAxis"].toBool() == true) {
        m_XAxis->setLinePen(pen);
        m_YAxis->setLinePen(pen);
    }
    QBrush brush_backup = m_chart->backgroundBrush();
    QBrush brush;
    brush.setColor(Qt::transparent);

    // remove background of chart
    if (m_currentChartConfig["transparentImage"].toBool() == true)
        m_chart->setBackgroundBrush(brush);

    // cache all individual series size and border colors and remove border colors and resize series

    QList<QColor> colors;
    QList<double> size, width;
    QList<bool> openGl;
    for (QAbstractSeries* serie : m_chart->series()) {
        if (qobject_cast<QScatterSeries*>(serie)) {
            colors << qobject_cast<QScatterSeries*>(serie)->borderColor();
            qobject_cast<QScatterSeries*>(serie)->setBorderColor(Qt::transparent);
            size << qobject_cast<QScatterSeries*>(serie)->markerSize();

            // if (qobject_cast<QScatterSeries*>(serie)->markerSize() != m_markerSize)
            qobject_cast<QScatterSeries*>(serie)->setMarkerSize(m_markerSize);

        } else if (qobject_cast<LineSeries*>(serie)) {
            width << qobject_cast<LineSeries*>(serie)->LineWidth();
            qobject_cast<LineSeries*>(serie)->setLineWidth(m_lineWidth);
        }
        openGl << serie->useOpenGL();
        serie->setUseOpenGL(false);
    }

    // do the painting!!
    m_chart->scene()->render(&painter, QRectF(0, 0, m_scaling * w, m_scaling * h), m_chart->rect());

    /*
     * copyied from here:
     *
     * https://stackoverflow.com/questions/3720947/does-qt-have-a-way-to-find-bounding-box-of-an-image
     *
     */
    // This part is kept of historical reasons ;-)
    /*
    auto border = [](const QImage& tmp) -> QImage {
        int l = tmp.width(), r = 0, t = tmp.height(), b = 0;
        for (int y = 0; y < tmp.height(); ++y) {
            QRgb* row = (QRgb*)tmp.scanLine(y);
            bool rowFilled = false;
            for (int x = 0; x < tmp.width(); ++x) {
                if (qAlpha(row[x])) {
                    rowFilled = true;
                    r = std::max(r, x);
                    if (l > x) {
                        l = x;
                        x = r; // shortcut to only search for new right bound from here
                    }
                }
            }
            if (rowFilled) {
                t = std::min(t, y);
                b = y;
            }
        }
        return tmp.copy(l + 1, t + 1, r + 1, b + 1);
    };
    */
    QPixmap pixmap;

    // remove transparent border of resulting image

    if (m_currentChartConfig["cropImage"].toBool() == true) {

        // As is this part of the code
        /*
        QImage mirrored = border(image);//border(border(image.mirrored(true, true)).mirrored(true, true).mirrored(true, true)).mirrored(true, true);
        pixmap = QPixmap::fromImage(border(mirrored));
        */

        QRect region = QRegion(QBitmap::fromImage(image.createMaskFromColor(0x00000000))).boundingRect();
        pixmap = QPixmap::fromImage(image.copy(region));
    } else
        pixmap = QPixmap::fromImage(image);

    // restore background brush
    m_chart->setBackgroundBrush(brush_backup);

    // restore series colors and size
    for (QAbstractSeries* serie : m_chart->series()) {
        if (qobject_cast<QScatterSeries*>(serie)) {
            qobject_cast<QScatterSeries*>(serie)->setBorderColor(colors.takeFirst());
            qobject_cast<QScatterSeries*>(serie)->setMarkerSize(size.takeFirst());
        } else if (qobject_cast<LineSeries*>(serie)) {
            qobject_cast<LineSeries*>(serie)->setLineWidth(width.takeFirst());
        }
        serie->setUseOpenGL(openGl.takeFirst());
    }

    // bring back the grids
    m_XAxis->setGridLineVisible(xGrid);
    m_YAxis->setGridLineVisible(yGrid);

    // bring back the old and weak axis
    m_XAxis->setLinePen(xPen);
    m_YAxis->setLinePen(yPen);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // restore old size
    m_centralWidget->resize(widgetSize);

    for (PeakCallOut* call : m_peak_anno)
        call->Update();

    // and nothing ever happens -> Lemon Tree
    m_chart->setAnimationOptions(animation);
    m_last_filename = str;
    QFile file(str);
    file.open(QIODevice::WriteOnly);
    pixmap.save(&file, "PNG");

    m_chart_private->setVerticalLineEnabled(verticalline);

    QApplication::restoreOverrideCursor();
}


void ChartView::resizeEvent(QResizeEvent* event)
{
    event->accept();
    m_centralWidget->resize(0.99 * size());
}

void ChartView::SaveFontConfig()
{
    const QString str = QFileDialog::getSaveFileName(this, tr("Save File"),
        qApp->instance()->property("lastDir").toString(),
        tr("Json(*.json)"));
    if (str.isEmpty() || str.isNull())
        return;

    QFile file(str);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << QJsonDocument(CurrentFontConfig()).toJson(QJsonDocument::Indented);
        file.close();
    }
}

void ChartView::LoadFontConfig()
{
    const QString str = QFileDialog::getOpenFileName(this, tr("Open File"),
        qApp->instance()->property("lastDir").toString(),
        tr("Json (*.json)"));
    if (str.isEmpty() || str.isNull())
        return;

    QFile file(str);
    if (!file.open(QIODevice::ReadOnly))
        return;
    auto content = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(content);
    // m_currentChartConfig = doc.object();
    setFontConfig(doc.object());
    QFileInfo info(str);
    AddExportSetting(info.baseName(), str, m_currentChartConfig);
    emit ExportSettingsFileAdded(info.baseName(), str, m_currentChartConfig);
    m_chartconfigdialog->setChartConfig(CurrentChartConfig());
}

#include "chartview.moc"
