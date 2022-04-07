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

#include <QtCore/QtMath>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFontDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>

#include "axisconfig.h"

AxisConfig::AxisConfig(const QString& name, QGroupBox* parent)
    : QGroupBox{ parent }
{
    setTitle(name);
    setUi();
}

void AxisConfig::setUi()
{
    QVBoxLayout* mainlayout = new QVBoxLayout;
    QHBoxLayout* layout = new QHBoxLayout;
    m_title = new QLineEdit;
    connect(m_title, &QLineEdit::textChanged, this, &AxisConfig::ConfigChanged);
    layout->addWidget(new QLabel(tr("<h4>Axis Title</ h4>")));
    layout->addWidget(m_title);
    mainlayout->addLayout(layout);

    layout = new QHBoxLayout;
    layout->addWidget(new QLabel(tr("Axis Begin:")));
    m_min = new QDoubleSpinBox;
    m_min->setMaximum(1e23);
    m_min->setMinimum(-1e23);
    m_min->setDecimals(6);
    connect(m_min, &QDoubleSpinBox::textChanged, this, &AxisConfig::ConfigChanged);

    layout->addWidget(m_min);
    mainlayout->addLayout(layout);

    layout->addWidget(new QLabel(tr("Axis End:")));
    m_max = new QDoubleSpinBox;
    m_max->setMaximum(1e23);
    m_max->setMinimum(-1e23);
    m_max->setDecimals(6);
    connect(m_max, &QDoubleSpinBox::textChanged, this, &AxisConfig::ConfigChanged);

    layout->addWidget(m_max);

    m_dynamic = new QRadioButton(tr("Dynamic"));
    m_fixed = new QRadioButton(tr("Fixed"));
    layout->addWidget(new QLabel(tr("Ticks are:")));
    layout->addWidget(m_dynamic);
    layout->addWidget(m_fixed);
    connect(m_dynamic, &QRadioButton::toggled, this, &AxisConfig::ConfigChanged);

    mainlayout->addLayout(layout);

    layout = new QHBoxLayout;
    m_format = new QComboBox;
    m_format->setEditable(true);
    m_format->addItems(m_number_format);
    connect(m_format, &QComboBox::currentTextChanged, this, &AxisConfig::ConfigChanged);

    layout->addWidget(new QLabel(tr("Tick Format")));
    layout->addWidget(m_format);

    m_visible = new QCheckBox(tr("Show Axis"));
    m_major_visible = new QCheckBox(tr("Show Major Grid"));
    m_minor_visible = new QCheckBox(tr("Show Minor Grid"));
    connect(m_visible, &QCheckBox::stateChanged, this, &AxisConfig::ConfigChanged);
    connect(m_major_visible, &QCheckBox::stateChanged, this, &AxisConfig::ConfigChanged);
    connect(m_minor_visible, &QCheckBox::stateChanged, this, &AxisConfig::ConfigChanged);

    layout->addWidget(m_visible);
    layout->addWidget(m_major_visible);
    layout->addWidget(m_minor_visible);

    m_interval = new QDoubleSpinBox;
    m_interval->setMinimum(-1e23);
    m_interval->setMaximum(1e23);
    m_interval->setDecimals(6);

    m_tickAnchor = new QDoubleSpinBox;
    m_tickAnchor->setMinimum(-1e23);
    m_tickAnchor->setMaximum(1e23);
    m_tickAnchor->setDecimals(6);

    m_major_tick_count = new QSpinBox;
    m_minor_tick_count = new QSpinBox;
    m_minor_tick_count->setValue(2);
    connect(m_interval, &QDoubleSpinBox::textChanged, this, &AxisConfig::ConfigChanged);
    connect(m_tickAnchor, &QDoubleSpinBox::textChanged, this, &AxisConfig::ConfigChanged);
    connect(m_major_tick_count, &QSpinBox::textChanged, this, &AxisConfig::ConfigChanged);
    connect(m_minor_tick_count, &QSpinBox::textChanged, this, &AxisConfig::ConfigChanged);

    layout->addWidget(new QLabel("Minor Ticks"));
    layout->addWidget(m_minor_tick_count);

    QWidget* fixed = new QWidget;
    QHBoxLayout* fixedLayout = new QHBoxLayout;
    fixedLayout->addWidget(new QLabel(tr("Anchor")));
    fixedLayout->addWidget(m_tickAnchor);
    fixedLayout->addWidget(new QLabel(tr("Interval")));
    fixedLayout->addWidget(m_interval);
    fixed->setLayout(fixedLayout);

    QWidget* dynamic = new QWidget;
    QHBoxLayout* dynamicLayout = new QHBoxLayout;
    dynamicLayout->addWidget(new QLabel(tr("Tick Count")));
    dynamicLayout->addWidget(m_major_tick_count);
    dynamic->setLayout(dynamicLayout);

    QStackedWidget* stack = new QStackedWidget;
    stack->addWidget(fixed);
    stack->addWidget(dynamic);
    layout->addWidget(stack);

    connect(m_dynamic, &QRadioButton::toggled, this, [this, stack]() {
        stack->setCurrentIndex(m_dynamic->isChecked());
    });

    mainlayout->addLayout(layout);

    m_title_font = new QPushButton(tr("Title Font"));
    connect(m_title_font, &QPushButton::clicked, this, &AxisConfig::ChooseTitleFont);

    m_ticks_font = new QPushButton(tr("Ticks Font"));
    connect(m_ticks_font, &QPushButton::clicked, this, &AxisConfig::ChooseTicksFont);

    layout = new QHBoxLayout;
    layout->addWidget(m_title_font);
    layout->addWidget(m_ticks_font);
    mainlayout->addLayout(layout);

    setLayout(mainlayout);

    connect(m_minor_visible, &QCheckBox::stateChanged, this, [this](int state) {
        m_minor_tick_count->setEnabled(state);
    });
}

