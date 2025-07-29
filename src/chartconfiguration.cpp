/*
 * CuteCharts - Configuration management component (Claude Generated)
 * Copyright (C) 2023 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "chartconfiguration.h"
#include "tools.h"

#include <QtCharts/QAbstractAxis>
#include <QtCharts/QValueAxis>

#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QSettings>

#include <QtWidgets/QApplication>

#ifdef DEBUG_ON
#include <QtCore/QDebug>
#endif

// Default chart configuration settings (moved from chartview.h)
const QJsonObject DefaultChartConfig{
    { "Title", "" },
    { "Legend", false },
    { "ScalingLocked", false },
    { "Annotation", false },
    { "xSize", 600 },
    { "ySize", 400 },
    { "Scaling", 2 },
    { "lineWidth", 4 },
    { "markerSize", 8 },
    { "Theme", 0 },
    { "cropImage", true },
    { "transparentImage", true },
    { "emphasizeAxis", true },
    { "noGrid", true }
};

ChartConfiguration::ChartConfiguration(QObject* parent)
    : QObject(parent)
    , m_currentConfig(DefaultChartConfig)
    , m_font("Arial")
{
#ifdef DEBUG_ON
    qDebug() << "ChartConfiguration: Initialized with default config";
#endif
}

QJsonObject ChartConfiguration::getDefaultConfig()
{
    return DefaultChartConfig;
}

QJsonObject ChartConfiguration::currentFontConfig() const
{
    QJsonObject fontConfig;
    fontConfig["fontFamily"] = m_font;
    fontConfig["fontSize"] = 12;
    fontConfig["bold"] = false;
    fontConfig["italic"] = false;

    // Include current chart config font-related settings
    if (m_currentConfig.contains("fontSettings")) {
        fontConfig = ChartTools::MergeJsonObject(fontConfig, m_currentConfig["fontSettings"].toObject());
    }

    return fontConfig;
}

void ChartConfiguration::loadConfig(const QJsonObject& config, bool force)
{
#ifdef DEBUG_ON
    qDebug() << "ChartConfiguration: Loading config, force =" << force;
#endif

    if (!force && !validateConfig(config)) {
#ifdef DEBUG_ON
        qDebug() << "ChartConfiguration: Config validation failed";
#endif
        return;
    }

    QJsonObject oldConfig = m_currentConfig;
    m_currentConfig = mergeConfigs(m_currentConfig, config);

    if (isConfigDifferent(oldConfig, m_currentConfig)) {
        emit configurationChanged(m_currentConfig);
#ifdef DEBUG_ON
        qDebug() << "ChartConfiguration: Configuration changed and signal emitted";
#endif
    }
}

void ChartConfiguration::updateConfig(const QJsonObject& config, bool force)
{
    if (force) {
        forceConfig(config);
        return;
    }

    m_pendingConfig = config;
    applyPendingConfig();
}

void ChartConfiguration::forceConfig(const QJsonObject& config)
{
#ifdef DEBUG_ON
    qDebug() << "ChartConfiguration: Force applying config";
#endif

    m_lastConfig = m_currentConfig;
    m_currentConfig = mergeConfigs(m_currentConfig, config);

    emit configurationChanged(m_currentConfig);
}

void ChartConfiguration::setFontConfig(const QJsonObject& fontConfig)
{
#ifdef DEBUG_ON
    qDebug() << "ChartConfiguration: Setting font config";
#endif

    if (fontConfig.contains("fontFamily")) {
        m_font = fontConfig["fontFamily"].toString();
    }

    // Store font settings in current config
    QJsonObject currentConfig = m_currentConfig;
    currentConfig["fontSettings"] = fontConfig;
    m_currentConfig = currentConfig;

    m_currentFontConfig = fontConfig;
    emit fontConfigurationChanged(fontConfig);
}

void ChartConfiguration::saveFontConfig()
{
    QSettings settings;
    QJsonObject fontConfig = currentFontConfig();

    QJsonDocument doc(fontConfig);
    settings.setValue("chartFontConfig", doc.toJson());

#ifdef DEBUG_ON
    qDebug() << "ChartConfiguration: Font config saved to settings";
#endif
}

void ChartConfiguration::loadFontConfig()
{
    QSettings settings;
    QByteArray configData = settings.value("chartFontConfig").toByteArray();

    if (!configData.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(configData);
        if (!doc.isNull() && doc.isObject()) {
            setFontConfig(doc.object());
#ifdef DEBUG_ON
            qDebug() << "ChartConfiguration: Font config loaded from settings";
#endif
        }
    }
}

void ChartConfiguration::addExportSetting(const QString& name, const QString& description, const QJsonObject& settings)
{
    m_exportSettings[name] = qMakePair(description, settings);
    emit exportSettingAdded(name, description, settings);

#ifdef DEBUG_ON
    qDebug() << "ChartConfiguration: Export setting added:" << name;
#endif
}

QJsonObject ChartConfiguration::getExportSetting(const QString& name) const
{
    if (m_exportSettings.contains(name)) {
        return m_exportSettings[name].second;
    }
    return QJsonObject();
}

QStringList ChartConfiguration::getExportSettingNames() const
{
    return m_exportSettings.keys();
}

void ChartConfiguration::updateAxisConfig(const QJsonObject& config, QAbstractAxis* axis)
{
    if (!axis) {
#ifdef DEBUG_ON
        qDebug() << "ChartConfiguration: Cannot update null axis";
#endif
        return;
    }

    if (auto* valueAxis = qobject_cast<QValueAxis*>(axis)) {
        if (config.contains("min")) {
            valueAxis->setMin(config["min"].toDouble());
        }
        if (config.contains("max")) {
            valueAxis->setMax(config["max"].toDouble());
        }
        if (config.contains("tickCount")) {
            valueAxis->setTickCount(config["tickCount"].toInt());
        }
        if (config.contains("labelFormat")) {
            valueAxis->setLabelFormat(config["labelFormat"].toString());
        }
        if (config.contains("titleText")) {
            valueAxis->setTitleText(config["titleText"].toString());
        }
    }

#ifdef DEBUG_ON
    qDebug() << "ChartConfiguration: Axis config updated";
#endif
}

QJsonObject ChartConfiguration::getAxisConfig(const QAbstractAxis* axis) const
{
    QJsonObject config;

    if (!axis) {
        return config;
    }

    if (const auto* valueAxis = qobject_cast<const QValueAxis*>(axis)) {
        config["min"] = valueAxis->min();
        config["max"] = valueAxis->max();
        config["tickCount"] = valueAxis->tickCount();
        config["labelFormat"] = valueAxis->labelFormat();
        config["titleText"] = valueAxis->titleText();
    }

    return config;
}

bool ChartConfiguration::validateConfig(const QJsonObject& config) const
{
    // Basic validation - check for required fields and types
    if (config.contains("xSize") && !config["xSize"].isDouble()) {
        return false;
    }
    if (config.contains("ySize") && !config["ySize"].isDouble()) {
        return false;
    }
    if (config.contains("Scaling") && !config["Scaling"].isDouble()) {
        return false;
    }
    if (config.contains("Legend") && !config["Legend"].isBool()) {
        return false;
    }

    return true;
}

QJsonObject ChartConfiguration::mergeConfigs(const QJsonObject& base, const QJsonObject& overlay)
{
    return ChartTools::MergeJsonObject(base, overlay);
}

void ChartConfiguration::applyPendingConfig()
{
    if (m_pendingConfig.isEmpty()) {
        return;
    }

    if (validateConfig(m_pendingConfig)) {
        m_currentConfig = mergeConfigs(m_currentConfig, m_pendingConfig);
        emit configurationChanged(m_currentConfig);
#ifdef DEBUG_ON
        qDebug() << "ChartConfiguration: Pending config applied";
#endif
    }

    m_pendingConfig = QJsonObject();
}

bool ChartConfiguration::isConfigDifferent(const QJsonObject& config1, const QJsonObject& config2) const
{
    // Simple comparison - in practice, might need more sophisticated comparison
    QJsonDocument doc1(config1);
    QJsonDocument doc2(config2);
    return doc1.toJson() != doc2.toJson();
}