#include "MainWindow.h"
#include <QtWidgets/QApplication>
//#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //加载qss样式表
    QFile qssFile(":/Resource/qss/dark_medical.css");
    if (qssFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&qssFile);
        app.setStyleSheet(stream.readAll());
        qssFile.close();
    }
    else {
        qWarning() << "无法加载 QSS 资源文件！";
    }

    MainWindow window;
    window.show();
    return app.exec();
}
