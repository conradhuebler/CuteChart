/*
 * CuteCharts - An enhanced chart visualization framework based on Qt Charts
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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

/**
 * @file chartview.cpp
 * @brief Main scrollable chart widget (UI orchestration + config application).
 *
 * Architecture (Claude Generated, 2025): ChartView owns the QChart, the interaction
 * layer (ChartViewPrivate) and the tool/config UI, and delegates domain work to
 * dedicated components held as unique_ptr members:
 *   - ChartAxisManager : auto-scaling of the chart's axes (spaceScale / nice numbers).
 *                        Adopts ChartView's default axes via setAxes() in addSeries().
 *   - ChartExporter    : high-resolution render/export pipeline (ChartView::exportPNG
 *                        does the widget orchestration and calls createPixmap()).
 *   - ChartConfiguration : owns the config state (current/pending/last JSON + export
 *                        presets). ChartView keeps the dialog-driven Apply/Ignore/Revert
 *                        application logic (applying the config to the live chart/axes).
 */

#include "chartaxismanager.h"
#include "chartconfig.h"
#include "chartconfiguration.h"
#include "chartexporter.h"
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
    // Claude Generated: config state (current/pending/last + export presets) is owned by
    // ChartConfiguration; ChartView keeps the dialog-driven application logic.
    m_configuration = std::make_unique<ChartConfiguration>(this);
    m_configuration->setCurrentConfig(DefaultConfig);
    m_chart = new QChart();
    m_chart_private = new ChartViewPrivate(m_chart, this);
    // Claude Generated: export/render pipeline lives in a dedicated component
    m_exporter = std::make_unique<ChartExporter>(m_chart, this, this);
    // Claude Generated: axis scaling lives in a dedicated component (adopts the chart's
    // default axes once they are created in addSeries())
    m_axisManager = std::make_unique<ChartAxisManager>(m_chart, this);

    connect(m_chart_private, &ChartViewPrivate::zoomChanged, this, &ChartView::zoomChanged);
    connect(m_chart_private, &ChartViewPrivate::zoomRect, this, &ChartView::zoomRect);
    connect(m_chart_private, &ChartViewPrivate::scaleDown, this, &ChartView::scaleDown);
    connect(m_chart_private, &ChartViewPrivate::scaleUp, this, &ChartView::scaleUp);
    connect(m_chart_private, &ChartViewPrivate::addRect, this, &ChartView::addRect);
    connect(m_chart_private, &ChartViewPrivate::pointDoubleClicked, this, &ChartView::pointDoubleClicked);
    connect(m_chart_private, &ChartViewPrivate::escapeSelectMode, this, &ChartView::escapeSelectMode);
    connect(m_chart_private, &ChartViewPrivate::rightKey, this, &ChartView::rightKey);
    connect(m_chart_private, &ChartViewPrivate::leftKey, this, &ChartView::leftKey);

    m_chart->legend()->setVisible(false);
    m_chart->legend()->setAlignment(Qt::AlignRight);
    setUi();
    setZoomStrategy(ZoomStrategy::Rectangular);
    setSelectStrategy(SelectStrategy::None);
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
    connect(m_configure_series, &QAction::triggered, this, &ChartView::configure);

    QAction* plotsettings = new QAction(this);
    plotsettings->setText(tr("Plot Settings"));
    connect(plotsettings, &QAction::triggered, this, &ChartView::plotSettings);
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
        forceFormatAxis();
        m_autoscalestrategy = strategy;
    });

    menu->addAction(scaleAction);

    QAction* MinMaxscaleAction = new QAction(this);
    MinMaxscaleAction->setText(tr("Autoscale Min/Max"));
    connect(MinMaxscaleAction, &QAction::triggered, this, [this]() {
        AutoScaleStrategy strategy = m_autoscalestrategy;
        m_autoscalestrategy = AutoScaleStrategy::QtNiceNumbers;
        forceFormatAxis();
        m_autoscalestrategy = strategy;
    });
    menu->addAction(MinMaxscaleAction);

    QAction* exportpng = new QAction(this);
    exportpng->setText(tr("Export Diagram (PNG)"));
    connect(exportpng, &QAction::triggered, this, &ChartView::exportPNG);
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
    connect(saveConfig, &QAction::triggered, this, &ChartView::saveFontConfig);
    menu->addAction(saveConfig);

    QAction* loadConfig = new QAction(this);
    loadConfig->setText(tr("Load Font Config (Json)"));
    connect(loadConfig, &QAction::triggered, this, &ChartView::loadFontConfig);
    menu->addAction(loadConfig);

    m_select_strategy = new QMenu(tr("Select Strategy"));

    m_select_none = new QAction(tr("None"));
    m_select_none->setData(static_cast<int>(SelectStrategy::None));
    m_select_none->setCheckable(true);

    m_select_horizonal = new QAction(tr("Horizontal"));
    m_select_horizonal->setData(static_cast<int>(SelectStrategy::Horizontal));
    m_select_horizonal->setCheckable(true);

    m_select_vertical = new QAction(tr("Vertical"));
    m_select_vertical->setData(static_cast<int>(SelectStrategy::Vertical));
    m_select_vertical->setCheckable(true);

    m_select_rectangular = new QAction(tr("Rectangular"));
    m_select_rectangular->setData(static_cast<int>(SelectStrategy::Rectangular));
    m_select_rectangular->setCheckable(true);

    m_select_strategy->addAction(m_select_none);
    m_select_strategy->addAction(m_select_horizonal);
    m_select_strategy->addAction(m_select_vertical);
    m_select_strategy->addAction(m_select_rectangular);

    menu->addMenu(m_select_strategy);
    connect(m_select_strategy, &QMenu::triggered, m_select_strategy, [this](QAction* action) {
        SelectStrategy select = static_cast<SelectStrategy>(action->data().toInt());
        this->m_chart_private->setSelectStrategy(select);
        m_select_none->setChecked(select == SelectStrategy::None);
        m_select_horizonal->setChecked(select == SelectStrategy::Horizontal);
        m_select_vertical->setChecked(select == SelectStrategy::Vertical);
        m_select_rectangular->setChecked(select == SelectStrategy::Rectangular);
    });

    m_zoom_strategy = new QMenu(tr("Zoom Strategy"));

    m_zoom_none = new QAction(tr("None"));
    m_zoom_none->setData(static_cast<int>(ZoomStrategy::None));
    m_zoom_none->setCheckable(true);

    m_zoom_horizonal = new QAction(tr("Horizontal"));
    m_zoom_horizonal->setData(static_cast<int>(ZoomStrategy::Horizontal));
    m_zoom_horizonal->setCheckable(true);

    m_zoom_vertical = new QAction(tr("Vertical"));
    m_zoom_vertical->setData(static_cast<int>(ZoomStrategy::Vertical));
    m_zoom_vertical->setCheckable(true);

    m_zoom_rectangular = new QAction(tr("Rectangular"));
    m_zoom_rectangular->setData(static_cast<int>(ZoomStrategy::Rectangular));
    m_zoom_rectangular->setCheckable(true);

    m_zoom_strategy->addAction(m_zoom_none);
    m_zoom_strategy->addAction(m_zoom_horizonal);
    m_zoom_strategy->addAction(m_zoom_vertical);
    m_zoom_strategy->addAction(m_zoom_rectangular);

    menu->addMenu(m_zoom_strategy);
    connect(m_zoom_strategy, &QMenu::triggered, m_zoom_strategy, [this](QAction* action) {
        ZoomStrategy select = static_cast<ZoomStrategy>(action->data().toInt());
        this->m_chart_private->setZoomStrategy(select);
        m_zoom_none->setChecked(select == ZoomStrategy::None);
        m_zoom_horizonal->setChecked(select == ZoomStrategy::Horizontal);
        m_zoom_vertical->setChecked(select == ZoomStrategy::Vertical);
        m_zoom_rectangular->setChecked(select == ZoomStrategy::Rectangular);
    });

    m_ignore = new QPushButton(tr("Ignore"));
    m_ignore->setMaximumWidth(100);

    m_action_button = new QPushButton;
    m_action_button->setMaximumWidth(100);
    connect(m_action_button, &QPushButton::clicked, this, &ChartView::applyConfigAction);

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
        this->forceChartConfig(config);
        emit configurationChanged();
    });
    connect(m_chartconfigdialog, &ChartConfigDialog::ScaleAxis, this, &ChartView::forceFormatAxis);
