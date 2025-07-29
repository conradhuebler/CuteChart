/*
 * CuteCharts - ChartView PIMPL implementation (Claude Generated)
 * Copyright (C) 2016-2023 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "chartview_pimpl.h"

// All heavy includes moved here - not visible to clients
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
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QtCore/QDebug>

#include <QtWidgets/QGraphicsTextItem>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>

#include <QtGui/QResizeEvent>

#ifdef DEBUG_ON
#include <QtCore/QDebug>
#endif

/**
 * @brief Private implementation class for ChartView (Claude Generated)
 *
 * This class contains all the actual implementation details that were
 * previously in ChartView. By moving them here, we reduce compilation
 * dependencies for clients that include chartview.h.
 */
class ChartView::ChartViewImpl {
public:
    explicit ChartViewImpl(ChartView* parent);
    ~ChartViewImpl();

    // Core Components (using our refactored architecture)
    std::unique_ptr<ChartConfiguration> configuration;
    std::unique_ptr<ChartAxisManager> axisManager;
    std::unique_ptr<ChartExporter> exporter;

    // Qt Chart Components
    QPointer<QChart> chart;
    ChartViewPrivate* chartPrivate;

    // UI Components
    QStackedWidget* centralWidget;
    QWidget* configureWidget;
    QPushButton* configButton;
    QPushButton* actionButton;
    QPushButton* ignoreButton;
    QGridLayout* centralLayout;
    ChartConfigDialog* chartConfigDialog;

    // State
    QString name;
    QString lastFilename;
    qreal yMax = 0.0;
    bool hasLegend = false;
    bool connected = false;
    bool modal = true;
    bool preventNotification = false;
    int applyAction = 0;

    // Series Management
    QVector<QPointer<QAbstractSeries>> seriesList;
    QVector<QPointer<PeakCallOut>> peakAnnotations;

    // Methods
    void setupUI();
    void setupActions();
    void connectSignals();
    void connectLegendCallbacks(QAbstractSeries* series, bool initialShowState);
    void applyConfigAction();

private:
    ChartView* q; // Back pointer to public interface
};

ChartView::ChartViewImpl::ChartViewImpl(ChartView* parent)
    : q(parent)
    , centralWidget(nullptr)
    , configureWidget(nullptr)
    , configButton(nullptr)
    , actionButton(nullptr)
    , ignoreButton(nullptr)
    , centralLayout(nullptr)
    , chartConfigDialog(nullptr)
{
    // Create chart and private view
    chart = new QChart();
    chartPrivate = new ChartViewPrivate(chart, q);

    // Create our refactored components
    configuration = std::make_unique<ChartConfiguration>(q);
    axisManager = std::make_unique<ChartAxisManager>(chart, q);
    exporter = std::make_unique<ChartExporter>(chart, q, q);

    // Initialize chart
    chart->legend()->setVisible(false);
    chart->legend()->setAlignment(Qt::AlignRight);

    // Setup UI
    setupUI();
    setupActions();
    connectSignals();

    // Set default strategies
    chartPrivate->setZoomStrategy(ZoomStrategy::Rectangular);
    chartPrivate->setSelectStrategy(SelectStrategy::None);
    axisManager->setAutoScaleStrategy(AutoScaleStrategy::SpaceScale);
    chartPrivate->setVerticalLineEnabled(false);

#ifdef DEBUG_ON
    qDebug() << "ChartViewImpl: Initialized with refactored components";
#endif
}

ChartView::ChartViewImpl::~ChartViewImpl()
{
    qDeleteAll(peakAnnotations);

#ifdef DEBUG_ON
    qDebug() << "ChartViewImpl: Destroyed";
#endif
}

