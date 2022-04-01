/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtGlobal>

#include <QAction>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFontDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

#include <QtCharts>

#include <QtCharts/QChartView>

#include <iostream>

#include "src/configwidgets/axisconfig.h"

#include "chartconfig.h"

ChartConfigDialog::ChartConfigDialog(QWidget* widget)
    : QDialog(widget)
{
    setModal(false);
    QGridLayout* layout = new QGridLayout;

    m_title = new QLineEdit;
    connect(m_title, &QLineEdit::textChanged, this, &ChartConfigDialog::Changed);
    m_title->setMinimumWidth(400);
    m_titlefont = new QPushButton(tr("Font"));
    connect(m_titlefont, &QPushButton::clicked, this, &ChartConfigDialog::setTitleFont);
    m_titlefont->setMaximumSize(60, 30);
    m_titlefont->setStyleSheet("background-color: #F3ECE0;");

    m_scaleaxis = new QPushButton(tr("Reset Scaling"));
    connect(m_scaleaxis, SIGNAL(clicked()), this, SIGNAL(ScaleAxis()));
    m_scaleaxis->setMaximumSize(100, 30);
    m_scaleaxis->setStyleSheet("background-color: #F3ECE0;");

    m_resetFontConfig = new QPushButton(tr("Reset Font Config"));
    m_resetFontConfig->setStyleSheet("background-color: #F3ECE0;");
    connect(m_resetFontConfig, &QPushButton::clicked, m_resetFontConfig, [this]() {
        QMessageBox::StandardButton replay;
        QString message;
#ifdef noto_font
        message = "Fonts will be set Google Noto Font!";
#else
        message = "Fonts will be set to your systems standard font configuration!";
#endif
        replay = QMessageBox::information(this, tr("Reset Font Config."), tr("Do you really want to reset the current font config?\n%1").arg(message), QMessageBox::Yes | QMessageBox::No);
        if (replay == QMessageBox::Yes) {
            emit this->ResetFontConfig();
        }
    });

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &ChartConfigDialog::Changed);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    m_legend = new QCheckBox(tr("Show Legend"));
    connect(m_legend, &QCheckBox::stateChanged, this, &ChartConfigDialog::Changed);

    m_lock_scaling = new QCheckBox(tr("Lock Scaling"));
    connect(m_lock_scaling, &QCheckBox::stateChanged, this, &ChartConfigDialog::Changed);

    m_annotation = new QCheckBox(tr("In Chart Annotation"));
    m_annotation->setChecked(true);
    connect(m_annotation, &QCheckBox::stateChanged, this, &ChartConfigDialog::Changed);

    m_theme = new QComboBox;
    m_theme->addItem("Light", QChart::ChartThemeLight);
    m_theme->addItem("Blue Cerulean", QChart::ChartThemeBlueCerulean);
    m_theme->addItem("Dark", QChart::ChartThemeDark);
    m_theme->addItem("Brown Sand", QChart::ChartThemeBrownSand);
    m_theme->addItem("Blue NCS", QChart::ChartThemeBlueNcs);
    m_theme->addItem("High Contrast", QChart::ChartThemeHighContrast);
    m_theme->addItem("Blue Icy", QChart::ChartThemeBlueIcy);
    m_theme->addItem("Qt", QChart::ChartThemeQt);
    m_theme->addItem("Black 'n' White", 8);
    connect(m_theme, qOverload<int>(&QComboBox::currentIndexChanged), this, &ChartConfigDialog::Changed);

    layout->addWidget(new QLabel(tr("<h4>Chart Title</h4>")), 0, 0);
    layout->addWidget(m_title, 0, 1);
    layout->addWidget(m_titlefont, 0, 2);

    m_x_config = new AxisConfig(tr("X Axis"));
    layout->addWidget(m_x_config, 1, 0, 1, 3);
    connect(m_x_config, &AxisConfig::ConfigChanged, this, &ChartConfigDialog::Changed);
    m_y_config = new AxisConfig(tr("Y Axis"));
    layout->addWidget(m_y_config, 2, 0, 1, 3);
    connect(m_y_config, &AxisConfig::ConfigChanged, this, &ChartConfigDialog::Changed);

    layout->addWidget(m_scaleaxis, 3, 1);
    layout->addWidget(m_lock_scaling, 3, 2);
    m_keys = new QPushButton(tr("Legend Font"));
    connect(m_keys, &QPushButton::clicked, this, &ChartConfigDialog::setKeysFont);
    m_keys->setMaximumSize(90, 30);
    m_keys->setStyleSheet("background-color: #F3ECE0;");

    m_alignment = new QPushButton(tr("Align Legend"));
    m_alignment->setMaximumSize(130, 30);
    m_alignment->setStyleSheet("background-color: #F3ECE0;");

    m_show_axis = new QCheckBox(tr("Show Axis"));
    m_show_axis->setChecked(true);

    QMenu* align = new QMenu(this);
    QAction* left = new QAction(tr("Left"), this);
    left->setData(Qt::AlignLeft);
    align->addAction(left);

    QAction* right = new QAction(tr("Right"), this);
    right->setData(Qt::AlignRight);
    align->addAction(right);

    QAction* top = new QAction(tr("Top"), this);
    top->setData(Qt::AlignTop);
    align->addAction(top);

    QAction* bottom = new QAction(tr("Bottom"), this);
    bottom->setData(Qt::AlignBottom);
    align->addAction(bottom);
    m_alignment->setMenu(align);

    connect(align, &QMenu::triggered, [this](QAction* action) {
        // m_chartconfig.align = Qt::Alignment(action->data().toInt());
        m_chart_config["Alignment"] = action->data().toInt();
        emit ConfigChanged(ChartConfigJson());
    });

    QHBoxLayout* actions = new QHBoxLayout;

    actions->addStretch(300);
    actions->addWidget(m_keys);
    actions->addWidget(m_alignment);
    actions->addWidget(m_legend);
    actions->addWidget(m_annotation);
    actions->addWidget(m_show_axis);
    layout->addLayout(actions, 4, 0, 1, 3);

    m_x_size = new QSpinBox;
    m_x_size->setRange(0, 1e6);
    m_x_size->setValue(qApp->instance()->property("xSize").toInt());

    m_y_size = new QSpinBox;
    m_y_size->setRange(0, 1e6);
    m_y_size->setValue(qApp->instance()->property("ySize").toInt());

    m_scaling = new QSpinBox;
    m_scaling->setRange(0, 1e6);
    m_scaling->setValue(qApp->instance()->property("chartScaling").toInt());

    m_markerSize = new QDoubleSpinBox;
    m_markerSize->setRange(0, 30);
    m_markerSize->setValue(qApp->instance()->property("markerSize").toDouble());

    m_lineWidth = new QDoubleSpinBox;
    m_lineWidth->setRange(0, 30);
    m_lineWidth->setValue(qApp->instance()->property("lineWidth").toDouble());

    actions = new QHBoxLayout;
    actions->addWidget(new QLabel(tr("X Size:")));
    actions->addWidget(m_x_size);

    actions->addWidget(new QLabel(tr("Y Size:")));
    actions->addWidget(m_y_size);

    actions->addWidget(new QLabel(tr("Scaling:")));
    actions->addWidget(m_scaling);

    actions->addWidget(new QLabel(tr("Marker Size:")));
    actions->addWidget(m_markerSize);

    actions->addWidget(new QLabel(tr("Line Width:")));
    actions->addWidget(m_lineWidth);

    layout->addLayout(actions, 5, 0, 1, 3);

    layout->addWidget(m_resetFontConfig, 6, 0);
    layout->addWidget(m_theme, 6, 1);

    layout->addWidget(m_buttons, 6, 2, 1, 1);
    setLayout(layout);
    setWindowTitle("Configure charts ...");
}