#pragma message "TODO: connect to ChartConfigDialog::ResetFontConfig"
    // connect(m_chartconfigdialog, &ChartConfigDialog::ResetFontConfig, this, &ChartView::ResetFontConfig);

    connect(m_chart_private, &ChartViewPrivate::lockZoom, this, [this]() {
        this->m_lock_scaling = true;
        this->m_lock_action->setChecked(true);
    });

    connect(m_chart_private, &ChartViewPrivate::unlockZoom, this, [this]() {
        this->m_lock_scaling = false;
        this->m_lock_action->setChecked(false);
    });
    m_config->setEnabled(m_series.size());
}

void ChartView::addExportSetting(const QString& name, const QString& description, const QJsonObject& settings)
{
    if (m_configuration->hasExportSetting(name))
        return;
    m_configuration->addExportSetting(name, description, settings);
    m_exportMenu->clear();
    QAction* defaultAction = new QAction(tr("Default"));
    defaultAction->setData(DefaultConfig);
    m_exportMenu->addAction(defaultAction);
    const auto& stored = m_configuration->exportSettings();
    for (auto i = stored.constBegin(); i != stored.constEnd(); ++i) {
        QAction* action = new QAction(m_exportMenu);
        action->setText(i.key());
        action->setToolTip(i.value().first);
        action->setData(i.value().second);
        m_exportMenu->addAction(action);
    }
}

