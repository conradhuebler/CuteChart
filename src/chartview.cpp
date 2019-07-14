/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2016 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "chartconfig.h"
#include "peakcallout.h"
#include "series.h"
#include "tools.h"

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QXYSeries>

#include <QtCore/QBuffer>
#include <QtCore/QDebug>
#include <QtCore/QMimeData>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>

#include <QtGui/QCursor>
#include <QtGui/QDrag>

#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>

#include <cmath>
#include <iostream>

#include "chartview.h"

ChartViewPrivate::ChartViewPrivate(QtCharts::QChart* chart, QWidget* parent)
    : QtCharts::QChartView(parent)
    , m_vertical_line_visible(false)
    , m_zoom_strategy(Z_None)
    , m_select_strategy(S_None)
{
    setChart(chart);
    // setAcceptDrops(true);
    setRenderHint(QPainter::Antialiasing, true);
    setRubberBand(QChartView::NoRubberBand);
    /*QFont font = chart->titleFont();
    font.setBold(true);
    font.setPointSize(12);
    chart->setTitleFont(font);*/

    m_vertical_line = new QGraphicsLineItem(chart);
    QPen pen;
    pen.setWidth(1);
    pen.setColor(Qt::gray);
    m_vertical_line->setPen(pen);
    m_vertical_line->setLine(0, -1, 0, 10);
    m_vertical_line->show();

    m_line_position = new QGraphicsTextItem(chart);
    m_select_box = new QGraphicsRectItem(chart);
    m_select_box->setBrush(QColor::fromRgbF(0.68, 0.68, 0.67, 1));

    m_zoom_box = new QGraphicsRectItem(chart);
    m_zoom_box->setBrush(QColor::fromRgbF(0.18, 0.64, 0.71, 1));
    setMouseTracking(true);
}

void ChartViewPrivate::UpdateView(double min, double max)
{
    m_y_min = min;
    m_y_max = max;
}

QPointF ChartViewPrivate::mapToPoint(QMouseEvent* event) const
{
    return mapToPoint(QPointF(event->x(), event->y()));
}

QPointF ChartViewPrivate::mapToPoint(const QPointF& point) const
{
    QPointF inPoint(point);
    if ((CurrentZoomStrategy() == ZoomStrategy::Z_Horizontal && m_single_left_click) || (CurrentSelectStrategy() == SelectStrategy::S_Horizontal && m_single_right_click)) {
        if (m_zoom_pending == false || m_select_pending == false)
            inPoint.setY(m_upperleft.y());
        else
            inPoint.setY(m_lowerright.y());

    } else if ((CurrentZoomStrategy() == ZoomStrategy::Z_Vertical && m_single_left_click) || (CurrentSelectStrategy() == SelectStrategy::S_Vertical && m_single_right_click)) {
        if (m_zoom_pending == false || m_select_pending == false)
            inPoint.setX(m_upperleft.x());
        else
            inPoint.setX(m_lowerright.x());
    }
    qDebug() << point << inPoint << m_upperleft << m_lowerright;

    return inPoint;
}

void ChartViewPrivate::setZoom(qreal x_min, qreal x_max, qreal y_min, qreal y_max)
{
    QPointer<QtCharts::QValueAxis> yaxis = qobject_cast<QtCharts::QValueAxis*>(chart()->axes(Qt::Vertical).first());
    if (!yaxis)
        return;

    QPointer<QtCharts::QValueAxis> xaxis = qobject_cast<QtCharts::QValueAxis*>(chart()->axes(Qt::Horizontal).first());
    if (!xaxis)
        return;

    yaxis->setMin(y_min);
    yaxis->setMax(y_max);
    m_y_min = y_min;
    m_y_max = y_max;

    xaxis->setMin(x_min);
    xaxis->setMax(x_max);
    m_x_min = x_min;
    m_x_max = x_max;
}

void ChartViewPrivate::setVerticalLineEnabled(bool enabled)
{
    m_line_position->setVisible(enabled);
    m_vertical_line->setVisible(enabled);
    m_vertical_line_visible = enabled;
}

void ChartViewPrivate::RectanglStart(QMouseEvent* event)
{
    QPointF inPoint = (mapFromGlobal(QCursor::pos()));
    m_rect_start = inPoint;

    if (m_select_pending)
        m_select_box->setRect(m_rect_start.x(), m_rect_start.y(), 0, 0);
    else if (m_zoom_pending)
        m_zoom_box->setRect(m_rect_start.x(), m_rect_start.y(), 0, 0);

    m_vertical_line->hide();
    m_line_position->hide();
    m_zoom_pending = true;
}

QPair<QPointF, QPointF> ChartViewPrivate::getCurrentRectangle(QMouseEvent* event)
{
    QPointF inPoint = (mapFromGlobal(QCursor::pos()));

    QPointF left, right;
    left.setX(qMin(inPoint.x(), m_rect_start.x()));
    right.setX(qMax(inPoint.x(), m_rect_start.x()));

    left.setY(qMin(inPoint.y(), m_rect_start.y()));
    right.setY(qMax(inPoint.y(), m_rect_start.y()));

    //qDebug() << left << right << m_upperleft << m_lowerright;
    return QPair<QPointF, QPointF>(left, right);
}

QPointF ChartViewPrivate::currentMousePosition() const
{
    return (mapFromGlobal(QCursor::pos()));
}

void ChartViewPrivate::UpdateCorner()
{
    m_upperleft = chart()->mapToPosition(QPointF(m_x_min, m_y_max));
    m_lowerright = chart()->mapToPosition(QPointF(m_x_max, m_y_min));
    // qDebug() << QPointF(m_x_min, m_y_max) << m_upperleft;
    // qDebug() << QPointF(m_x_max, m_y_min) << m_lowerright;
}

