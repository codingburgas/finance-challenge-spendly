#include "mainwindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/assets/assets/icon.png")); // Use the resource path
    QApplication::setWindowIcon(QIcon(":/assets/assets/icon-mac.icns"));
    MainWindow w;
    w.show();
    return a.exec();
}
