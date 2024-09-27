#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    QFile styleF;

    styleF.setFileName(QDir::currentPath()+ "\\style\\Combinear.qss");
    styleF.open(QFile::ReadOnly);
    QString qssStr = styleF.readAll();

    w.setStyleSheet(qssStr);
    w.loadSettings();
    w.show();
    return a.exec();
}