void ChartViewPrivate::mousePressEvent(QMouseEvent* event)
{
    UpdateCorner();

    if (event->button() == Qt::RightButton || event->buttons() == Qt::RightButton) {
        if (CurrentSelectStrategy() != SelectStrategy::S_None) {
            if (!m_select_pending) {
                m_single_right_click = true;
                m_select_pending = true;
                m_select_box->show();
                RectanglStart(event);
            }
        }
    } else if (event->button() == Qt::LeftButton || event->buttons() == Qt::LeftButton) {
        if (CurrentZoomStrategy() != ZoomStrategy::Z_None) {
            m_single_left_click = true;
            m_zoom_pending = true;
            m_zoom_box->show();
            RectanglStart(event);
        }

    } else if (event->button() == Qt::MiddleButton || event->buttons() == Qt::MiddleButton) {
        chart()->zoomReset();
        UpdateZoom();
    } else
        QChartView::mousePressEvent(event);
    event->ignore();
}

void ChartViewPrivate::wheelEvent(QWheelEvent* event)
{
    if (event->angleDelta().y() < 0)
        emit scaleDown();
    else
        emit scaleUp();

    QPointF inPoint;
    QPointF chartPoint;
    inPoint.setX(event->x());
    inPoint.setY(event->y());
    chartPoint = chart()->mapToValue(inPoint);

    event->ignore();
}

void ChartViewPrivate::setSelectBox(const QPointF& topleft, const QPointF& bottomright)
{
    m_border_start = chart()->mapToPosition(topleft);
    m_border_end = chart()->mapToPosition(bottomright);

    m_saved_zoom_strategy = m_zoom_strategy;
    m_saved_select_strategy = m_select_strategy;

    setSelectStrategy(SelectStrategy::S_Horizontal);
    setZoomStrategy(ZoomStrategy::Z_None);

    m_select_pending = true;
    QRectF rect;
    rect = QRectF(chart()->mapToPosition(topleft), chart()->mapToPosition(bottomright));
    m_rect_start = chart()->mapToPosition(topleft);
    m_select_box->setRect(rect);
    m_select_box->show();
    setFocus();
}

void ChartViewPrivate::mouseMoveEvent(QMouseEvent* event)
{
    if (this->chart()->axes(Qt::Horizontal).isEmpty() || this->chart()->axes(Qt::Vertical).isEmpty()) {
        return;
    }

    if (m_select_pending || m_zoom_pending) {
        QPair<QPointF, QPointF> inPoint = getCurrentRectangle(event);

        QRectF rect;
        rect = QRectF(inPoint.first, inPoint.second);
        if (m_zoom_pending)
            m_zoom_box->setRect(rect);
        else if (m_select_pending && m_single_right_click)
            m_select_box->setRect(rect);
        else if (m_select_pending && !m_single_right_click) {
            //qDebug() << m_border_start.x() << inPoint.first.x() << m_border_end.x()<<inPoint.second.x();
            if (m_border_start.x() <= inPoint.first.x() && m_border_end.x() >= inPoint.second.x()) {
                //  qDebug() << "in range";
                m_select_box->setRect(rect);
            } else {
                m_select_box->setRect(QRectF(m_border_start, m_border_end));
                //  qDebug() << "out";
            }
        }
        QChartView::mouseMoveEvent(event);
        return;
    }

    QPointF widgetPoint;
    QPointF chartPoint;
    widgetPoint.setX(event->x());
    widgetPoint.setY(event->y());
    chartPoint = chart()->mapToValue(widgetPoint);
    UpdateVerticalLine(chartPoint.x()); //, widgetPoint);
    QChartView::mouseMoveEvent(event);
}

void ChartViewPrivate::handleMouseMoved(const QPointF& ChartPoint, const QPointF& WidgetPoint)
{
    UpdateVerticalLine(ChartPoint.x());
}

void ChartViewPrivate::UpdateVerticalLine(double x)
{
    UpdateZoom();
    QPointF start = chart()->mapToPosition(QPointF(x, m_y_min));
    QPointF end = chart()->mapToPosition(QPointF(x, 0.95 * m_y_max));

    m_vertical_line->setLine(start.x(), start.y(), end.x(), end.y());

    m_line_position->setPos(chart()->mapToPosition(QPointF(x, 0.99 * m_y_max)));
    m_line_position->setPlainText(QString::number(x, 'f', 4));
}

void ChartViewPrivate::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton || event->buttons() == Qt::MiddleButton) {
        chart()->zoomReset();
        UpdateZoom();
        emit ZoomChanged();
    } else if (event->button() == Qt::RightButton || event->buttons() == Qt::RightButton) {
        if (m_select_pending) {
            m_single_right_click = false;
            m_select_pending = false;
            QPair<QPointF, QPointF> rect = getCurrentRectangle(event);

            if (m_border_start.x() <= rect.first.x() && m_border_end.x() >= rect.second.x()) {
                emit AddRect(chart()->mapToValue(rect.first), chart()->mapToValue(rect.second));

            } else {
                emit AddRect(chart()->mapToValue(m_border_start), chart()->mapToValue(m_border_end));
            }
            m_vertical_line->setVisible(m_vertical_line_visible);
            m_line_position->setVisible(m_vertical_line_visible);
            m_select_box->hide();

            setSelectStrategy(m_saved_select_strategy);
            setZoomStrategy(m_saved_zoom_strategy);
        }
    } else if (event->button() == Qt::LeftButton || event->buttons() == Qt::LeftButton) {
        if (m_zoom_pending) {
            QPair<QPointF, QPointF> rect = getCurrentRectangle(event);
            chart()->zoomIn(QRectF(rect.first, rect.second));
            ZoomRect(chart()->mapToValue(rect.first), chart()->mapToValue(rect.second));
            UpdateZoom();
            m_zoom_pending = false;
            m_single_left_click = false;
            m_zoom_box->hide();
            m_vertical_line->setVisible(m_vertical_line_visible);
            m_line_position->setVisible(m_vertical_line_visible);
        }
    } else {
        QChartView::mouseReleaseEvent(event);
        if (chart()->axes(Qt::Vertical).isEmpty())
            return;

        QPointer<QtCharts::QValueAxis> yaxis = qobject_cast<QtCharts::QValueAxis*>(chart()->axes(Qt::Vertical).first());
        if (!yaxis)
            return;
        UpdateView(yaxis->min(), yaxis->max());
    }
}

