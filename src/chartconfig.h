/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2022  Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QJsonObject>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QLabel;
class QDoubleSpinBox;
class QSpinBox;
class QDialogButtonBox;

class AxisConfig;

class ChartConfigDialog : public QDialog {
    Q_OBJECT

public:
    ChartConfigDialog(QWidget* widget);
    ~ChartConfigDialog();
    void setChartConfig(const QJsonObject& chartconfig);
    inline QJsonObject ChartConfigJson() const { return m_chart_config; }
    QDialogButtonBox* m_buttons;

private:
    AxisConfig *m_x_config, *m_y_config;
    QPushButton *m_scaleaxis, *m_keys, *m_alignment, *m_titlefont, *m_resetFontConfig;
    QLineEdit* m_title;
    QDoubleSpinBox *m_markerSize, *m_lineWidth;
    QSpinBox *m_scaling, *m_x_size, *m_y_size;
    QCheckBox *m_legend, *m_lock_scaling, *m_annotation, *m_show_axis;
    QComboBox* m_theme;
    QJsonObject m_chart_config;

    QStringList m_number_format = { "%2.2f", "%2.5f" };
private slots:
    void Changed();
    void setKeysFont();
    void setTitleFont();

signals:
    void ConfigChanged(const QJsonObject& chartconfig);
    void ScaleAxis();
    void ResetFontConfig();
};