void ChartView::ChartViewImpl::setupUI()
{
    centralWidget = new QStackedWidget(q);
    configureWidget = new QWidget(q);

    centralLayout = new QGridLayout(q);
    centralLayout->addWidget(chartPrivate, 0, 0, 1, 3);

    // Create control buttons
    configButton = new QPushButton(tr("Tools"), q);
    configButton->setFlat(true);
    configButton->setMaximumWidth(100);

    actionButton = new QPushButton(tr("Apply"), q);
    actionButton->setFlat(true);
    actionButton->setMaximumWidth(100);

    ignoreButton = new QPushButton(tr("Ignore"), q);
    ignoreButton->setFlat(true);
    ignoreButton->setMaximumWidth(100);

    centralLayout->addWidget(ignoreButton, 1, 0);
    centralLayout->addWidget(actionButton, 1, 1);
    centralLayout->addWidget(configButton, 1, 2);

    centralWidget->addWidget(configureWidget);
    configureWidget->setLayout(centralLayout);

    q->setWidget(centralWidget);
    q->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    q->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Create configuration dialog
    chartConfigDialog = new ChartConfigDialog(q);
    chartConfigDialog->setModal(modal);
}

void ChartView::ChartViewImpl::setupActions()
{
    // Connect button actions
    QObject::connect(actionButton, &QPushButton::clicked, [this]() {
        applyConfigAction();
    });

    QObject::connect(configButton, &QPushButton::clicked, [this]() {
        if (chartConfigDialog) {
            chartConfigDialog->setChartConfig(configuration->currentConfig());
            chartConfigDialog->show();
        }
    });
}

void ChartView::ChartViewImpl::connectSignals()
{
    // Connect chart private signals
    QObject::connect(chartPrivate, &ChartViewPrivate::zoomChanged, q, &ChartView::zoomChanged);
    QObject::connect(chartPrivate, &ChartViewPrivate::zoomRect, q, &ChartView::zoomRect);
    QObject::connect(chartPrivate, &ChartViewPrivate::scaleDown, q, &ChartView::scaleDown);
    QObject::connect(chartPrivate, &ChartViewPrivate::scaleUp, q, &ChartView::scaleUp);
    QObject::connect(chartPrivate, &ChartViewPrivate::addRect, q, &ChartView::addRect);
    QObject::connect(chartPrivate, &ChartViewPrivate::pointDoubleClicked, q, &ChartView::pointDoubleClicked);
    QObject::connect(chartPrivate, &ChartViewPrivate::escapeSelectMode, q, &ChartView::escapeSelectMode);
    QObject::connect(chartPrivate, &ChartViewPrivate::rightKey, q, &ChartView::rightKey);
    QObject::connect(chartPrivate, &ChartViewPrivate::leftKey, q, &ChartView::leftKey);

    // Connect configuration signals
    QObject::connect(configuration.get(), &ChartConfiguration::configurationChanged,
        q, &ChartView::configurationChanged);

    // Connect axis manager signals
    QObject::connect(axisManager.get(), &ChartAxisManager::axisRangeChanged,
        q, &ChartView::zoomChanged);
    QObject::connect(axisManager.get(), &ChartAxisManager::axisLabelsChanged,
        q, &ChartView::axisChanged);

    // Connect exporter signals
    QObject::connect(exporter.get(), &ChartExporter::lastDirectoryChanged,
        q, &ChartView::lastDirChanged);
    QObject::connect(exporter.get(), &ChartExporter::exportPresetAdded,
        [q](const QString& name, const QString& description) {
            q->exportSettingsFileAdded(name, description, QJsonObject());
        });
}

void ChartView::ChartViewImpl::connectLegendCallbacks(QAbstractSeries* series, bool initialShowState)
{
    // Implementation moved from original ChartView
    Q_UNUSED(series)
    Q_UNUSED(initialShowState)
    // TODO: Implement legend callbacks if needed
}

void ChartView::ChartViewImpl::applyConfigAction()
{
    // Implementation moved from original ChartView
    if (applyAction == 1) {
        // Apply pending configuration
        if (chartConfigDialog) {
            configuration->loadConfig(chartConfigDialog->getChartConfig());
        }
        applyAction = 0;
    } else if (applyAction == -1) {
        // Revert to previous configuration
        applyAction = 0;
    }
}

