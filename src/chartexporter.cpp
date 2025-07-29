/*
 * CuteCharts - Export functionality component (Claude Generated)
 * Copyright (C) 2023 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "chartexporter.h"
#include "series.h"

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>

#include <QtGui/QCursor>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QScrollArea>

#ifdef DEBUG_ON
#include <QtCore/QDebug>
#endif

ChartExporter::ChartExporter(QChart* chart, QWidget* parentWidget, QObject* parent)
    : QObject(parent)
    , m_chart(chart)
    , m_parentWidget(parentWidget)
{
    // Initialize default export settings
    m_currentSettings.width = 600;
    m_currentSettings.height = 400;
    m_currentSettings.scaling = 2;
    m_currentSettings.cropImage = true;
    m_currentSettings.transparentBackground = true;
    m_currentSettings.format = ExportFormat::PNG;

#ifdef DEBUG_ON
    qDebug() << "ChartExporter: Initialized with default settings";
#endif
}

bool ChartExporter::exportToPNG(const QString& fileName)
{
    ExportSettings pngSettings = m_currentSettings;
    pngSettings.format = ExportFormat::PNG;
    pngSettings.fileName = fileName;

    return exportWithSettings(pngSettings);
}

bool ChartExporter::exportWithSettings(const ExportSettings& settings)
{
    if (!m_chart) {
#ifdef DEBUG_ON
        qDebug() << "ChartExporter: Cannot export - chart is null";
#endif
        return false;
    }

    QString fileName = settings.fileName;
    if (fileName.isEmpty()) {
        fileName = getExportFileName("chart", settings.format);
        if (fileName.isEmpty()) {
            return false; // User cancelled
        }
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    try {
        // Save series states before applying export overrides
        auto savedStates = saveSeriesStates();

        // Apply export-specific series overrides
        applySeriesOverrides(settings.seriesOverrides);

        // Create the pixmap
        QPixmap pixmap = createPixmap(settings);

        // Process the image
        pixmap = processImage(pixmap, settings);

        // Restore series states
        restoreSeriesStates(savedStates);

        // Save the image
        bool success = pixmap.save(fileName);

        if (success) {
            m_lastFileName = fileName;
            emit exportCompleted(fileName, true);
#ifdef DEBUG_ON
            qDebug() << "ChartExporter: Successfully exported to" << fileName;
#endif
        } else {
#ifdef DEBUG_ON
            qDebug() << "ChartExporter: Failed to save image to" << fileName;
#endif
        }

        QApplication::restoreOverrideCursor();
        return success;

    } catch (const std::exception& e) {
#ifdef DEBUG_ON
        qDebug() << "ChartExporter: Exception during export:" << e.what();
#endif
        QApplication::restoreOverrideCursor();
        emit exportCompleted(fileName, false);
        return false;
    }
}

bool ChartExporter::exportWithPreset(const QString& presetName, const QString& fileName)
{
    if (!m_exportPresets.contains(presetName)) {
#ifdef DEBUG_ON
        qDebug() << "ChartExporter: Preset not found:" << presetName;
#endif
        return false;
    }

    ExportSettings settings = m_exportPresets[presetName].second;
    settings.fileName = fileName;

    return exportWithSettings(settings);
}

void ChartExporter::setExportSettings(const ExportSettings& settings)
{
    m_currentSettings = settings;

#ifdef DEBUG_ON
    qDebug() << "ChartExporter: Settings updated" << settings.width << "x" << settings.height;
#endif
}

void ChartExporter::addExportPreset(const QString& name, const QString& description, const ExportSettings& settings)
{
    m_exportPresets[name] = qMakePair(description, settings);
    emit exportPresetAdded(name, description);

#ifdef DEBUG_ON
    qDebug() << "ChartExporter: Export preset added:" << name;
#endif
}

void ChartExporter::removeExportPreset(const QString& name)
{
    m_exportPresets.remove(name);

#ifdef DEBUG_ON
    qDebug() << "ChartExporter: Export preset removed:" << name;
#endif
}

QStringList ChartExporter::getExportPresetNames() const
{
    return m_exportPresets.keys();
}

ChartExporter::ExportSettings ChartExporter::getExportPreset(const QString& name) const
{
    if (m_exportPresets.contains(name)) {
        return m_exportPresets[name].second;
    }
    return m_currentSettings; // Return default if not found
}

QPixmap ChartExporter::createPixmap(const ExportSettings& settings) const
{
    if (!m_chart) {
        return QPixmap();
    }

    // Save current chart state
    QChart::AnimationOptions originalAnimation = m_chart->animationOptions();
    m_chart->setAnimationOptions(QChart::NoAnimation);

    // Prepare chart for rendering
    m_chart->scene()->update();
    QApplication::processEvents();

    // Create high-resolution image
    int w = settings.width;
    int h = settings.height;
    QImage image(QSize(settings.scaling * w, settings.scaling * h), QImage::Format_ARGB32);

    if (settings.transparentBackground) {
        image.fill(Qt::transparent);
    } else {
        image.fill(Qt::white);
    }

    // Setup painter with antialiasing
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Handle background transparency
    QBrush originalBrush = m_chart->backgroundBrush();
    if (settings.transparentBackground) {
        QBrush transparentBrush;
        transparentBrush.setColor(Qt::transparent);
        m_chart->setBackgroundBrush(transparentBrush);
    }

    // Render chart to image
    QRectF sourceRect = m_chart->rect();
    QRectF targetRect(0, 0, settings.scaling * w, settings.scaling * h);
    m_chart->scene()->render(&painter, targetRect, sourceRect);

    // Restore chart state
    m_chart->setBackgroundBrush(originalBrush);
    m_chart->setAnimationOptions(originalAnimation);

#ifdef DEBUG_ON
    qDebug() << "ChartExporter: Pixmap created" << w << "x" << h << "scaling:" << settings.scaling;
#endif

    return QPixmap::fromImage(image);
}

void ChartExporter::showExportDialog()
{
    const QString fileName = QFileDialog::getSaveFileName(
        m_parentWidget,
        tr("Save Chart"),
        m_lastDirectory + "/" + m_lastFileName,
        tr("PNG Images (*.png);;All Files (*)"));

    if (!fileName.isEmpty()) {
        exportToPNG(fileName);

        // Update last directory
        QFileInfo fileInfo(fileName);
        m_lastDirectory = fileInfo.absolutePath();
        emit lastDirectoryChanged(m_lastDirectory);
    }
}

QHash<QAbstractSeries*, QSharedPointer<SeriesState>> ChartExporter::saveSeriesStates() const
{
    QHash<QAbstractSeries*, QSharedPointer<SeriesState>> savedStates;

    if (!m_chart) {
        return savedStates;
    }

    for (QAbstractSeries* series : m_chart->series()) {
        auto uniqueState = SeriesStateFactory::createState(series);
        if (uniqueState) {
            // Convert unique_ptr to QSharedPointer for Qt-style usage
            QSharedPointer<SeriesState> state(uniqueState.release());
            state->saveState(series);
            savedStates[series] = state;
        }
    }

#ifdef DEBUG_ON
    qDebug() << "ChartExporter: Saved" << savedStates.size() << "series states";
#endif

    return savedStates;
}

void ChartExporter::restoreSeriesStates(const QHash<QAbstractSeries*, QSharedPointer<SeriesState>>& savedStates) const
{
    for (auto it = savedStates.begin(); it != savedStates.end(); ++it) {
        if (it.value()) {
            it.value()->restoreState(it.key());
        }
    }

#ifdef DEBUG_ON
    qDebug() << "ChartExporter: Restored" << savedStates.size() << "series states";
#endif
}

void ChartExporter::applySeriesOverrides(const QJsonObject& overrides)
{
    if (overrides.isEmpty() || !m_chart) {
        return;
    }

    // Apply global series appearance overrides for export
    qreal lineWidth = overrides.value("lineWidth").toDouble(2.0);
    qreal markerSize = overrides.value("markerSize").toDouble(8.0);

    for (QAbstractSeries* series : m_chart->series()) {
        // Disable OpenGL for export (ensures consistent rendering)
        series->setUseOpenGL(false);

        if (auto* lineSeries = qobject_cast<LineSeries*>(series)) {
            lineSeries->setLineWidth(lineWidth);
        } else if (auto* scatterSeries = qobject_cast<QScatterSeries*>(series)) {
            scatterSeries->setMarkerSize(markerSize);
            scatterSeries->setBorderColor(Qt::transparent);
        }
    }

#ifdef DEBUG_ON
    qDebug() << "ChartExporter: Series overrides applied";
#endif
}

QPixmap ChartExporter::processImage(const QPixmap& pixmap, const ExportSettings& settings) const
{
    QPixmap result = pixmap;

    if (settings.cropImage) {
        result = cropImage(result);
    }

    if (settings.transparentBackground) {
        result = makeTransparent(result);
    }

    return result;
}

QPixmap ChartExporter::cropImage(const QPixmap& pixmap) const
{
    QImage image = pixmap.toImage();
    QRect region = QRegion(QBitmap::fromImage(image.createMaskFromColor(0x00000000))).boundingRect();

    if (region.isValid() && !region.isEmpty()) {
        return QPixmap::fromImage(image.copy(region));
    }

    return pixmap;
}

QPixmap ChartExporter::makeTransparent(const QPixmap& pixmap) const
{
    // Image should already be transparent if transparentBackground was set
    // This method can be extended for additional transparency processing
    return pixmap;
}

QString ChartExporter::getExportFileName(const QString& suggestedName, ExportFormat format) const
{
    QString filter;
    QString defaultSuffix;

    switch (format) {
    case ExportFormat::PNG:
        filter = tr("PNG Images (*.png)");
        defaultSuffix = ".png";
        break;
    case ExportFormat::SVG:
        filter = tr("SVG Images (*.svg)");
        defaultSuffix = ".svg";
        break;
    case ExportFormat::PDF:
        filter = tr("PDF Documents (*.pdf)");
        defaultSuffix = ".pdf";
        break;
    }

    QString fileName = suggestedName;
    if (!fileName.endsWith(defaultSuffix)) {
        fileName += defaultSuffix;
    }

    return QFileDialog::getSaveFileName(
        m_parentWidget,
        tr("Export Chart"),
        m_lastDirectory + "/" + fileName,
        filter);
}

QJsonObject ChartExporter::settingsToJson(const ExportSettings& settings)
{
    QJsonObject json;
    json["width"] = settings.width;
    json["height"] = settings.height;
    json["scaling"] = settings.scaling;
    json["cropImage"] = settings.cropImage;
    json["transparentBackground"] = settings.transparentBackground;
    json["format"] = static_cast<int>(settings.format);
    json["seriesOverrides"] = settings.seriesOverrides;
    return json;
}

ChartExporter::ExportSettings ChartExporter::settingsFromJson(const QJsonObject& json)
{
    ExportSettings settings;
    settings.width = json.value("width").toInt(600);
    settings.height = json.value("height").toInt(400);
    settings.scaling = json.value("scaling").toInt(2);
    settings.cropImage = json.value("cropImage").toBool(true);
    settings.transparentBackground = json.value("transparentBackground").toBool(true);
    settings.format = static_cast<ExportFormat>(json.value("format").toInt(0));
    settings.seriesOverrides = json.value("seriesOverrides").toObject();
    return settings;
}