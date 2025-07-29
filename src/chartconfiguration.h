/*
 * CuteCharts - Configuration management component
 * Copyright (C) 2023 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QString>

#include <memory>

class QAbstractAxis;

/**
 * @brief Manages chart configuration settings and persistence
 *
 * Extracted from ChartView to handle all configuration-related operations:
 * - JSON configuration loading/saving
 * - Font configuration management
 * - Export settings management
 * - Configuration validation and merging
 */
class ChartConfiguration : public QObject {
    Q_OBJECT

public:
    explicit ChartConfiguration(QObject* parent = nullptr);
    ~ChartConfiguration() override = default;

    /**
     * @brief Get current chart configuration
     * @return Current configuration as JSON object
     */
    QJsonObject currentConfig() const { return m_currentConfig; }

    /**
     * @brief Get current font configuration
     * @return Current font settings as JSON object
     */
    QJsonObject currentFontConfig() const;

    /**
     * @brief Load configuration from JSON object
     * @param config Configuration to load
     * @param force Whether to force loading even if validation fails
     */
    void loadConfig(const QJsonObject& config, bool force = false);

    /**
     * @brief Update specific configuration values
     * @param config Partial configuration with changes
     * @param force Whether to force update
     */
    void updateConfig(const QJsonObject& config, bool force = false);

    /**
     * @brief Force configuration update without validation
     * @param config Configuration to apply
     */
    void forceConfig(const QJsonObject& config);

    /**
     * @brief Apply font configuration
     * @param fontConfig Font settings to apply
     */
    void setFontConfig(const QJsonObject& fontConfig);

    /**
     * @brief Save current font configuration to settings
     */
    void saveFontConfig();

    /**
     * @brief Load font configuration from settings
     */
    void loadFontConfig();

    /**
     * @brief Add export setting preset
     * @param name Name of the preset
     * @param description Description of the preset
     * @param settings Export settings as JSON
     */
    void addExportSetting(const QString& name, const QString& description, const QJsonObject& settings);

    /**
     * @brief Get export settings by name
     * @param name Name of the export preset
     * @return Export settings or empty object if not found
     */
    QJsonObject getExportSetting(const QString& name) const;

    /**
     * @brief Get all available export setting names
     * @return List of export preset names
     */
    QStringList getExportSettingNames() const;

    /**
     * @brief Update axis configuration
     * @param config Axis configuration
     * @param axis Target axis to configure
     */
    void updateAxisConfig(const QJsonObject& config, QAbstractAxis* axis);

    /**
     * @brief Get axis configuration from axis object
     * @param axis Source axis
     * @return Axis configuration as JSON
     */
    QJsonObject getAxisConfig(const QAbstractAxis* axis) const;

    /**
     * @brief Validate configuration object
     * @param config Configuration to validate
     * @return True if configuration is valid
     */
    bool validateConfig(const QJsonObject& config) const;

    /**
     * @brief Merge configuration objects
     * @param base Base configuration
     * @param overlay Configuration to overlay
     * @return Merged configuration
     */
    static QJsonObject mergeConfigs(const QJsonObject& base, const QJsonObject& overlay);

    /**
     * @brief Get default configuration
     * @return Default configuration object
     */
    static QJsonObject getDefaultConfig();

signals:
    /**
     * @brief Emitted when configuration changes
     * @param config New configuration
     */
    void configurationChanged(const QJsonObject& config);

    /**
     * @brief Emitted when font configuration changes
     * @param fontConfig New font configuration
     */
    void fontConfigurationChanged(const QJsonObject& fontConfig);

    /**
     * @brief Emitted when export setting is added
     * @param name Name of the setting
     * @param description Description of the setting
     * @param settings Settings data
     */
    void exportSettingAdded(const QString& name, const QString& description, const QJsonObject& settings);

private:
    QJsonObject m_currentConfig;
    QJsonObject m_pendingConfig;
    QJsonObject m_lastConfig;
    QJsonObject m_currentFontConfig;

    // Export settings storage: name -> (description, settings)
    QHash<QString, QPair<QString, QJsonObject>> m_exportSettings;

    QString m_font;

    void applyPendingConfig();
    bool isConfigDifferent(const QJsonObject& config1, const QJsonObject& config2) const;
};