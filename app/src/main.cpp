#include "mainwindow.h"

#include <QTranslator>
#include <QApplication>
#include <QFile>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);
    MainWindow w;

    QTranslator qtSerialTranslator;
    if (qtSerialTranslator.load("qtserialport_" + QLocale::system().name(),QDir::currentPath() + "/translations" ))
    {   qApp->installTranslator(&qtSerialTranslator); }
    else {    qDebug() << "not load qtSerialTranslator";    }

    QTranslator qtBaseTranslator;
    if (qtBaseTranslator.load("qtbase_" + QLocale::system().name(),QDir::currentPath() + "/translations" ))
    {   qApp->installTranslator(&qtBaseTranslator); }
    else {    qDebug() << "not load qtBaseTranslator";    }

    QFile styleF;

    styleF.setFileName(QDir::currentPath() + "/style/Combinear.qss");
    styleF.open(QFile::ReadOnly);
    QString qssStr = styleF.readAll();

    w.setStyleSheet(qssStr);
    w.loadSettings();
    w.show();
    return a.exec();
}