void ChartViewPrivate::mouseDoubleClickEvent(QMouseEvent* event)
{
    UpdateCorner();

    if (event->button() == Qt::RightButton || event->buttons() == Qt::RightButton) {
        event->ignore();
    } else if (event->button() == Qt::LeftButton || event->buttons() == Qt::LeftButton) {
        QPointF inPoint;
        inPoint.setX(event->x());
        inPoint.setY(event->y());
        emit PointDoubleClicked(chart()->mapToValue(inPoint));
    } else
        event->ignore();
}

void ChartViewPrivate::UpdateZoom()
{
    QPointer<QtCharts::QValueAxis> yaxis = qobject_cast<QtCharts::QValueAxis*>(chart()->axes(Qt::Vertical).first());
    if (!yaxis)
        return;

    QPointer<QtCharts::QValueAxis> xaxis = qobject_cast<QtCharts::QValueAxis*>(chart()->axes(Qt::Horizontal).first());
    if (!xaxis)
        return;

    m_y_min = yaxis->min();
    m_y_max = yaxis->max();

    m_x_min = xaxis->min();
    m_x_max = xaxis->max();
}

void ChartViewPrivate::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        m_double_right_clicked = false;
        m_zoom_pending = false;
        m_select_pending = false;
        m_single_left_click = false;
        m_single_right_click = false;
        m_select_box->hide();
        m_zoom_box->hide();
        m_vertical_line->setVisible(m_vertical_line_visible);
        m_line_position->setVisible(m_vertical_line_visible);
        emit EscapeSelectMode();
        break;
    case Qt::Key_Left:
        emit LeftKey();
        break;
    case Qt::Key_Right:
        emit RightKey();
        break;
    default:
        QGraphicsView::keyPressEvent(event);
        break;
    }
}

ChartView::ChartView()
    : has_legend(false)
    , connected(false)
    , m_x_axis(QString())
    , m_y_axis(QString())
    , m_pending(false)
    , m_lock_scaling(false)
{
    m_chart = new QtCharts::QChart();
    m_chart_private = new ChartViewPrivate(m_chart, this);

    connect(m_chart_private, SIGNAL(ZoomChanged()), this, SIGNAL(ZoomChanged()));
    connect(m_chart_private, &ChartViewPrivate::ZoomRect, this, &ChartView::ZoomRect);
    connect(m_chart_private, SIGNAL(scaleDown()), this, SIGNAL(scaleDown()));
    connect(m_chart_private, SIGNAL(scaleUp()), this, SIGNAL(scaleUp()));
    connect(m_chart_private, SIGNAL(AddRect(const QPointF&, const QPointF&)), this, SIGNAL(AddRect(const QPointF&, const QPointF&)));
    connect(m_chart_private, &ChartViewPrivate::PointDoubleClicked, this, &ChartView::PointDoubleClicked);
    connect(m_chart_private, &ChartViewPrivate::EscapeSelectMode, this, &ChartView::EscapeSelectMode);
    connect(m_chart_private, &ChartViewPrivate::RightKey, this, &ChartView::RightKey);
    connect(m_chart_private, &ChartViewPrivate::LeftKey, this, &ChartView::LeftKey);

    m_chart->legend()->setVisible(false);
    m_chart->legend()->setAlignment(Qt::AlignRight);
    setUi();
    setZoomStrategy(ZoomStrategy::Z_Rectangular);
    setSelectStrategy(S_None);
    m_chart_private->setVerticalLineEnabled(false);
}

ChartView::~ChartView()
{
    //WriteSettings(m_last_config);

    qDeleteAll(m_peak_anno);
}

void ChartView::setAnimationEnabled(bool animation)
{
    if (!animation)
        m_chart->setAnimationOptions(QtCharts::QChart::NoAnimation);
    else
        m_chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
}