void AxisConfig::setConfig(const QJsonObject& config)
{
    m_title->setText(config["Title"].toString());
    m_min->setValue(config["Min"].toDouble());
    m_max->setValue(config["Max"].toDouble());
    m_format->setCurrentText(config["TickFormat"].toString().isEmpty() ? m_number_format[0] : config["TickFormat"].toString());
    m_visible->setChecked(config["showAxis"].toBool());
    m_minor_visible->setChecked(config["MinorVisible"].toBool());
    m_major_visible->setChecked(config["MajorVisible"].toBool());
    m_dynamic->setChecked(config["TickType"].toInt() == 1);
    m_tickAnchor->setValue(config["TickAnchor"].toDouble());
    if (config["TickType"].toInt() == 1)
        m_interval->setValue(qPow(10, int(log10(m_max->value() - m_min->value())) - 1));
    else
        m_interval->setValue(config["TickInterval"].toDouble());
    m_major_tick_count->setValue(config["TickCount"].toDouble());
    m_minor_tick_count->setValue(config["MinorTickCount"].toDouble());
    m_title_font_object.fromString(config["TitleFont"].toString());
    m_ticks_font_object.fromString(config["TicksFont"].toString());
}

QJsonObject AxisConfig::getConfig() const
{
    QJsonObject config;
    config["Title"] = m_title->text();
    config["Min"] = m_min->value();
    config["Max"] = m_max->value();
    config["TickFormat"] = m_format->currentText();
    config["showAxis"] = m_visible->isChecked();
    config["MinorVisible"] = m_minor_visible->isChecked();
    config["MajorVisible"] = m_major_visible->isChecked();
    config["TickType"] = m_dynamic->isChecked() == true ? 1 : 0;
    config["TickAnchor"] = m_tickAnchor->value();
    config["TickInterval"] = m_interval->value();
    config["TickCount"] = m_major_tick_count->value();
    config["MinorTickCount"] = m_minor_tick_count->value();
    config["TitleFont"] = m_title_font_object.toString();
    config["TicksFont"] = m_ticks_font_object.toString();
    return config;
}

void AxisConfig::ChooseTicksFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_ticks_font_object, this);
    if (ok) {
        m_ticks_font_object = font;
        emit ConfigChanged();
    }
}

void AxisConfig::ChooseTitleFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_title_font_object, this);
    if (ok) {
        m_title_font_object = font;
        emit ConfigChanged();
    }
}
