
#include <QApplication>
#include <QMainWindow>
#include <QWidget>

#include "charts.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QMainWindow* window = new QMainWindow;

    ChartView* view = new ChartView;
    LineSeries* series = new LineSeries;
    for (int i = 0; i < 10000; ++i)
        series->append(i / 100.0, std::sin(i / 100.0));

    view->addSeries(series);
    window->setCentralWidget(view);
    window->setWindowTitle(("Example Series for CuteCharts"));
    window->show();
    window->resize(800, 600);
    return app.exec();
}