void ChartView::setUi()
{
    m_name = "chart";
    mCentralLayout = new QGridLayout;
    QMenu* menu = new QMenu(this);

    QAction* plotsettings = new QAction(this);
    plotsettings->setText(tr("Plot Settings"));
    connect(plotsettings, SIGNAL(triggered()), this, SLOT(PlotSettings()));
    menu->addAction(plotsettings);

    m_lock_action = new QAction(this);
    m_lock_action->setText(tr("Lock Scaling"));
    m_lock_action->setCheckable(true);
    connect(m_lock_action, &QAction::triggered, this, [this]() {
        m_lock_scaling = m_lock_action->isChecked();
    });
    menu->addAction(m_lock_action);

    QAction* scaleAction = new QAction(this);
    scaleAction->setText(tr("Rescale Axis"));
    connect(scaleAction, SIGNAL(triggered()), this, SLOT(forceformatAxis()));
    menu->addAction(scaleAction);

    QAction* MinMaxscaleAction = new QAction(this);
    MinMaxscaleAction->setText(tr("Autoscale Min/Max"));
    connect(MinMaxscaleAction, SIGNAL(triggered()), this, SLOT(MinMaxScale()));
    menu->addAction(MinMaxscaleAction);

    QAction* exportpng = new QAction(this);
    exportpng->setText(tr("Export Diagram (PNG)"));
    connect(exportpng, SIGNAL(triggered()), this, SLOT(ExportPNG()));
    menu->addAction(exportpng);

    m_select_strategy = new QMenu(tr("Select Strategy"));

    m_select_none = new QAction(tr("None"));
    m_select_none->setData(SelectStrategy::S_None);
    m_select_none->setCheckable(true);

    m_select_horizonal = new QAction(tr("Horizontal"));
    m_select_horizonal->setData(SelectStrategy::S_Horizontal);
    m_select_horizonal->setCheckable(true);

    m_select_vertical = new QAction(tr("Vertical"));
    m_select_vertical->setData(SelectStrategy::S_Vertical);
    m_select_vertical->setCheckable(true);

    m_select_rectangular = new QAction(tr("Rectangular"));
    m_select_rectangular->setData(SelectStrategy::S_Rectangular);
    m_select_rectangular->setCheckable(true);

    m_select_strategy->addAction(m_select_none);
    m_select_strategy->addAction(m_select_horizonal);
    m_select_strategy->addAction(m_select_vertical);
    m_select_strategy->addAction(m_select_rectangular);

    menu->addMenu(m_select_strategy);
    connect(m_select_strategy, &QMenu::triggered, [this](QAction* action) {
        SelectStrategy select = static_cast<SelectStrategy>(action->data().toInt());
        this->m_chart_private->setSelectStrategy(select);
        m_select_none->setChecked(select == SelectStrategy::S_None);
        m_select_horizonal->setChecked(select == SelectStrategy::S_Horizontal);
        m_select_vertical->setChecked(select == SelectStrategy::S_Vertical);
        m_select_rectangular->setChecked(select == SelectStrategy::S_Rectangular);
    });

    m_zoom_strategy = new QMenu(tr("Zoom Strategy"));

    m_zoom_none = new QAction(tr("None"));
    m_zoom_none->setData(ZoomStrategy::Z_None);
    m_zoom_none->setCheckable(true);

    m_zoom_horizonal = new QAction(tr("Horizontal"));
    m_zoom_horizonal->setData(ZoomStrategy::Z_Horizontal);
    m_zoom_horizonal->setCheckable(true);

    m_zoom_vertical = new QAction(tr("Vertical"));
    m_zoom_vertical->setData(ZoomStrategy::Z_Vertical);
    m_zoom_vertical->setCheckable(true);

    m_zoom_rectangular = new QAction(tr("Rectangular"));
    m_zoom_rectangular->setData(ZoomStrategy::Z_Rectangular);
    m_zoom_rectangular->setCheckable(true);

    m_zoom_strategy->addAction(m_zoom_none);
    m_zoom_strategy->addAction(m_zoom_horizonal);
    m_zoom_strategy->addAction(m_zoom_vertical);
    m_zoom_strategy->addAction(m_zoom_rectangular);

    menu->addMenu(m_zoom_strategy);
    connect(m_zoom_strategy, &QMenu::triggered, [this](QAction* action) {
        ZoomStrategy select = static_cast<ZoomStrategy>(action->data().toInt());
        this->m_chart_private->setZoomStrategy(select);
        m_zoom_none->setChecked(select == ZoomStrategy::Z_None);
        m_zoom_horizonal->setChecked(select == ZoomStrategy::Z_Horizontal);
        m_zoom_vertical->setChecked(select == ZoomStrategy::Z_Vertical);
        m_zoom_rectangular->setChecked(select == ZoomStrategy::Z_Rectangular);
    });

    m_config = new QPushButton(tr("Tools"));
    m_config->setFlat(true);
    m_config->setIcon(QIcon::fromTheme("applications-system"));
    m_config->setMaximumWidth(100);
    m_config->setStyleSheet("QPushButton {background-color: #A3C1DA; color: black;}");
    m_config->setMenu(menu);

    mCentralLayout->addWidget(m_chart_private, 0, 0, 1, 5);
    mCentralLayout->addWidget(m_config, 0, 4, Qt::AlignTop);

    mCentralHolder = new QWidget;
    mCentralHolder->setLayout(mCentralLayout);
    setWidget(mCentralHolder);

    m_chartconfigdialog = new ChartConfigDialog(this);

    connect(m_chartconfigdialog, &ChartConfigDialog::ConfigChanged, this, [this](const ChartConfig& config) {
        this->setChartConfig(config);
        this->WriteSettings(config);
        emit ConfigurationChanged();
    });
    connect(m_chartconfigdialog, SIGNAL(ScaleAxis()), this, SLOT(forceformatAxis()));
    connect(m_chartconfigdialog, SIGNAL(ResetFontConfig()), this, SLOT(ResetFontConfig()));

    ApplyConfigurationChange();

    connect(m_chart_private, &ChartViewPrivate::LockZoom, this, [this]() {
        this->m_lock_scaling = true;
        this->m_lock_action->setChecked(true);
    });

    connect(m_chart_private, &ChartViewPrivate::UnLockZoom, this, [this]() {
        this->m_lock_scaling = false;
        this->m_lock_action->setChecked(false);
    });

    m_x_size = (qApp->instance()->property("xSize").toInt());
    m_y_size = (qApp->instance()->property("ySize").toInt());
    m_scaling = (qApp->instance()->property("chartScaling").toInt());
    m_lineWidth = (qApp->instance()->property("chartScaling").toDouble());
    m_markerSize = (qApp->instance()->property("markerSize").toDouble());
}

void ChartView::setZoomStrategy(ZoomStrategy strategy)
{
    m_chart_private->setZoomStrategy(strategy);
    m_zoom_none->setChecked(strategy == ZoomStrategy::Z_None);
    m_zoom_horizonal->setChecked(strategy == ZoomStrategy::Z_Horizontal);
    m_zoom_vertical->setChecked(strategy == ZoomStrategy::Z_Vertical);
    m_zoom_rectangular->setChecked(strategy == ZoomStrategy::Z_Rectangular);
}

void ChartView::setSelectStrategy(SelectStrategy strategy)
{
    m_chart_private->setSelectStrategy(strategy);
    m_select_none->setChecked(strategy == SelectStrategy::S_None);
    m_select_horizonal->setChecked(strategy == SelectStrategy::S_Horizontal);
    m_select_vertical->setChecked(strategy == SelectStrategy::S_Vertical);
    m_select_rectangular->setChecked(strategy == SelectStrategy::S_Rectangular);
}

