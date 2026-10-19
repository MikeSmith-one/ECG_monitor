#pragma once

#include <QtWidgets/QWidget>
#include <QTimer>
#include <QDateTime>
#include <QThread>
#include <QSoundEffect>
#include "ui_MainWindow.h"
#include "./ConfigManager/ConfigManager.h"
#include "./Network/NetworkManager.h"
#include "./MultiMedia/AudioManager/AudioManager.h"

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
    //void signalAlarmTrigged(QString alarmType, int value);//触发报警信号
    void signalAlarmTriggered(bool currentIsFlatline, int alarmValue);

private slots:
    void on_btnMore_clicked();
    void on_btnBloodPress_clicked();//血压按钮点击槽函数
    void onUiTimerTimeout();//提取队列数据的定时器响应槽函数
    void onNibpClearTimeout();//5秒清空血压显示的定时器响应槽函数

    void on_btnFreeze_clicked();//冻结按钮点击槽函数
    void on_btnMuteAlarm_clicked();//消音按钮点击槽函数
    void onBlinkTimerTimeout();//闪烁定时器响应槽函数
    void onMuteTimeout();//消音结束定时器响应槽函数

private:
    double m_runningTime = 0.0;//记录波形运行的绝对时间 (X轴坐标）

    bool m_isFrozen = false;         //冻结标志
    bool m_isMuted = false;          //消音标志
    bool m_isAlarming = false;       //当前是否处于报警状态
	bool m_isFlatline = false;       //当前是否处于脱落/归零状态
    bool m_blinkState = false;       //闪烁状态翻转标志

    //各个体征的独立报警标志（持久保存）
    bool m_alarmHR = false;
    bool m_alarmSpO2 = false;
    bool m_alarmRESP = false;
    bool m_alarmTEMP = false;

    QTimer* m_blinkTimer = nullptr;  //报警闪烁定时器
    QTimer* m_muteTimer = nullptr;   //120秒消音定时器
    //音频线程
    QThread* m_audioThread = nullptr;
    AudioManager* m_audioManager = nullptr;

    QTimer* m_uiTimer;               //提取队列数据的定时器
    QTimer* m_nibpClearTimer;        //血压显示清空计时器
    //网络通信线程
    QThread* m_netThread = nullptr;
    NetworkManager* m_netManager = nullptr;
    Ui::MainWindowClass *ui;
};

