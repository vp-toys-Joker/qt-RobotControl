#include "RobotControl.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
//    MainWindow w;
//    w.show();
    RobotControlDlg d;
    QObject::connect(&d, SIGNAL(accepted()), &app, SLOT(quit()));
    d.show();


    return app.exec();
}