QtCharts::QLineSeries* ChartView::addLinearSeries(qreal m, qreal n, qreal min, qreal max)
{
    qreal y_min = m * min + n;
    qreal y_max = m * max + n;
    QtCharts::QLineSeries* series = new QtCharts::QLineSeries(this);
    series->append(min, y_min);
    series->append(max, y_max);
    addSeries(series);
    return series;
}

void ChartView::addSeries(QtCharts::QAbstractSeries* series, bool callout)
{
    if (!m_chart->series().contains(series) || !series) {
        QPointer<QtCharts::QXYSeries> serie = qobject_cast<QtCharts::QXYSeries*>(series);
        if (serie) {
            if (serie->points().size() > 5e3)
                serie->setUseOpenGL(true);
            if (callout) {
                qreal x = 0;
                for (const QPointF& point : serie->points())
                    x += point.x();
                x /= double(serie->points().size());
                QPointF point(x, 1.5);

                QPointer<PeakCallOut> annotation = new PeakCallOut(m_chart);
                annotation->setSeries(series);
                annotation->setText(series->name(), point);
                annotation->setAnchor(point);
                annotation->setZValue(11);
                //annotation->updateGeometry();
                annotation->show();
                connect(series, &QtCharts::QAbstractSeries::visibleChanged, series, [series, annotation]() {
                    annotation->setVisible(series->isVisible());
                });
                connect(serie, &QtCharts::QXYSeries::colorChanged, serie, [serie, annotation]() {
                    annotation->setColor(serie->color());
                });
                connect(serie, &QtCharts::QXYSeries::nameChanged, serie, [serie, annotation, point]() {
                    annotation->setText(serie->name(), point);
                });
                annotation->setColor(serie->color());
                m_peak_anno.append(annotation);
            }
        }
        m_chart->addSeries(series);
        if (!m_hasAxis) {
            m_chart->createDefaultAxes();
            m_XAxis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axes(Qt::Horizontal).first());
            m_YAxis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axes(Qt::Vertical).first());
            m_hasAxis = true;
            ReadSettings();
        } else {
            series->attachAxis(m_XAxis);
            series->attachAxis(m_YAxis);
        }
        m_series << series;
    }
    connect(series, &QtCharts::QAbstractSeries::nameChanged, series, [this, series]() {
        if (series) {
            //qDebug() << series->name();
#pragma message("this can be compressed due to logic gatters")
            bool show = series->name().isEmpty() || series->name().isNull() || series->name().simplified() == QString(" ");
            this->m_chart->legend()->markers(series).first()->setVisible(!show);
        }
    });
    connect(series, &QtCharts::QAbstractSeries::visibleChanged, series, [this, series]() {
        if (series) {
            //qDebug() << series->name();
#pragma message("this can be compressed due to logic gatters")
            bool show = series->name().isEmpty() || series->name().isNull() || series->name().simplified() == QString(" ");
            if (series->isVisible())
                this->m_chart->legend()->markers(series).first()->setVisible(!show);
        }
    });
    //qDebug() << series->name();
    m_chart->legend()->markers(series).first()->setVisible(!(series->name().isEmpty() || series->name().isNull() || series->name().simplified() == QString(" ")));
    connect(series, SIGNAL(visibleChanged()), this, SLOT(forceformatAxis()));
    if (!connected)
        if (connect(this, SIGNAL(AxisChanged()), this, SLOT(forceformatAxis())))
            connected = true;
    forceformatAxis();
}

void ChartView::ClearChart()
{
    m_chart->removeAllSeries();
    emit ChartCleared();
}

void ChartView::formatAxis()
{
    if (m_pending || m_chart->series().isEmpty())
        return;
    forceformatAxis();
}

void ChartView::ZoomRect(const QPointF& point1, const QPointF& point2)
{
    if (m_manual_zoom == true)
        return;
    /*
    qreal max = qMax(point1.x(), point2.x());
    qreal min = qMin(point1.x(), point2.x());
    m_XAxis->setRange(min, max);

    min = qMin(point1.y(), point2.y());
    max = qMax(point1.y(), point2.y());
    m_YAxis->setRange(min, max);
    */
    m_chart_private->UpdateZoom();
}

void ChartView::ScaleAxis(QPointer<QtCharts::QValueAxis> axis, qreal& min, qreal& max)
{

    int mean = (max + min) / 2;

    if (1 < mean && mean < 10) {
        max = std::ceil(max);
        min = std::floor(min);
    } else {
        max = ChartTools::ceil(max - mean) + mean;
        if (min && !(0 < min && min < 1))
            min = ChartTools::floor(min - mean) + mean;
        else
            min = 0;
    }

    int ticks = ChartTools::scale(max - min) / int(ChartTools::scale(max - min) / 5) + 1;
    //ticks = 2*(max-min)-1;
    if (ticks < 10) {
        axis->setTickCount(ticks);
        axis->setRange(min, max);
    } else
        axis->applyNiceNumbers();
}

