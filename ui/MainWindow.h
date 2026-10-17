#pragma once

#include <QtWidgets/QWidget>
#include <QTimer>
#include <QDateTime>
#include <QThread>
#include "ui_MainWindow.h"
#include "./ConfigManager/ConfigManager.h"
#include "./Network/NetworkManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindowClass; };
QT_END_NAMESPACE

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void initCharts();

signals:
    void sendNetCommand(quint8 cmdId);//用于跨线程通知 NetworkManager 发送指令

private slots:
    void on_btnMore_clicked();
    void on_btnBloodPress_clicked();//血压按钮点击槽函数
    void onUiTimerTimeout();//提取队列数据的定时器响应槽函数
    void onNibpClearTimeout();//5秒清空血压显示的定时器响应槽函数

private:
    double m_runningTime = 0.0;//记录波形运行的绝对时间 X轴坐标）

    QTimer* m_uiTimer;//提取队列数据的定时器
    QTimer* m_nibpClearTimer;//血压显示清空计时器
    QThread* m_netThread = nullptr;
    NetworkManager* m_netManager = nullptr;
    Ui::MainWindowClass *ui;
};