void ChartView::configure()
{
    if (m_centralWidget->currentIndex() == 0)
        m_centralWidget->setCurrentIndex(1);
    else
        m_centralWidget->setCurrentIndex(0);
}

void ChartView::setZoomStrategy(ZoomStrategy strategy)
{
    m_chart_private->setZoomStrategy(strategy);
    m_zoom_none->setChecked(strategy == ZoomStrategy::None);
    m_zoom_horizonal->setChecked(strategy == ZoomStrategy::Horizontal);
    m_zoom_vertical->setChecked(strategy == ZoomStrategy::Vertical);
    m_zoom_rectangular->setChecked(strategy == ZoomStrategy::Rectangular);
}

void ChartView::setSelectStrategy(SelectStrategy strategy)
{
    m_chart_private->setSelectStrategy(strategy);
    m_select_none->setChecked(strategy == SelectStrategy::None);
    m_select_horizonal->setChecked(strategy == SelectStrategy::Horizontal);
    m_select_vertical->setChecked(strategy == SelectStrategy::Vertical);
    m_select_rectangular->setChecked(strategy == SelectStrategy::Rectangular);
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
                annotation->setVisible(true);
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
            // Claude Generated: hand the freshly created axes to the axis manager so it
            // scales the very same axes the series are attached to
            m_axisManager->setAxes(m_XAxis, m_YAxis);
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
                show = line->showInLegend();
            } else if (scatter)

            {
                show = scatter->showInLegend();
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
                show = line->showInLegend();
            } else if (scatter)

            {
                show = scatter->showInLegend();
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

        show = line->showInLegend();
    } else if (scatter) {
        connect(scatter, &ScatterSeries::legendChanged, series, [this, series](bool legend) {
            bool vis = m_chart->legend()->isVisible();
            m_chart->legend()->setVisible(false);
            m_chart->legend()->markers(series).first()->setVisible(legend);
            m_chart->legend()->setVisible(vis);
        });

        show = scatter->showInLegend();
    }
    m_chart->legend()->markers(series).first()->setVisible(show);
    connect(series, &QAbstractSeries::visibleChanged, this, &ChartView::forceFormatAxis);
    if (!connected)
        if (connect(this, &ChartView::axisChanged, this, &ChartView::forceFormatAxis))
            connected = true;
    forceFormatAxis();
    m_config->setEnabled(m_series.size());
    emit setUpFinished();
}

void ChartView::clearChart()
{
    m_chart->removeAllSeries();
    emit chartCleared();
}

void ChartView::formatAxis()
{
    if (m_pending || m_chart->series().isEmpty())
        return;
    forceFormatAxis();
}

void ChartView::zoomRect(const QPointF& point1, const QPointF& point2)
{
    Q_UNUSED(point1)
    Q_UNUSED(point2)
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
    m_chart_private->updateZoom();
}

void ChartView::scaleAxis(QPointer<QValueAxis> axis, qreal& min, qreal& max)
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
        max = ChartTools::CustomCeil(max - mean) + mean;
        if (min && !(0 < min && min < 1))
            min = ChartTools::CustomFloor(min - mean) + mean;
        else
            min = 0;
    }

    int ticks = ChartTools::ScaleToNormalizedRange(max - min) / int(ChartTools::ScaleToNormalizedRange(max - min) / 5) + 1;
    axis->setTickCount(ticks);
    axis->setRange(min, max);
}