void ChartView::forceformatAxis()
{
    if (m_lock_scaling || m_chart->series().size() == 0)
        return;
    m_pending = true;

    qreal x_min = 0;
    qreal x_max = 0;
    qreal y_max = 0;
    qreal y_min = 0;
    int start = 0;
    for (QtCharts::QAbstractSeries* series : m_chart->series()) {
        QPointer<QtCharts::QXYSeries> serie = qobject_cast<QtCharts::QXYSeries*>(series);
        if (!serie)
            continue;
        if (!serie->isVisible())
            continue;

        QVector<QPointF> points = serie->pointsVector();
        if (start == 0 && points.size()) {
            y_min = points.first().y();
            y_max = points.first().y();

            x_min = points.first().x();
            x_max = points.first().x();
            start = 1;
        }
        for (int i = 0; i < points.size(); ++i) {
            y_min = qMin(y_min, points[i].y());
            y_max = qMax(y_max, points[i].y());

            x_min = qMin(x_min, points[i].x());
            x_max = qMax(x_max, points[i].x());
        }
    }

    ScaleAxis(m_XAxis, x_min, x_max);
    ScaleAxis(m_YAxis, y_min, y_max);

    m_XAxis->setTitleText(m_x_axis);
    m_YAxis->setTitleText(m_y_axis);

    m_pending = false;
    m_ymax = y_max;
    m_ymin = y_min;
    m_xmin = x_min;
    m_xmax = x_max;

    if (connected)
        m_chartconfigdialog->setConfig(getChartConfig());
    m_chart_private->UpdateZoom();
}

void ChartView::MinMaxScale()
{

    if (m_lock_scaling || m_chart->series().size() == 0)
        return;
    m_pending = true;

    qreal x_min = 0;
    qreal x_max = 0;
    qreal y_max = 0;
    qreal y_min = 0;
    int start = 0;
    for (QtCharts::QAbstractSeries* series : m_chart->series()) {
        QPointer<QtCharts::QXYSeries> serie = qobject_cast<QtCharts::QXYSeries*>(series);
        if (!serie)
            continue;
        if (!serie->isVisible())
            continue;

        QVector<QPointF> points = serie->pointsVector();
        if (start == 0 && points.size()) {
            y_min = points.first().y();
            y_max = points.first().y();

            x_min = points.first().x();
            x_max = points.first().x();
            start = 1;
        }
        for (int i = 0; i < points.size(); ++i) {
            y_min = qMin(y_min, points[i].y());
            y_max = qMax(y_max, points[i].y());

            x_min = qMin(x_min, points[i].x());
            x_max = qMax(x_max, points[i].x());
        }
    }

    m_XAxis->setRange(x_min, x_max);
    m_XAxis->applyNiceNumbers();
    m_YAxis->setRange(y_min, y_max);
    m_YAxis->applyNiceNumbers();

    m_XAxis->setTitleText(m_x_axis);
    m_YAxis->setTitleText(m_y_axis);

    m_ymax = y_max;
    m_ymin = y_min;
    m_xmin = x_min;
    m_xmax = x_max;

    if (connected)
        m_chartconfigdialog->setConfig(getChartConfig());
    m_chart_private->UpdateZoom();
}

void ChartView::PlotSettings()
{
    if (!connected)
        return;
    m_chartconfigdialog->setConfig(getChartConfig());
    m_chartconfigdialog->show();
    m_chartconfigdialog->raise();
    m_chartconfigdialog->activateWindow();
}

void ChartView::setChartConfig(const ChartConfig& chartconfig)
{
    m_last_config = chartconfig;

    m_lock_scaling = chartconfig.m_lock_scaling;
    m_lock_action->setChecked(m_lock_scaling);

    m_x_size = chartconfig.x_size;
    m_y_size = chartconfig.y_size;
    m_scaling = chartconfig.scaling;

    m_markerSize = chartconfig.markerSize;
    m_lineWidth = chartconfig.lineWidth;

    // m_XAxis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisX());
    if (m_XAxis) {
        m_XAxis->setTitleText(chartconfig.x_axis);
        m_XAxis->setTickCount(chartconfig.x_step);
        m_XAxis->setMin(chartconfig.x_min);
        m_XAxis->setMax(chartconfig.x_max);
        m_XAxis->setTitleFont(chartconfig.m_label);
        m_XAxis->setLabelsFont(chartconfig.m_ticks);
        m_XAxis->setVisible(chartconfig.showAxis);
    }
    //QPointer<QtCharts::QValueAxis> m_YAxis = qobject_cast<QtCharts::QValueAxis*>(m_chart->axisY());
    if (m_YAxis) {
        m_YAxis->setTitleText(chartconfig.y_axis);
        m_YAxis->setTickCount(chartconfig.y_step);
        m_YAxis->setMin(chartconfig.y_min);
        m_YAxis->setMax(chartconfig.y_max);
        m_YAxis->setTitleFont(chartconfig.m_label);
        m_YAxis->setLabelsFont(chartconfig.m_ticks);
        m_YAxis->setVisible(chartconfig.showAxis);
    }

    if (chartconfig.m_legend) {
        m_chart->legend()->setVisible(true);
        if (chartconfig.align == Qt::AlignTop
            || chartconfig.align == Qt::AlignBottom
            || chartconfig.align == Qt::AlignLeft
            || chartconfig.align == Qt::AlignRight)
            m_chart->legend()->setAlignment(chartconfig.align);
        else
            m_chart->legend()->setAlignment(Qt::AlignRight);
        m_chart->legend()->setFont(chartconfig.m_keys);
        for (PeakCallOut* call : m_peak_anno)
            call->setFont(chartconfig.m_keys);
    } else {
        m_chart->legend()->setVisible(false);
    }
    setTitle(chartconfig.title);
    m_chart->setTitleFont(chartconfig.m_title);

    int Theme = chartconfig.Theme;
    if (Theme < 8)
        m_chart->setTheme(static_cast<QtCharts::QChart::ChartTheme>(Theme));
    else {
        for (int i = 0; i < m_series.size(); ++i) {
            if (!m_series[i])
                continue;

            if (qobject_cast<QtCharts::QXYSeries*>(m_series[i])) {
                QtCharts::QXYSeries* series = qobject_cast<QtCharts::QXYSeries*>(m_series[i]);
                series->setColor(QColor("black"));
            } else if (qobject_cast<QtCharts::QAreaSeries*>(m_series[i])) {
                QtCharts::QAreaSeries* series = qobject_cast<QtCharts::QAreaSeries*>(m_series[i]);
                QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
                gradient.setColorAt(0.0, QColor("darkGray"));
                gradient.setColorAt(1.0, QColor("lightGray"));
                gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
                series->setBrush(gradient);
                series->setOpacity(0.4);
                QPen pen(QColor("darkGray"));
                pen.setWidth(3);
                series->setPen(pen);
            }
            QBrush brush;
            brush.setColor(Qt::transparent);
            m_chart->setBackgroundBrush(brush);
            //m_chart->setStyleSheet("background-color: transparent;");
        }
        /* QFont font = chartconfig.m_title;
        font.setPointSize(12);
        m_chart->setTitleFont(font);
        m_YAxis->setLabelsFont(font);
        m_XAxis->setLabelsFont(font);
        m_XAxis->setTitleFont(font);
        m_YAxis->setTitleFont(font);

        m_chart->legend()->setFont(font);
        for (PeakCallOut* call : m_peak_anno)
            call->setFont(font);
        */
    }
    for (QPointer<PeakCallOut> call : m_peak_anno) {
        if (!call)
            continue;
        call->setVisible(chartconfig.m_annotation);
        call->setFont(chartconfig.m_keys);
    }
}

