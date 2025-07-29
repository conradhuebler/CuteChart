/*
 * CuteCharts - Export functionality component
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
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <QtGui/QPixmap>

class QChart;
class QWidget;
class SeriesState;
class QAbstractSeries;

/**
 * @brief Handles chart export operations
 *
 * Extracted from ChartView to handle all export-related operations:
 * - PNG export with various settings
 * - Series state management during export
 * - Export presets and configurations
 * - Image processing (cropping, transparency)
 */
class ChartExporter : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Export format options
     */
    enum class ExportFormat {
        PNG,
        SVG,
        PDF
    };

    /**
     * @brief Export quality settings
     */
    struct ExportSettings {
        int width = 600;
        int height = 400;
        int scaling = 2;
        bool cropImage = true;
        bool transparentBackground = true;
        ExportFormat format = ExportFormat::PNG;
        QString fileName;

        // Series appearance overrides for export
        QJsonObject seriesOverrides;
    };

    explicit ChartExporter(QChart* chart, QWidget* parentWidget, QObject* parent = nullptr);
    ~ChartExporter() override = default;

    /**
     * @brief Export chart to PNG with current settings
     * @param fileName Output file name (empty for dialog)
     * @return True if export was successful
     */
    bool exportToPNG(const QString& fileName = QString());

    /**
     * @brief Export chart with specific settings
     * @param settings Export configuration
     * @return True if export was successful
     */
    bool exportWithSettings(const ExportSettings& settings);

    /**
     * @brief Export chart using preset configuration
     * @param presetName Name of the export preset
     * @param fileName Output file name (empty for dialog)
     * @return True if export was successful
     */
    bool exportWithPreset(const QString& presetName, const QString& fileName = QString());

    /**
     * @brief Get current export settings
     * @return Current export configuration
     */
    ExportSettings getCurrentSettings() const { return m_currentSettings; }

    /**
     * @brief Set export settings
     * @param settings New export configuration
     */
    void setExportSettings(const ExportSettings& settings);

    /**
     * @brief Add export preset
     * @param name Preset name
     * @param description Preset description
     * @param settings Export settings for this preset
     */
    void addExportPreset(const QString& name, const QString& description, const ExportSettings& settings);

    /**
     * @brief Remove export preset
     * @param name Preset name to remove
     */
    void removeExportPreset(const QString& name);

    /**
     * @brief Get available export preset names
     * @return List of preset names
     */
    QStringList getExportPresetNames() const;

    /**
     * @brief Get export preset settings
     * @param name Preset name
     * @return Export settings for the preset, or default if not found
     */
    ExportSettings getExportPreset(const QString& name) const;

    /**
     * @brief Set last used directory for file dialogs
     * @param directory Directory path
     */
    void setLastDirectory(const QString& directory) { m_lastDirectory = directory; }

    /**
     * @brief Get last used directory
     * @return Last directory path
     */
    QString getLastDirectory() const { return m_lastDirectory; }

    /**
     * @brief Create pixmap from chart with given settings
     * @param settings Export settings to use
     * @return Generated pixmap
     */
    QPixmap createPixmap(const ExportSettings& settings) const;

public slots:
    /**
     * @brief Show export dialog and export chart
     */
    void showExportDialog();

signals:
    /**
     * @brief Emitted when export is completed
     * @param fileName Output file name
     * @param success Whether export was successful
     */
    void exportCompleted(const QString& fileName, bool success);

    /**
     * @brief Emitted when last directory changes
     * @param directory New directory path
     */
    void lastDirectoryChanged(const QString& directory);

    /**
     * @brief Emitted when export preset is added
     * @param name Preset name
     * @param description Preset description
     */
    void exportPresetAdded(const QString& name, const QString& description);

private:
    QChart* m_chart;
    QWidget* m_parentWidget;

    ExportSettings m_currentSettings;
    QString m_lastDirectory;
    QString m_lastFileName;

    // Export presets: name -> (description, settings)
    QHash<QString, QPair<QString, ExportSettings>> m_exportPresets;

    /**
     * @brief Save series states before applying export overrides
     * @return Map of series to their saved states
     */
    QHash<QAbstractSeries*, QSharedPointer<SeriesState>> saveSeriesStates() const;

    /**
     * @brief Restore series states after export
     * @param savedStates Hash of series to their saved states
     */
    void restoreSeriesStates(const QHash<QAbstractSeries*, QSharedPointer<SeriesState>>& savedStates) const;

    /**
     * @brief Apply export-specific series overrides
     * @param overrides JSON object with series appearance overrides
     */
    void applySeriesOverrides(const QJsonObject& overrides);

    /**
     * @brief Process exported image according to settings
     * @param pixmap Source pixmap
     * @param settings Export settings
     * @return Processed pixmap
     */
    QPixmap processImage(const QPixmap& pixmap, const ExportSettings& settings) const;

    /**
     * @brief Crop image to remove unnecessary whitespace
     * @param pixmap Source pixmap
     * @return Cropped pixmap
     */
    QPixmap cropImage(const QPixmap& pixmap) const;

    /**
     * @brief Make image background transparent
     * @param pixmap Source pixmap
     * @return Pixmap with transparent background
     */
    QPixmap makeTransparent(const QPixmap& pixmap) const;

    /**
     * @brief Get file name from dialog or use provided name
     * @param suggestedName Suggested file name
     * @param format Export format
     * @return Selected file name or empty if cancelled
     */
    QString getExportFileName(const QString& suggestedName, ExportFormat format) const;

    /**
     * @brief Convert ExportSettings to JSON
     * @param settings Settings to convert
     * @return JSON representation
     */
    static QJsonObject settingsToJson(const ExportSettings& settings);

    /**
     * @brief Convert JSON to ExportSettings
     * @param json JSON to convert
     * @return Export settings
     */
    static ExportSettings settingsFromJson(const QJsonObject& json);
};