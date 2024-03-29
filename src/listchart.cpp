/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 */

#include <QtCore/QDebug>
#include <QtCore/QPointer>

#include <QtCharts/QBoxPlotSeries>

#include <QtWidgets/QColorDialog>
#include <QtWidgets/QLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QWidget>

#include "chartview.h"
#include "series.h"

#include "listchart.h"

ListChart::ListChart()
{
    m_chartview = new ChartView;
    connect(m_chartview, &ChartView::LastDirChanged, this, &ListChart::LastDirChanged);
    connect(m_chartview, &ChartView::ConfigurationChanged, this, &ListChart::ConfigurationChanged);
    connect(m_chartview, &ChartView::setUpFinished, this, &ListChart::setUpFinished);
    connect(m_chartview, &ChartView::ExportSettingsFileAdded, this, &ListChart::ExportSettingsFileAdded);

    m_chartview->setYAxis("Y");
    m_chartview->setXAxis("X");

    m_list = new QListWidget;
    m_list->setMaximumWidth(200);

    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_list, &QListWidget::customContextMenuRequested, this, &ListChart::ContextMenu);

    m_names_list = new QListWidget;
    m_names_list->setMaximumWidth(200);

    QSplitter* list_splitter = new QSplitter(Qt::Vertical);
    list_splitter->addWidget(m_list);
    list_splitter->addWidget(m_names_list);
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(m_chartview);
    splitter->addWidget(list_splitter);
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(splitter);
    setLayout(layout);

    connect(m_list, &QListWidget::itemDoubleClicked, this, &ListChart::SeriesListClicked);
    connect(m_names_list, &QListWidget::itemDoubleClicked, this, &ListChart::NamesListClicked);
}

ListChart::~ListChart()
{
}

void ListChart::setXAxis(const QString& str)
{
    m_chartview->setXAxis(str);
}

void ListChart::setYAxis(const QString& str)
{
    m_chartview->setYAxis(str);
}

void ListChart::addSeries(QAbstractSeries* series, int index, const QColor& color, QString name, bool callout)
{
    if (name.isEmpty() || name.isNull())
        name = series->name();

    m_chartview->addSeries(series, callout);

    QListWidgetItem* item = NULL;
    if (index >= m_list->count()) {
        item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, index);
        if (qobject_cast<QXYSeries*>(series))
            item->setBackground(qobject_cast<QXYSeries*>(series)->color());
        else
            item->setBackground(color);
        m_list->addItem(item);
        item->setData(Qt::UserRole + 1, QVariant::fromValue(series));
    }

    QXYSeries* s = qobject_cast<QXYSeries*>(series);
    if (s && item != NULL)
        connect(s, &QXYSeries::colorChanged, this, [item](const QColor& color) {
            item->setBackground(color);
        });

    if (!m_names_list->findItems(name, Qt::MatchExactly).size()) {
        item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, name);
        m_names_list->addItem(item);
    }

    m_list->setItemDelegate(new HTMLListItem(m_list));
    m_names_list->setItemDelegate(new HTMLListItem(m_names_list));
    m_hidden[index] = true;
    m_series.insert(index, series);
    m_chartview->formatAxis();
    if (m_list->count() == m_names_list->count())
        m_names_list->hide();
    else
        m_names_list->show();
}

QLineSeries* ListChart::addLinearSeries(qreal m, qreal n, qreal min, qreal max, int index)
{
    QLineSeries* serie = m_chartview->addLinearSeries(m, n, min, max);
    m_series.insert(index, serie);
    return serie;
}

void ListChart::setColor(int index, const QColor& color)
{
    if (index < m_list->count())
        m_list->item(index)->setBackground(color);
}

void ListChart::Clear()
{
    m_chartview->ClearChart();
    m_series.clear();
    m_list->clear();
    m_names_list->clear();
}

void ListChart::NamesListClicked(QListWidgetItem* item)
{
    QString str = item->data(Qt::UserRole).toString();
    QList<QListWidgetItem*> list = m_list->findItems(str, Qt::MatchExactly);
    for (int i = 0; i < list.size(); ++i)
        SeriesListClicked(list[i]);
}

void ListChart::HideSeries(int index)
{
    m_hidden[index] = !m_hidden[index];
    QList<QAbstractSeries*> series = m_series.values(index);
    for (int j = 0; j < series.size(); ++j) {
        if (qobject_cast<BoxPlotSeries*>(series[j])) // visibility doesnt work for boxplots ??
            qobject_cast<BoxPlotSeries*>(series[j])->setVisible(m_hidden[index]);
        else
            series[j]->setVisible(m_hidden[index]);
    }
}

void ListChart::ContextMenu(const QPoint& pos)
{
    // Handle global position
    QPoint globalPos = m_list->mapToGlobal(pos);

    QListWidgetItem* item = m_list->item(m_list->currentRow());

    // Create menu and insert some actions
    QMenu myMenu;
    if (item->flags().testFlag(Qt::ItemIsEditable) == false)
        myMenu.addAction("Rename", this, SLOT(RenameSeries()));
    else
        myMenu.addAction("Save", this, SLOT(RenameSeries()));

    myMenu.addAction("Change Color", this, SLOT(ChangeColor()));

    // Show context menu at handling position
    myMenu.exec(globalPos);
}

void ListChart::RenameSeries()
{
    QListWidgetItem* item = m_list->item(m_list->currentRow());
    QAbstractSeries* series = item->data(Qt::UserRole + 1).value<QAbstractSeries*>();
    if (item->flags().testFlag(Qt::ItemIsEditable) == false) {
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_list->editItem(item);
    } else {
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        m_list->editItem(item);
        if (qobject_cast<QXYSeries*>(series)) {
            qobject_cast<QXYSeries*>(series)->setName(item->text());
        }
    }
}

void ListChart::ChangeColor()
{
    QListWidgetItem* item = m_list->item(m_list->currentRow());
    QAbstractSeries* series = item->data(Qt::UserRole + 1).value<QAbstractSeries*>();

    QColor color = QColorDialog::getColor(tr("Choose Color for Series"));

    if (qobject_cast<QXYSeries*>(series)) {
        item->setBackground(color);
        qobject_cast<QXYSeries*>(series)->setColor(color);
    }
    if (qobject_cast<QScatterSeries*>(series)) {
        qobject_cast<QScatterSeries*>(series)->setBorderColor(color);
    }
}

#include "listchart.moc"
