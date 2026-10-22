#include "MainWindow.h"
#include <QtWidgets/QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "./StorageManager/StorageManager.h"

int main(int argc, char *argv[])
{
    //全局日志拦截器
    qInstallMessageHandler(StorageManager::messageHandler);

    QApplication app(argc, argv);

    //加载qss样式表
    QFile qssFile(":/Resource/qss/dark_medical.css");
    if (qssFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&qssFile);
        qApp->setStyleSheet(stream.readAll());
        qssFile.close();
    }
    else {
        qWarning() << "无法加载 QSS 资源文件！";
    }

    MainWindow window;
    window.show();
    return app.exec();
}