void ChartView::forceFormatAxis()
{
    if (m_lock_scaling || m_chart->series().size() == 0)
        return;
    m_pending = true;

    if (m_autoscalestrategy == AutoScaleStrategy::QtNiceNumbers)
        qtNiceNumbersScale();
    else if (m_autoscalestrategy == AutoScaleStrategy::SpaceScale)
        spaceScale();

    if (connected)
        m_chartconfigdialog->setChartConfig(getChartConfig());

    m_chart_private->updateZoom();

    // Claude Generated: clear the re-entrancy guard here, strategy-independent, so both
    // spaceScale() and qtNiceNumbersScale() leave it reset. Previously only spaceScale()
    // reset m_pending; after any QtNiceNumbers pass (e.g. the "Autoscale Min/Max" menu) it
    // latched true and formatAxis() became a permanent no-op — the chart appeared "frozen"
    // (stale axes, fit curve out of range) until opening a new model forced a rescale.
    m_pending = false;
}

void ChartView::spaceScale()
{
    // Claude Generated: delegate the space-scaling computation to ChartAxisManager, which
    // operates on the same axes (setAxes in addSeries). ChartView keeps only the bookkeeping
    // (current axis labels, cached ranges, m_pending flag) used by the rest of the class.
    m_axisManager->setAxisLabels(m_x_axis, m_y_axis);
    m_axisManager->applySpaceScale();

    const QPair<qreal, qreal> xRange = m_axisManager->getXRange();
    const QPair<qreal, qreal> yRange = m_axisManager->getYRange();
    m_xmin = xRange.first;
    m_xmax = xRange.second;
    m_ymin = yRange.first;
    m_ymax = yRange.second;
    // m_pending is cleared centrally in forceFormatAxis() (its sole caller).
}

void ChartView::qtNiceNumbersScale()
{
    // Claude Generated: delegate the Qt-nice-numbers scaling to ChartAxisManager (see spaceScale).
    m_axisManager->setAxisLabels(m_x_axis, m_y_axis);
    m_axisManager->applyQtNiceNumbersScale();

    const QPair<qreal, qreal> xRange = m_axisManager->getXRange();
    const QPair<qreal, qreal> yRange = m_axisManager->getYRange();
    m_xmin = xRange.first;
    m_xmax = xRange.second;
    m_ymin = yRange.first;
    m_ymax = yRange.second;
}

void ChartView::plotSettings()
{
    if (!connected)
        return;
    m_configuration->setCurrentConfig(getChartConfig());
    m_chartconfigdialog->setChartConfig(m_configuration->currentConfig());
    m_chartconfigdialog->show();
    m_chartconfigdialog->raise();
    m_chartconfigdialog->activateWindow();
}

void ChartView::updateAxisConfig(const QJsonObject& config, QAbstractAxis* axis)
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

void ChartView::forceChartConfig(const QJsonObject& config)
{
    QJsonObject tmp = ChartTools::MergeJsonObject(getChartConfig(), config);

    setChartConfig(tmp);
    m_apply_action = -1;
    m_action_button->setText(tr("Revert"));
    m_action_button->setStyleSheet("QPushButton {background-color: #BF593E; color: black;}");
    m_action_button->setHidden(false);
    m_ignore->setHidden(false);
}

void ChartView::updateChartConfig(const QJsonObject& config, bool force)
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
        m_configuration->setPendingConfig(tmp);
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
    if(!m_XAxis || !m_YAxis)
        return;

    /* Something very strange is going on here, If I did not copy the const QJsonObject,
       the config get modifed (although const) - GCC and Clang Qt 6.2.3, Manjaro Linux */
    QJsonObject config = chartconfig;
    m_configuration->setLastConfig(m_configuration->currentConfig());
    m_configuration->setCurrentConfig(chartconfig);

    m_lock_scaling = config["ScalingLocked"].toBool();
    m_x_size = config["xSize"].toDouble();
    m_y_size = config["ySize"].toDouble();
    m_scaling = config["Scaling"].toDouble();

    m_markerSize = config["markerSize"].toDouble();
    m_lineWidth = config["lineWidth"].toDouble();

    updateAxisConfig(config["xAxis"].toObject(), m_XAxis);
    updateAxisConfig(config["yAxis"].toObject(), m_YAxis);

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
    m_chartconfigdialog->setChartConfig(m_configuration->currentConfig());
}