// === ChartView Public Interface Implementation ===

ChartView::ChartView()
    : QScrollArea()
    , d(std::make_unique<ChartViewImpl>(this))
{
#ifdef DEBUG_ON
    qDebug() << "ChartView: PIMPL version initialized";
#endif
}

ChartView::~ChartView() = default; // unique_ptr handles cleanup

void ChartView::setZoomStrategy(ZoomStrategy strategy)
{
    d->chartPrivate->setZoomStrategy(strategy);
}

void ChartView::setSelectStrategy(SelectStrategy strategy)
{
    d->chartPrivate->setSelectStrategy(strategy);
}

ZoomStrategy ChartView::currentZoomStrategy() const
{
    return d->chartPrivate->currentZoomStrategy();
}

SelectStrategy ChartView::currentSelectStrategy() const
{
    return d->chartPrivate->currentSelectStrategy();
}

void ChartView::addSeries(QAbstractSeries* series, bool callout)
{
    if (!series)
        return;

    d->chart->addSeries(series);
    d->axisManager->initializeAxes();

    if (callout) {
        // Add callout annotation
        PeakCallOut* callOut = new PeakCallOut(d->chart);
        d->peakAnnotations.append(callOut);
    }

    d->seriesList.append(series);
    d->connectLegendCallbacks(series, true);

#ifdef DEBUG_ON
    qDebug() << "ChartView: Series added, total:" << d->seriesList.size();
#endif
}

void ChartView::removeSeries(QAbstractSeries* series)
{
    if (series && d->chart) {
        d->chart->removeSeries(series);
        d->seriesList.removeAll(series);

#ifdef DEBUG_ON
        qDebug() << "ChartView: Series removed, remaining:" << d->seriesList.size();
#endif
    }
}

QList<QAbstractSeries*> ChartView::series() const
{
    return d->chart ? d->chart->series() : QList<QAbstractSeries*>();
}

QLineSeries* ChartView::addLinearSeries(qreal m, qreal n, qreal min, qreal max)
{
    LineSeries* series = new LineSeries();

    // Generate linear data points
    for (qreal x = min; x <= max; x += (max - min) / 100.0) {
        series->append(x, m * x + n);
    }

    addSeries(series);
    return series;
}

void ChartView::clearChart()
{
    if (d->chart) {
        d->chart->removeAllSeries();
        d->seriesList.clear();
        qDeleteAll(d->peakAnnotations);
        d->peakAnnotations.clear();

        emit chartCleared();

#ifdef DEBUG_ON
        qDebug() << "ChartView: Chart cleared";
#endif
    }
}

void ChartView::setAnimationEnabled(bool animation)
{
    if (d->chart) {
        if (!animation) {
            d->chart->setAnimationOptions(QChart::NoAnimation);
        } else {
            d->chart->setAnimationOptions(QChart::SeriesAnimations);
        }
    }
}

void ChartView::setName(const QString& name)
{
    d->name = name;
}

void ChartView::setTitle(const QString& str)
{
    if (d->chart) {
        d->chart->setTitle(str);
    }
}

qreal ChartView::YMax() const
{
    return d->yMax;
}

QPointer<QValueAxis> ChartView::axisX() const
{
    return d->axisManager->axisX();
}

QPointer<QValueAxis> ChartView::axisY() const
{
    return d->axisManager->axisY();
}

qreal ChartView::XMinRange() const
{
    return d->axisManager->getXRange().first;
}

qreal ChartView::XMaxRange() const
{
    return d->axisManager->getXRange().second;
}

qreal ChartView::YMinRange() const
{
    return d->axisManager->getYRange().first;
}

qreal ChartView::YMaxRange() const
{
    return d->axisManager->getYRange().second;
}

void ChartView::setXRange(qreal min, qreal max, bool nice)
{
    d->axisManager->setXRange(min, max, nice);
}

