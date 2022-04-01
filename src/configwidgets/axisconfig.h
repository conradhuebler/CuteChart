/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QWidget>

class QLineEdit;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QRadioButton;
class QPushButton;

class AxisConfig : public QGroupBox {
    Q_OBJECT
public:
    explicit AxisConfig(const QString& name, QGroupBox* parent = nullptr);
    void setConfig(const QJsonObject& config);
    QJsonObject getConfig() const;

private:
    void setUi();
    void ChooseTitleFont();
    void ChooseTicksFont();

    QPushButton *m_title_font, *m_ticks_font;
    QLineEdit* m_title;
    QDoubleSpinBox *m_min, *m_max, *m_interval, *m_tickAnchor;
    QComboBox* m_format;
    QRadioButton *m_dynamic, *m_fixed;
    QSpinBox *m_major_tick_count, *m_minor_tick_count;
    QCheckBox *m_major_visible, *m_minor_visible, *m_visible;

    QStringList m_number_format = { "%2.2f", "%2.5f", "%2.2E", "%2.5E" };
    QFont m_title_font_object, m_ticks_font_object;
signals:
    void ConfigChanged();
};