void ChartView::WriteSettings(const ChartConfig& chartconfig)
{
    QSettings _settings;
    _settings.beginGroup(m_name);
    _settings.setValue("labels", chartconfig.m_label);
    _settings.setValue("ticks", chartconfig.m_ticks);
    _settings.setValue("title", chartconfig.m_title);
    _settings.setValue("legend", chartconfig.m_keys);
    _settings.endGroup();
}

ChartConfig ChartView::ReadSettings()
{
    ChartConfig chartconfig = m_last_config;
    QSettings _settings;
    if (!_settings.contains(m_name)) {

        QFont font;

        if (!m_font.isNull() && !m_font.isEmpty())
            font = QFont(m_font, 11);

        font.setPointSize(11);
        font.setWeight(QFont::DemiBold);

        chartconfig.m_label = font;
        chartconfig.m_ticks = font;
        chartconfig.m_keys = font;

        font.setPointSize(11);
        chartconfig.m_title = font;
    } else {
        _settings.beginGroup(m_name);
        chartconfig.m_label = _settings.value("labels").value<QFont>();
        chartconfig.m_ticks = _settings.value("ticks").value<QFont>();
        chartconfig.m_title = _settings.value("title").value<QFont>();
        chartconfig.m_keys = _settings.value("legend").value<QFont>();
        _settings.endGroup();
    }
    m_last_config = chartconfig;
    setChartConfig(chartconfig);
    m_chartconfigdialog->setConfig(chartconfig);

    return chartconfig;
}

void ChartView::setTitle(const QString& str)
{
    m_chart->setTitle(str);
}

ChartConfig ChartView::getChartConfig() const
{
    ChartConfig chartconfig;
    if (m_hasAxis) {

        chartconfig.x_axis = m_XAxis->titleText();
        chartconfig.x_min = m_XAxis->min();
        chartconfig.x_max = m_XAxis->max();
        chartconfig.x_step = m_XAxis->tickCount();
        chartconfig.m_label = m_XAxis->titleFont();
        chartconfig.m_ticks = m_XAxis->labelsFont();
        chartconfig.y_axis = m_YAxis->titleText();
        chartconfig.y_min = m_YAxis->min();
        chartconfig.y_max = m_YAxis->max();
        chartconfig.y_step = m_YAxis->tickCount();
    }
    chartconfig.m_legend = m_chart->legend()->isVisible();
    chartconfig.m_lock_scaling = m_lock_scaling;

    chartconfig.m_keys = m_chart->legend()->font();
    chartconfig.align = m_chart->legend()->alignment();
    chartconfig.title = m_chart->title();
    chartconfig.m_title = m_chart->titleFont();

    return chartconfig;
}

QString ChartView::Color2RGB(const QColor& color) const
{
    QString result;
    result = QString::number(color.toRgb().red()) + "," + QString::number(color.toRgb().green()) + "," + QString::number(color.toRgb().blue());
    return result;
}