void ChartView::setYRange(qreal min, qreal max, bool nice)
{
    d->axisManager->setYRange(min, max, nice);
}

void ChartView::setXMin(qreal min, bool nice)
{
    d->axisManager->setXMin(min, nice);
}

void ChartView::setXMax(qreal max, bool nice)
{
    d->axisManager->setXMax(max, nice);
}

void ChartView::setYMin(qreal min, bool nice)
{
    d->axisManager->setYMin(min, nice);
}

void ChartView::setYMax(qreal max, bool nice)
{
    d->axisManager->setYMax(max, nice);
}

QJsonObject ChartView::currentChartConfig() const
{
    return d->configuration->currentConfig();
}

QJsonObject ChartView::currentFontConfig() const
{
    return d->configuration->currentFontConfig();
}

void ChartView::updateChartConfig(const QJsonObject& config, bool force)
{
    d->configuration->updateConfig(config, force);
}

void ChartView::setChartConfig(const QJsonObject& config)
{
    d->configuration->loadConfig(config);
}

void ChartView::forceChartConfig(const QJsonObject& config)
{
    d->configuration->forceConfig(config);
}

void ChartView::setFontConfig(const QJsonObject& config)
{
    d->configuration->setFontConfig(config);
}

void ChartView::addExportSetting(const QString& name, const QString& description, const QJsonObject& settings)
{
    ChartExporter::ExportSettings exportSettings = ChartExporter::settingsFromJson(settings);
    d->exporter->addExportPreset(name, description, exportSettings);
}

void ChartView::setModal(bool modal)
{
    d->modal = modal;
    if (d->chartConfigDialog) {
        d->chartConfigDialog->setModal(modal);
    }
}

void ChartView::setAutoScaleStrategy(AutoScaleStrategy strategy)
{
    d->axisManager->setAutoScaleStrategy(strategy);
}

void ChartView::setVerticalLineEnabled(bool enabled)
{
    d->chartPrivate->setVerticalLineEnabled(enabled);
}

void ChartView::setFont(const QString& font)
{
    QJsonObject fontConfig;
    fontConfig["fontFamily"] = font;
    d->configuration->setFontConfig(fontConfig);
}

QPointF ChartView::currentMousePosition() const
{
    return d->chartPrivate->currentMousePosition();
}

void ChartView::addVerticalLine(double positionX)
{
    d->chartPrivate->addVerticalLine(positionX);
}

bool ChartView::removeVerticalLine(double positionX)
{
    return d->chartPrivate->removeVerticalLine(positionX);
}

void ChartView::removeAllVerticalLines()
{
    d->chartPrivate->removeAllVerticalLines();
}

// === Slots Implementation ===

void ChartView::formatAxis()
{
    d->axisManager->formatAxis();
}

void ChartView::qtNiceNumbersScale()
{
    d->axisManager->applyQtNiceNumbersScale();
}

void ChartView::spaceScale()
{
    d->axisManager->applySpaceScale();
}

void ChartView::setXAxis(const QString& str)
{
    auto labels = d->axisManager->getAxisLabels();
    d->axisManager->setAxisLabels(str, labels.second);
    emit axisChanged();
}

void ChartView::setYAxis(const QString& str)
{
    auto labels = d->axisManager->getAxisLabels();
    d->axisManager->setAxisLabels(labels.first, str);
    emit axisChanged();
}

void ChartView::setSelectBox(const QPointF& topleft, const QPointF& bottomright)
{
    d->chartPrivate->setSelectBox(topleft, bottomright);
}

void ChartView::zoomRect(const QPointF& point1, const QPointF& point2)
{
    d->axisManager->zoomToRect(point1, point2);
}

void ChartView::resizeEvent(QResizeEvent* event)
{
    QScrollArea::resizeEvent(event);

    // Update callouts on resize
    for (PeakCallOut* callout : d->peakAnnotations) {
        if (callout) {
            callout->update();
        }
    }
}