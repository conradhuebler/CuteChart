/*
 * CuteCharts - Refactored ChartView with decomposed responsibilities
 * Copyright (C) 2016-2023 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "tools.h"

#include <QtWidgets/QScrollArea>

#include <QtCore/QJsonObject>
#include <QtCore/QPointer>

#include <memory>

// Forward declarations to reduce header dependencies
class QChart;
class QAbstractSeries;
class QLineSeries;
class QValueAxis;
class QStackedWidget;
class QGridLayout;
class QPushButton;
class QAction;
class QMenu;

class ChartViewPrivate;
class ChartConfiguration;
class ChartAxisManager;
class ChartExporter;
class ChartConfigDialog;
class PeakCallOut;

/**
 * @brief Refactored ChartView with separated concerns
 *
 * This refactored version delegates responsibilities to specialized components:
 * - ChartConfiguration: Handles all configuration management
 * - ChartAxisManager: Manages axis operations and scaling
 * - ChartExporter: Handles export functionality
 * - ChartViewPrivate: Manages chart interaction (unchanged)
 */
class ChartView : public QScrollArea {
    Q_OBJECT

public:
    explicit ChartView();
    ~ChartView() override;

    // === Strategy Management ===
    void setZoomStrategy(ZoomStrategy strategy);
    void setSelectStrategy(SelectStrategy strategy);
    ZoomStrategy currentZoomStrategy() const;
    SelectStrategy currentSelectStrategy() const;

    // === Series Management ===
    void addSeries(QAbstractSeries* series, bool callout = false);
    void removeSeries(QAbstractSeries* series);
    QList<QAbstractSeries*> series() const;
    QLineSeries* addLinearSeries(qreal m, qreal n, qreal min, qreal max);
    void clearChart();

    // === Chart Properties ===
    void setAnimationEnabled(bool animation);
    void setName(const QString& name);
    void setTitle(const QString& str);
    qreal YMax() const { return m_yMax; }

    // === Axis Access (delegated to ChartAxisManager) ===
    QPointer<QValueAxis> axisX() const;
    QPointer<QValueAxis> axisY() const;

    qreal XMinRange() const;
    qreal XMaxRange() const;
    qreal YMinRange() const;
    qreal YMaxRange() const;

    void setXRange(qreal min, qreal max, bool nice = false);
    void setYRange(qreal min, qreal max, bool nice = false);
    void setXMin(qreal min, bool nice = false);
    void setXMax(qreal max, bool nice = false);
    void setYMin(qreal min, bool nice = false);
    void setYMax(qreal max, bool nice = false);

    // === Configuration Access (delegated to ChartConfiguration) ===
    QJsonObject currentChartConfig() const;
    QJsonObject currentFontConfig() const;
    void updateChartConfig(const QJsonObject& config, bool force = false);
    void setChartConfig(const QJsonObject& config);
    void forceChartConfig(const QJsonObject& config);
    void setFontConfig(const QJsonObject& config);

    // === Export Access (delegated to ChartExporter) ===
    void addExportSetting(const QString& name, const QString& description, const QJsonObject& settings);

    // === Direct Access to Components ===
    ChartConfiguration* configuration() const { return m_configuration.get(); }
    ChartAxisManager* axisManager() const { return m_axisManager.get(); }
    ChartExporter* exporter() const { return m_exporter.get(); }
    ChartViewPrivate* privateView() const { return m_chartPrivate; }
    QChart* chart() const { return m_chart; }

    // === UI Settings ===
    void setModal(bool modal);
    void setAutoScaleStrategy(AutoScaleStrategy strategy);
    void setVerticalLineEnabled(bool enabled);
    void setFont(const QString& font);

    // === Interaction ===
    QPointF currentMousePosition() const;
    void addVerticalLine(double positionX);
    bool removeVerticalLine(double positionX);
    void removeAllVerticalLines();

public slots:
    // === Axis Operations (delegated) ===
    void formatAxis();
    void qtNiceNumbersScale();
    void spaceScale();
    void setXAxis(const QString& str);
    void setYAxis(const QString& str);

    // === Selection and Zoom ===
    void setSelectBox(const QPointF& topleft, const QPointF& bottomright);
    void zoomRect(const QPointF& point1, const QPointF& point2);

signals:
    void setUpFinished();
    void axisChanged();
    void chartCleared();
    void configurationChanged();
    void lastDirChanged(const QString& dir);
    void pointDoubleClicked(const QPointF& point);
    void zoomChanged();
    void scaleUp();
    void scaleDown();
    void addRect(const QPointF& point1, const QPointF& point2);
    void escapeSelectMode();
    void rightKey();
    void leftKey();
    void exportSettingsFileAdded(const QString& name, const QString& description, const QJsonObject& data);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void plotSettings();
    void configure();
    void exportPNG();
    void forceFormatAxis();

private:
    // === Core Components ===
    std::unique_ptr<ChartConfiguration> m_configuration;
    std::unique_ptr<ChartAxisManager> m_axisManager;
    std::unique_ptr<ChartExporter> m_exporter;

    // === Qt Chart Components ===
    QPointer<QChart> m_chart;
    ChartViewPrivate* m_chartPrivate;

    // === UI Components ===
    QStackedWidget* m_centralWidget;
    QWidget* m_configure;
    QPushButton* m_config;
    QPushButton* m_actionButton;
    QPushButton* m_ignore;
    QGridLayout* m_centralLayout;
    ChartConfigDialog* m_chartConfigDialog;

    // === Actions and Menus ===
    QAction* m_lockAction;
    QAction* m_configureSeries;
    QAction* m_selectNone;
    QAction* m_selectHorizontal;
    QAction* m_selectVertical;
    QAction* m_selectRectangular;
    QAction* m_zoomNone;
    QAction* m_zoomHorizontal;
    QAction* m_zoomVertical;
    QAction* m_zoomRectangular;
    QMenu* m_selectStrategy;
    QMenu* m_zoomStrategy;
    QMenu* m_exportMenu;

    // === State ===
    QString m_name;
    QString m_lastFilename;
    qreal m_yMax = 0.0;
    bool m_hasLegend = false;
    bool m_connected = false;
    bool m_modal = true;
    bool m_preventNotification = false;
    int m_applyAction = 0;

    // === Series Management ===
    QVector<QPointer<QAbstractSeries>> m_series;
    QVector<QPointer<PeakCallOut>> m_peakAnnotations;

    // === Private Methods ===
    void setupUI();
    void setupActions();
    void setupMenus();
    void connectSignals();
    void connectLegendCallbacks(QAbstractSeries* series, bool initialShowState);
    void applyConfigAction();
};