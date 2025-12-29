#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("K4Controller");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("K4Radio");

    MainWindow window;
    window.show();

    return app.exec();
}
