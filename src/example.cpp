
#include <QApplication>
#include <QWidget>

#include "charts.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    ChartView* view = new ChartView;
    view->show();

    return app.exec();
}