void ChartView::applyConfigAction()
{
    if (m_apply_action == -1) {
        setChartConfig(m_configuration->lastConfig());
    } else if (m_apply_action == 1) {
        setChartConfig(ChartTools::MergeJsonObject(getChartConfig(), m_configuration->pendingConfig()));
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

QJsonObject ChartView::currentChartConfig() const
{
    // Claude Generated: config state lives in ChartConfiguration
    return m_configuration->currentConfig();
}

QJsonObject ChartView::getChartConfig() const
{
    QJsonObject chartconfig = m_configuration->currentConfig();

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

QJsonObject ChartView::currentFontConfig() const
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

QString ChartView::color2RGB(const QColor& color) const
{
    QString result;
    result = QString::number(color.toRgb().red()) + "," + QString::number(color.toRgb().green()) + "," + QString::number(color.toRgb().blue());
    return result;
}

void ChartView::exportPNG()
{
    const QString str = QFileDialog::getSaveFileName(this, tr("Save File"),
        qApp->instance()->property("lastDir").toString() + m_last_filename,
        tr("Images (*.png)"));
    if (str.isEmpty() || str.isNull())
        return;
    emit lastDirChanged(str);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Save current state
    bool verticalLineEnabled = m_chart_private->isVerticalLineEnabled();
    m_chart_private->setVerticalLineEnabled(false);
    QChart::AnimationOptions animation = m_chart->animationOptions();
    m_chart->setAnimationOptions(QChart::NoAnimation);

    // Prepare for export
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    // Cache the original size
    QSize widgetSize = m_centralWidget->size();

    // Resize for export
    m_chart->resize(m_x_size, m_y_size);
    m_centralWidget->resize(m_x_size, m_y_size);

    // Update callouts and scene
    for (PeakCallOut* call : m_peak_anno) {
        call->update();
    }
    m_chart->scene()->update();
    QApplication::processEvents();

    // Claude Generated: Delegate the faithful high-resolution render (grid removal, axis
    // emphasis, transparency, per-series export appearance and cropping) to ChartExporter.
    // ChartView keeps only the widget-level orchestration (resize, callouts, vertical line).
    const QJsonObject cfg = m_configuration->currentConfig();
    ChartExporter::ExportSettings settings;
    settings.scaling = m_scaling;
    settings.transparentBackground = cfg["transparentImage"].toBool();
    settings.cropImage = cfg["cropImage"].toBool();
    settings.noGrid = cfg["noGrid"].toBool();
    settings.emphasizeAxis = cfg["emphasizeAxis"].toBool();
    settings.markerSize = m_markerSize;
    settings.lineWidth = m_lineWidth;

    QPixmap pixmap = m_exporter->createPixmap(settings);

    // Restore widget state
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_centralWidget->resize(widgetSize);

    // Update callouts
    for (PeakCallOut* call : m_peak_anno) {
        call->update();
    }

    // Restore animation settings
    m_chart->setAnimationOptions(animation);
    m_chart_private->setVerticalLineEnabled(verticalLineEnabled);

    // Save the image
    m_last_filename = str;
    QFile file(str);
    file.open(QIODevice::WriteOnly);
    pixmap.save(&file, "PNG");

    QApplication::restoreOverrideCursor();
}


void ChartView::resizeEvent(QResizeEvent* event)
{
    event->accept();
    m_centralWidget->resize(0.99 * size());
}

void ChartView::saveFontConfig()
{
    const QString str = QFileDialog::getSaveFileName(this, tr("Save File"),
        qApp->instance()->property("lastDir").toString(),
        tr("Json(*.json)"));
    if (str.isEmpty() || str.isNull())
        return;

    QFile file(str);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << QJsonDocument(currentFontConfig()).toJson(QJsonDocument::Indented);
        file.close();
    }
}

void ChartView::loadFontConfig()
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
    // m_configuration->setCurrentConfig(doc.object());
    setFontConfig(doc.object());
    QFileInfo info(str);
    addExportSetting(info.baseName(), str, m_configuration->currentConfig());
    emit exportSettingsFileAdded(info.baseName(), str, m_configuration->currentConfig());
    m_chartconfigdialog->setChartConfig(currentChartConfig());
}

// Moved inline functions from header for header dependency reduction (Claude Generated)

ZoomStrategy ChartView::currentZoomStrategy() const
{
    return m_chart_private->currentZoomStrategy();
}

SelectStrategy ChartView::currentSelectStrategy() const
{
    return m_chart_private->currentSelectStrategy();
}

void ChartView::removeSeries(QAbstractSeries* series)
{
    m_chart->removeSeries(series);
}

QList<QAbstractSeries*> ChartView::series() const
{
    return m_chart->series();
}

void ChartView::setModal(bool modal)
{
    m_chartconfigdialog->setModal(modal);
}

void ChartView::setAutoScaleStrategy(AutoScaleStrategy strategy)
{
    m_autoscalestrategy = strategy;
}

qreal ChartView::YMaxRange() const
{
    if (m_hasAxis)
        return m_YAxis->max();
    else
        return 0;
}

qreal ChartView::YMinRange() const
{
    if (m_hasAxis)
        return m_YAxis->min();
    else
        return 0;
}

qreal ChartView::XMaxRange() const
{
    if (m_hasAxis)
        return m_XAxis->max();
    else
        return 0;
}

qreal ChartView::XMinRange() const
{
    if (m_hasAxis)
        return m_XAxis->min();
    else
        return 0;
}

void ChartView::setXRange(qreal xmin, qreal xmax, bool nice)
{
    if (m_hasAxis) {
        if (nice) {
            m_XAxis->setMin(ChartTools::NiceScalingMin(xmin));
            m_XAxis->setMax(ChartTools::NiceScalingMax(xmax));
        } else {
            m_XAxis->setMin(xmin);
            m_XAxis->setMax(xmax);
        }
        m_XAxis->setTickInterval(ChartTools::CustomCeil(xmax + xmin) / 10.0);
    }
}

void ChartView::setXMax(qreal xmax, bool nice)
{
    if (m_hasAxis) {
        if (nice)
            m_XAxis->setMax(ChartTools::NiceScalingMax(xmax));
        else
            m_XAxis->setMax(xmax);
    }
}

void ChartView::setXMin(qreal xmin, bool nice)
{
    if (m_hasAxis) {
        if (nice)
            m_XAxis->setMin(ChartTools::NiceScalingMin(xmin));
        else
            m_XAxis->setMin(xmin);
    }
}

void ChartView::setYRange(qreal ymin, qreal ymax, bool nice)
{
    if (m_hasAxis) {
        if (nice) {
            m_YAxis->setMin(ChartTools::NiceScalingMin(ymin));
            m_YAxis->setMax(ChartTools::NiceScalingMax(ymax));
        } else {
            m_YAxis->setMin(ymin);
            m_YAxis->setMax(ymax);
        }
        m_YAxis->setTickInterval(ChartTools::CustomCeil(ymax + ymin) / 10.0);
        m_chart_private->updateView(ymin, ymax);
    }
}

void ChartView::setYMax(qreal ymax, bool nice)
{
    if (m_hasAxis) {
        if (nice)
            m_YAxis->setMax(ChartTools::NiceScalingMax(ymax));
        else
            m_YAxis->setMax(ymax);
    }
}

void ChartView::setYMin(qreal ymin, bool nice)
{
    if (m_hasAxis) {
        if (nice)
            m_YAxis->setMin(ChartTools::NiceScalingMin(ymin));
        else
            m_YAxis->setMin(ymin);
    }
}

void ChartView::setName(const QString& name)
{
    m_name = name;
}

void ChartView::setVerticalLineEnabled(bool enabled)
{
    m_chart_private->setVerticalLineEnabled(enabled);
}

void ChartView::setFont(const QString& font)
{
    m_font = font;
}

void ChartView::addVerticalLine(double position_x)
{
    m_chart_private->addVerticalLine(position_x);
}

bool ChartView::removeVerticalLine(double position_x)
{
    return m_chart_private->removeVerticalLine(position_x);
}

void ChartView::removeAllVerticalLines()
{
    m_chart_private->removeAllVerticalLines();
}

void ChartView::setSelectBox(const QPointF& topleft, const QPointF& bottomright)
{
    m_chart_private->setSelectBox(topleft, bottomright);
}

QPointF ChartView::currentMousePosition() const
{
    return m_chart_private->currentMousePosition();
}

#include "chartview.moc"