ChartConfigDialog::~ChartConfigDialog()
{
}

void ChartConfigDialog::setChartConfig(const QJsonObject& chartconfig)
{
    const QSignalBlocker blocker(this);
    QJsonObject axis = chartconfig["xAxis"].toObject();

    m_x_config->setConfig(chartconfig["xAxis"].toObject());
    m_y_config->setConfig(chartconfig["yAxis"].toObject());

    m_legend->setChecked(chartconfig["Legend"].toBool());
    m_lock_scaling->setChecked(chartconfig["ScalingLocked"].toBool());
    m_annotation->setChecked(chartconfig["Annotation"].toBool());

    m_theme->setCurrentIndex(chartconfig["Theme"].toInt());

    m_title->setText(chartconfig["Title"].toString());

    m_x_size->setValue(chartconfig["xSize"].toDouble());
    m_y_size->setValue(chartconfig["ySize"].toDouble());
    m_scaling->setValue(chartconfig["Scaling"].toDouble());
    m_lineWidth->setValue(chartconfig["lineWidth"].toDouble());
    m_markerSize->setValue(chartconfig["markerSize"].toDouble());
    m_show_axis->setChecked(chartconfig["showAxis"].toDouble());

    m_x_size->setValue(chartconfig["xSize"].toInt());
    m_y_size->setValue(chartconfig["ySize"].toInt());

    m_scaling->setValue(chartconfig["Scaling"].toInt());
    m_markerSize->setValue(chartconfig["markerSize"].toDouble());
    m_lineWidth->setValue(chartconfig["lineWidth"].toDouble());

    m_chart_config = chartconfig;
}

void ChartConfigDialog::Changed()
{
    m_chart_config["Title"] = m_title->text();
    m_chart_config["xAxis"] = m_x_config->getConfig();
    ;
    m_chart_config["yAxis"] = m_y_config->getConfig();
    ;
    m_chart_config["Legend"] = m_legend->isChecked();
    m_chart_config["ScalingLocked"] = m_lock_scaling->isChecked();
    m_chart_config["Annotation"] = m_annotation->isChecked();
    m_chart_config["xSize"] = m_x_size->value();
    m_chart_config["ySize"] = m_y_size->value();
    m_chart_config["Scaling"] = m_scaling->value();
    m_chart_config["lineWidth"] = m_lineWidth->value();
    m_chart_config["markerSize"] = m_markerSize->value();
    m_chart_config["Theme"] = m_theme->currentIndex();

    emit ConfigChanged(ChartConfigJson());
}

void ChartConfigDialog::setKeysFont()
{
    bool ok;
    QFont tmp;
    tmp.fromString(m_chart_config["KeyFont"].toString());
    QFont font = QFontDialog::getFont(&ok, tmp, this);
    if (ok) {
        m_chart_config["KeyFont"] = font.toString();
        emit ConfigChanged(ChartConfigJson());
    }
}

void ChartConfigDialog::setTitleFont()
{
    bool ok;
    QFont tmp;
    tmp.fromString(m_chart_config["TitleFont"].toString());
    QFont font = QFontDialog::getFont(&ok, tmp, this);
    std::cout << font.toString().toStdString() << std::endl;
    if (ok) {
        m_chart_config["TitleFont"] = font.toString();

        emit ConfigChanged(ChartConfigJson());
    }
}
#include "chartconfig.moc"
