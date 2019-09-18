/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#pragma once

#include "chartview.h"

#include <QtCharts/QAbstractSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>

#include <QtCore/QModelIndex>

#include <QtGui/QTextDocument>

#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QStyleOptionViewItem>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QWidget>

class QListWidget;
class QListWidgetItem;

/**
 * @todo write docs
 */

class HTMLListItem : public QStyledItemDelegate {
public:
    HTMLListItem(QObject* parent = 0)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);

        painter->save();

        QTextDocument doc;
        doc.setHtml(options.text);

        options.text = "";
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);

        painter->translate(options.rect.left(), options.rect.top());
        QRect clip(0, 0, options.rect.width(), options.rect.height());
        doc.drawContents(painter, clip);

        painter->restore();
    }
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        return QSize(150, QStyledItemDelegate::sizeHint(option, index).height());
    }
};

class ListChart : public QWidget {
    Q_OBJECT

public:
    ListChart();
    ~ListChart();
    void setXAxis(const QString& str);
    void setYAxis(const QString& str);
    inline void setTitle(const QString& str)
    {
        m_name = str;
        m_chartview->setTitle(str);
    }
    inline qreal YMax() const { return m_chartview->YMax(); }

    void addSeries(QtCharts::QAbstractSeries* series, int index, const QColor& color, QString name = QString(), bool callout = false);
    void Clear();
    QtCharts::QLineSeries* addLinearSeries(qreal m, qreal n, qreal min, qreal max, int index);

    inline void setAnimationOptions(QtCharts::QChart::AnimationOption option)
    {
        m_chartview->Chart()->setAnimationOptions(option);
    }

    inline void setTheme(QtCharts::QChart::ChartTheme theme)
    {
        m_chartview->Chart()->setTheme(theme);
    }

    inline void setName(const QString& name)
    {
        m_chartview->setName(name);
    }

    inline ChartView* Chart()
    {
        return m_chartview;
    }

    inline void setAutoScaleStrategy(AutoScaleStrategy strategy) { m_chartview->setAutoScaleStrategy(strategy); }

public slots:
    inline void formatAxis() { m_chartview->formatAxis(); }
    void setColor(int index, const QColor& color);
    void HideSeries(int index);
    void ApplyConfigurationChange();

private:
    QListWidget *m_list, *m_names_list;
    ChartView* m_chartview;
    //QtCharts::QChart* m_chart;
    QMultiHash<int, QtCharts::QAbstractSeries*> m_series;
    QHash<int, bool> m_hidden;
    QString m_name;

private slots:
    inline void SeriesListClicked(QListWidgetItem* item) { HideSeries(item->data(Qt::UserRole).toInt()); }
    void NamesListClicked(QListWidgetItem* item);
    void ContextMenu(const QPoint& pos);
    void RenameSeries();
    void ChangeColor();

signals:
    void itemDoubleClicked(QListWidgetItem* item);
    void LastDirChanged(const QString& dir);

    //     void itemDoubleClicked(QListWidgetItem *item);
};