void ChartView::ExportPNG()
{
    const QString str = QFileDialog::getSaveFileName(this, tr("Save File"),
        qApp->instance()->property("lastDir").toString(),
        tr("Images (*.png)"));
    if (str.isEmpty() || str.isNull())
        return;
    emit LastDirChanged(str);
    // setLastDir(str);

    bool verticalline = m_chart_private->isVerticalLineEnabled();
    m_chart_private->setVerticalLineEnabled(false);

    QtCharts::QChart::AnimationOptions animation = m_chart->animationOptions();

    m_chart->setAnimationOptions(QtCharts::QChart::NoAnimation);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    // first cache the original size, within SupraFit
    QSize widgetSize = mCentralHolder->size();

    // and resize as set in settings
    m_chart->resize(m_x_size, m_y_size);
    mCentralHolder->resize(m_x_size, m_y_size);

    for (PeakCallOut* call : m_peak_anno) {
        call->update();
    }
    m_chart->scene()->update();
    QApplication::processEvents();

    // Waiter wait;
    int w = m_chart->rect().size().width();
    int h = m_chart->rect().size().height();
    // scaling is important for good resolution
    QImage image(QSize(m_scaling * w, m_scaling * h), QImage::Format_ARGB32);

    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    bool xGrid = m_XAxis->isGridLineVisible();
    bool yGrid = m_YAxis->isGridLineVisible();

    // hide grid lines
    if (qApp->instance()->property("noGrid").toBool() == true) {
        m_XAxis->setGridLineVisible(false);
        m_YAxis->setGridLineVisible(false);
    }

    QPen xPen = m_XAxis->linePen();
    QPen yPen = m_YAxis->linePen();

    QPen pen = m_XAxis->linePen();
    pen.setColor(Qt::black);
    pen.setWidth(2);

    // make axis stronger
    if (qApp->instance()->property("empAxis").toBool() == true) {
        m_XAxis->setLinePen(pen);
        m_YAxis->setLinePen(pen);
    }
    QBrush brush_backup = m_chart->backgroundBrush();
    QBrush brush;
    brush.setColor(Qt::transparent);

    // remove background of chart
    if (qApp->instance()->property("transparentChart").toBool() == true)
        m_chart->setBackgroundBrush(brush);

    // cache all individual series size and border colors and remove border colors and resize series

    QList<QColor> colors;
    QList<int> size, width;
    QList<bool> openGl;
    for (QtCharts::QAbstractSeries* serie : m_chart->series()) {

        if (qobject_cast<QtCharts::QScatterSeries*>(serie)) {

            colors << qobject_cast<QtCharts::QScatterSeries*>(serie)->borderColor();
            qobject_cast<QtCharts::QScatterSeries*>(serie)->setBorderColor(Qt::transparent);
            size << qobject_cast<QtCharts::QScatterSeries*>(serie)->markerSize();

            if (qobject_cast<QtCharts::QScatterSeries*>(serie)->markerSize() > m_markerSize)
                qobject_cast<QtCharts::QScatterSeries*>(serie)->setMarkerSize(m_markerSize);

        } else if (qobject_cast<LineSeries*>(serie)) {

            width << qobject_cast<LineSeries*>(serie)->LineWidth();
            qobject_cast<LineSeries*>(serie)->setSize(m_lineWidth / 10.0);
        }
        openGl << serie->useOpenGL();
        serie->setUseOpenGL(false);
    }
    // do the painting!!
    m_chart->scene()->render(&painter, QRectF(0, 0, m_scaling * w, m_scaling * h), m_chart->rect());

    /*
     * copyied from here:
     *
     * https://stackoverflow.com/questions/3720947/does-qt-have-a-way-to-find-bounding-box-of-an-image
     *
     */

    auto border = [](const QImage& tmp) -> QImage {
        int l = tmp.width(), r = 0, t = tmp.height(), b = 0;
        for (int y = 0; y < tmp.height(); ++y) {
            QRgb* row = (QRgb*)tmp.scanLine(y);
            bool rowFilled = false;
            for (int x = 0; x < tmp.width(); ++x) {
                if (qAlpha(row[x])) {
                    rowFilled = true;
                    r = std::max(r, x);
                    if (l > x) {
                        l = x;
                        x = r; // shortcut to only search for new right bound from here
                    }
                }
            }
            if (rowFilled) {
                t = std::min(t, y);
                b = y;
            }
        }
        return tmp.copy(l + 1, t + 1, r + 1, b + 1);
    };

    QPixmap pixmap;

    // remove transparent border of resulting image

    if (qApp->instance()->property("cropedChart").toBool() == true) {
#pragma message("stupid things")
        /* dont unterstand that in particular, but it works somehow, and it is not to slow */

        QImage mirrored = border(border(image.mirrored(true, true)).mirrored(true, true).mirrored(true, true)).mirrored(true, true);
        pixmap = QPixmap::fromImage(border(mirrored));
    } else
        pixmap = QPixmap::fromImage(image);

    // restore background brush
    m_chart->setBackgroundBrush(brush_backup);

    // restore series colors and size
    for (QtCharts::QAbstractSeries* serie : m_chart->series()) {
        if (qobject_cast<QtCharts::QScatterSeries*>(serie)) {
            qobject_cast<QtCharts::QScatterSeries*>(serie)->setBorderColor(colors.takeFirst());
            qobject_cast<QtCharts::QScatterSeries*>(serie)->setMarkerSize(size.takeFirst());
        } else if (qobject_cast<LineSeries*>(serie)) {
            qobject_cast<LineSeries*>(serie)->setSize(width.takeFirst() / 10.0);
        }
        serie->setUseOpenGL(openGl.takeFirst());
    }

    // bring back the grids
    m_XAxis->setGridLineVisible(xGrid);
    m_YAxis->setGridLineVisible(yGrid);

    // bring back the old and weak axis
    m_XAxis->setLinePen(xPen);
    m_YAxis->setLinePen(yPen);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // restore old size
    mCentralHolder->resize(widgetSize);

    for (PeakCallOut* call : m_peak_anno)
        call->Update();

    // and nothing ever happens -> Lemon Tree
    m_chart->setAnimationOptions(animation);

    QFile file(str);
    file.open(QIODevice::WriteOnly);
    pixmap.save(&file, "PNG");

    m_chart_private->setVerticalLineEnabled(verticalline);
}

void ChartView::ApplyConfigurationChange(const QString& str)
{
    if (str == m_name)
        ReadSettings();
    else {
        bool animation = qApp->instance()->property("chartanimation").toBool();
        if (animation)
            m_chart->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
        else
            m_chart->setAnimationOptions(QtCharts::QChart::NoAnimation);

        m_chart->setTheme(QtCharts::QChart::ChartTheme(qApp->instance()->property("charttheme").toInt()));
    }
}

void ChartView::resizeEvent(QResizeEvent* event)
{
    event->accept();
    mCentralHolder->resize(0.99 * size());
    /*
    if(event->size().width() > event->size().height()){
        QWidget::resize(event->size().height(),event->size().height());
    }else{
        QWidget::resize(event->size().width(),event->size().width());
    }*/
}

void ChartView::ResetFontConfig()
{
    ChartConfig chartconfig = m_last_config;
    QFont font;

    if (!m_font.isEmpty() && !m_font.isNull())
        font = QFont(m_font);

    font.setPointSize(11);
    font.setWeight(QFont::DemiBold);

    chartconfig.m_label = font;
    chartconfig.m_ticks = font;
    chartconfig.m_keys = font;
    font.setPointSize(12);

    chartconfig.m_title = font;
    m_last_config = chartconfig;
    setChartConfig(chartconfig);
    m_chartconfigdialog->setConfig(chartconfig);
}

#include "chartview.moc"
