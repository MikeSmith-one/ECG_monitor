#include "MainWindow.h"
#include "./ConfigManager/ConfigManager.h"
#include "SettingDialog.h"
#include "RecordingDialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindowClass())
{
    ui->setupUi(this);

    initCharts();

    ConfigManager::instance()->loadConfigFromIni();

    // ==========================================
    // 启动实时时钟
    // ==========================================
    QTimer* clockTimer = new QTimer(this);

    //每次定时器超时，就更新一次 Labels
    connect(clockTimer, &QTimer::timeout, this, [=]() {
        //获取当前系统时间，并格式化为 "年-月-日 时:分"
        QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
        ui->labelSystemTime->setText("时间: " + currentTime);
        });

    //启动定时器，每 1000 毫秒 (1秒) 触发一次
    clockTimer->start(1000);

    // ===========================================
    // 创建通信线程
    // ===========================================
    m_netThread = new QThread(this);
    m_netManager = new NetworkManager();

    m_netManager->moveToThread(m_netThread);

    //绑定线程启动信号到串口打开槽函数 (使用 Qt::QueuedConnection 保证在子线程执行)
    connect(m_netThread, &QThread::started, m_netManager, &NetworkManager::startConnection);

    //绑定线程结束信号以清理内存
    connect(m_netThread, &QThread::finished, m_netManager, &QObject::deleteLater);

    m_netThread->start();

    // ============================================
    // UI刷新逻辑
    // ============================================
    m_uiTimer = new QTimer(this);//初始化 UI 刷新定时器 (30ms 刷新一次，用来从队列拿数据)
    connect(m_uiTimer, &QTimer::timeout, this, &MainWindow::onUiTimerTimeout);
    m_uiTimer->start(30);

    m_nibpClearTimer = new QTimer(this);//初始化 5秒 血压清空定时器
    m_nibpClearTimer->setSingleShot(true);
    connect(m_nibpClearTimer, &QTimer::timeout, this, &MainWindow::onNibpClearTimeout);
    
    //将主界面的发送信号，连接到子线程 NetworkManager 的发送槽函数
    connect(this, &MainWindow::sendNetCommand, m_netManager, &NetworkManager::sendCommand);

    // =============================================
    // 初始化报警闪烁与消音定时器
    // =============================================
    m_blinkTimer = new QTimer(this);
    connect(m_blinkTimer, &QTimer::timeout, this, &MainWindow::onBlinkTimerTimeout);

    m_muteTimer = new QTimer(this);
    m_muteTimer->setSingleShot(true);
    connect(m_muteTimer, &QTimer::timeout, this, &MainWindow::onMuteTimeout);

    // ============================================
    // 初始化通信看门狗定时器（2000ms 超时）
    // ============================================
    m_watchdogTimer = new QTimer(this);
    m_watchdogTimer->setSingleShot(true);//单词定时器
    connect(m_watchdogTimer, &QTimer::timeout, this, &MainWindow::onCommWatchdogTimeout);
    m_watchdogTimer->start(2000);//开机默认启动，2秒内无数据则变红

    // ============================================
    // 独立音频子线程
    // ============================================
    m_audioThread = new QThread(this);
    m_audioManager = new AudioManager();
    m_audioManager->moveToThread(m_audioThread);

    connect(m_audioThread, &QThread::started, m_audioManager, &AudioManager::initAudio);
    connect(m_audioThread, &QThread::finished, m_audioManager, &QObject::deleteLater);

    m_audioThread->start();

    // =============================================
    // 独立存储子线程（数据库和日志）
    // =============================================
    m_storageThread = new QThread(this);
    m_storageManager = new StorageManager();
    m_storageManager->moveToThread(m_storageThread);

    connect(m_storageThread, &QThread::started, m_storageManager, &StorageManager::initStorage);
    connect(m_storageThread, &QThread::finished, m_storageManager, &QObject::deleteLater);

    //连接主线程的数据信号到存储线程的保存槽函数
    connect(this, &MainWindow::signalSaveVitals, m_storageManager, &StorageManager::saveVitalSigns);

    m_storageThread->start();

    qDebug() << "初始化主界面成功!";
}

MainWindow::~MainWindow()
{
    if (m_netThread && m_netThread->isRunning()) {
        //先让 NetworkManager 停止记录，强制把内存里的数据落盘，防止丢数据
        QMetaObject::invokeMethod(m_netManager, "stopRecording", Qt::BlockingQueuedConnection);

        m_netThread->quit(); //通知线程退出事件循环
        m_netThread->wait(); //阻塞主线程，等子线程完全结束
    }

    if (m_audioThread && m_audioThread->isRunning()) {
        QMetaObject::invokeMethod(m_audioManager, "stopRecording", Qt::BlockingQueuedConnection);

        m_audioThread->quit(); //通知线程退出事件循环
        m_audioThread->wait(); //阻塞主线程，等子线程完全结束
    }

    if (m_storageThread && m_storageThread->isRunning()) {
        m_storageThread->quit();//通知线程退出事件循环
        m_storageThread->wait();//阻塞主线程，等子线程完全结束
    }

    delete ui;

    qDebug() << "主界面析构完成！";
}

void MainWindow::initCharts()
{
    //放入三个波形控件
    QList<QCustomPlot*> plots = { ui->plotECG, ui->plotSpO2, ui->plotRESP };

    for (auto plot : plots) {
        //设置画板背景色
        plot->setBackground(QBrush(QColor("#0F1117")));
        plot->axisRect()->setBackground(QBrush(QColor("#0F1117")));

        //显示坐标轴，并配置浅灰色（高对比度）画笔
        plot->xAxis->setVisible(true);
        plot->yAxis->setVisible(true);

        //坐标轴画笔
        QPen axisPen(QColor("#555A6E"), 1); 
        plot->xAxis->setBasePen(axisPen);
        plot->yAxis->setBasePen(axisPen);
        plot->xAxis->setTickPen(axisPen);
        plot->yAxis->setTickPen(axisPen);
        plot->xAxis->setSubTickPen(axisPen);
        plot->yAxis->setSubTickPen(axisPen);

        //设置刻度数字文本颜色为浅灰
        plot->xAxis->setTickLabelColor(QColor("#9CA3AF"));
        plot->yAxis->setTickLabelColor(QColor("#9CA3AF"));

        //配置网格线为暗颜色实线
        plot->xAxis->grid()->setVisible(true);
        plot->xAxis->grid()->setPen(QPen(QColor("#181C26"), 1, Qt::SolidLine));
        plot->yAxis->grid()->setVisible(true);
        plot->yAxis->grid()->setPen(QPen(QColor("#181C26"), 1, Qt::SolidLine));

        //禁用抗锯齿、开启 QCustomPlot 的性能加速
        plot->setAntialiasedElements((QCP::aeNone));
        plot->setPlottingHints(QCP::phFastPolylines | QCP::phCacheLabels);

        //允许 Qt 将绘图请求合并到系统的下一个垂直同步信号（vsync）中异步绘制
        plot->setPlottingHint(QCP::phImmediateRefresh, false);
    }

    ui->plotECG->addGraph()->setPen(QPen(QColor("#00FF7F"), 2));  //心电：心率绿
    ui->plotSpO2->addGraph()->setPen(QPen(QColor("#00E5FF"), 2)); //血氧：高亮蓝
    ui->plotRESP->addGraph()->setPen(QPen(QColor("#FFD700"), 2)); //呼吸：暖金黄

    // ---------------------------------------------
    // 配置 Y 轴固定显示范围 (根据模拟器生成的数据范围设定)
    // ---------------------------------------------
    ui->plotECG->yAxis->setRange(-5000, 20000); //心电范围
    ui->plotSpO2->yAxis->setRange(0, 255);      //血氧范围 (0-255)
    ui->plotRESP->yAxis->setRange(0, 255);      //呼吸范围 (0-255) 
   
    // ---------------------------------------------
    // 配置 X 轴（时间轴）
    // ---------------------------------------------
    for (auto plot : plots) {
        //初始 X 轴范围：0 至 5秒
        plot->xAxis->setRange(0, 5.0);
        //隐藏 X 轴的刻度数字
        plot->xAxis->setTickLabels(false);
        //关闭交互
        plot->setInteractions(QCP::iNone);
    }
}

//血压按钮点击事件
void MainWindow::on_btnBloodPress_clicked()
{
    ui->lblNIBP->setText("[NIBP 血压: 测量中...]");

    //发送指令 0x01 给下位机，要求测血压
    emit sendNetCommand(0x01);

    qDebug() << "开始测量血压";
}

//UI 数据提取逻辑
void MainWindow::onUiTimerTimeout()
{
    StationConfig config = ConfigManager::instance()->getCongfig();
    bool currentIsAlarming = false;//标记本周期是否出发了警报
    bool currentIsFlatline = false;//标记本周期是否脱落/归零
    bool hasRWave = false;//标记本周期是否有心跳

    // -------------------------------
    // 右侧数据面板
    //  ------------------------------
    
    //提取参数队列
    TablesData tData;
    while (m_netManager->tablesQueue.dequeue(tData)) {
        //更新心率、血氧等常态数据
        ui->HR_Num->setText(QString::number(tData.HR_Value) + " bpm");
        ui->SpO2_Num->setText(QString::number(tData.SpO2_Value) + " %");
        ui->RESP_Num->setText(QString::number(tData.RESP_Value) + " rpm");
        ui->lblTEMP->setText(QString("[TEMP 体温: %1 ℃]").arg(tData.TEMP_value / 10.0, 0, 'f', 1));
        //判断血压数据是否有效
        if (tData.NIBP_Sys > 0 && tData.NIBP_Dia > 0) {
            //收到了有效的血压数据，更新界面
            ui->lblNIBP->setText(QString("[NIBP 血压: %1 / %2 mmHg]").arg(tData.NIBP_Sys).arg(tData.NIBP_Dia));
            //启动/重置 5 秒清空定时器
            m_nibpClearTimer->start(5000);
        }

        emit signalSaveVitals(tData);//每秒收到参数时，发信号给存储线程落盘

        // ---------------------------------
        // 各个体征的独立阈值报警判定
        // ---------------------------------

        //心率超限判定
        m_alarmHR = (tData.HR_Value > config.alarmLimits["HR"].upperLimit ||
                     tData.HR_Value < config.alarmLimits["HR"].lowerLimit);

        //血氧过低判定
        m_alarmSpO2 = (tData.SpO2_Value < config.alarmLimits["SpO2"].lowerLimit);

        //呼吸率超限判定
        m_alarmRESP = (tData.RESP_Value > config.alarmLimits["RESP"].upperLimit ||
                       tData.RESP_Value < config.alarmLimits["RESP"].lowerLimit);

        //体温超限判定
        double currentTemp = tData.TEMP_value / 10.0;
        m_alarmTEMP = (currentTemp > config.alarmLimits["TEMP"].upperLimit ||
            currentTemp < config.alarmLimits["TEMP"].lowerLimit);
    }

    //报警状态综合判定：只要有任何一个指标超标，或者导联脱落，就触发整机声光报警
    currentIsAlarming = m_alarmHR || m_alarmSpO2 || m_alarmRESP || m_alarmTEMP || currentIsFlatline;

    // ---------------------------------
    // 左侧波形图
    // ---------------------------------

    //波形队列提取逻辑
    ChartsData cData;

    //创建临时容器，用于批量添加数据
    QVector<double> keys, ecgValues, spo2Values, respValues;

    bool hasWaveData = false;//标记本次是否有新波形数据

    while (m_netManager->chartsQueue.dequeue(cData)) {
        hasWaveData = true;

        //每次收到一个点，时间轴前进 0.01 秒
        m_runningTime += 0.01;

        keys.append(m_runningTime);
        ecgValues.append(cData.ECG_Data);
        spo2Values.append(cData.SpO2_Data);
        respValues.append(cData.RESP_Data);

        //状态标志位检测与发声
        if (cData.Status_Flag & 0x02) { //检测脱离/归零标志位
            currentIsFlatline = true;//导联脱落/归零
            currentIsAlarming = true;//异常/超限
        }
        else if (cData.Status_Flag & 0x01) { //检测到 R 波标志位，触发心跳反馈（利用位运算进行判断）
            hasRWave = true;
        }
    }

    // ---------------------------
    // 报警提声光控制逻辑
    // ---------------------------
    if (currentIsAlarming && !m_isAlarming) {
        //刚刚触发报警
        m_isAlarming = true;
        m_blinkTimer->start(500);//启动闪烁

		//推送给护士站的报警信号
        emit signalAlarmTriggered(currentIsFlatline ? "FLATLINE" : "THRESHOLD", 0);

        if (!m_isMuted) {
            if (currentIsFlatline)
                QMetaObject::invokeMethod(m_audioManager, "playFlatline", Qt::QueuedConnection);
            else
                QMetaObject::invokeMethod(m_audioManager, "playWarning", Qt::QueuedConnection);
        }
    }
    //从【报警】恢复为【正常】的那一瞬间
    else if (!currentIsAlarming && m_isAlarming) {
        m_isAlarming = false;
        m_blinkTimer->stop();

        //重置所有控件为默认 QSS 样式
        ui->HR_Num->setStyleSheet("");
        ui->SpO2_Num->setStyleSheet("");
        ui->RESP_Num->setStyleSheet("");
        ui->lblTEMP->setStyleSheet("");

        ui->lblAlarmStatus->setText("阈值报警: 🟩");
        QMetaObject::invokeMethod(m_audioManager, "stopAlarm", Qt::QueuedConnection);
    }

    //在报警期间，状态从【普通超限】恶化为【脱落/归零】
    if (currentIsFlatline && !m_isFlatline) {
        m_isFlatline = true;
        if (!m_isMuted) {
            QMetaObject::invokeMethod(m_audioManager, "stopAlarm", Qt::QueuedConnection);
            QMetaObject::invokeMethod(m_audioManager, "playFlatline", Qt::QueuedConnection);
        }
    }
    //在报警期间，从【脱落/归零】好转为【普通超限】
    else if (!currentIsFlatline && m_isFlatline) {
        m_isFlatline = false;
        if (m_isAlarming && !m_isMuted) {
            QMetaObject::invokeMethod(m_audioManager, "stopAlarm", Qt::QueuedConnection);
            QMetaObject::invokeMethod(m_audioManager, "playWarning", Qt::QueuedConnection);
        }
    }

    //正常心跳发声 (不处于报警、不处于消音、且本周期有R波)
    if (hasRWave && !m_isAlarming && !m_isMuted) {
        QMetaObject::invokeMethod(m_audioManager, "playBeep", Qt::QueuedConnection);
    }

    // -----------------------------
    // 批量绘图与冻结拦截
    // -----------------------------
    if (hasWaveData) {
        //成功拿到了波形数据，重置 2 秒倒计时
        m_watchdogTimer->start(2000);
        m_isCommTimeout = false;

        //如果当前没有被冻结，确保顶部现时为正常的绿色实时推流
        if (!m_isFrozen) {
            ui->labelStatus->setText("状态：🟩 实时推流");
            ui->labelStatus->setStyleSheet("");//恢复默认 QSS 浅色字体
        }

        //批量添加数据到 Gtaph
        ui->plotECG->graph(0)->addData(keys, ecgValues);
        ui->plotSpO2->graph(0)->addData(keys, spo2Values);
        ui->plotRESP->graph(0)->addData(keys, respValues);
        
        //内存清理逻辑：剔除跑到屏幕左边不可见的历史数据，防止内存溢出和卡顿
        double removeTime = m_runningTime - 6.0;//多保留1秒的余量
        if (removeTime > 0) {
            ui->plotECG->graph(0)->data()->removeBefore(removeTime);
            ui->plotSpO2->graph(0)->data()->removeBefore(removeTime);
            ui->plotRESP->graph(0)->data()->removeBefore(removeTime);
        }

       //如果没有冻结，才推进 X 轴并重绘
        if (!m_isFrozen) {
            if (m_runningTime > 5.0) {
                ui->plotECG->xAxis->setRange(m_runningTime - 5.0, m_runningTime);
                ui->plotSpO2->xAxis->setRange(m_runningTime - 5.0, m_runningTime);
                ui->plotRESP->xAxis->setRange(m_runningTime - 5.0, m_runningTime);
            }
            ui->plotECG->replot(QCustomPlot::rpQueuedReplot);
            ui->plotSpO2->replot(QCustomPlot::rpQueuedReplot);
            ui->plotRESP->replot(QCustomPlot::rpQueuedReplot);
        }
    }
}

//血压 5 秒清空逻辑
void MainWindow::onNibpClearTimeout()
{
    ui->lblNIBP->setText("[NIBP 血压: -- / -- mmHg]");

    qDebug() << "血压显示清空";
}

void MainWindow::on_btnMuteAlarm_clicked()
{
    m_isMuted = true;
    ui->btnMuteAlarm->setText("消音中 (120s)");
    ui->btnMuteAlarm->setStyleSheet("background-color: #B8860B; color: white;");//变成暗金色

    //立刻停止当前正在播放的报警音
    QMetaObject::invokeMethod(m_audioManager, "stopAlarm", Qt::QueuedConnection);

    m_muteTimer->start(120000);//120秒后恢复

    qDebug() << "开始静音，120 秒后恢复";
}

void MainWindow::on_btnFreeze_clicked()
{
    m_isFrozen = !m_isFrozen;
    if (m_isFrozen) {
        ui->btnFreeze->setText("解除冻结");
        ui->btnFreeze->setStyleSheet("background-color: #8B0000; color: white;");//变成暗红色提示

        //顶部状态栏联动提示已冻结
        ui->labelStatus->setText("状态：⏸️ 波形已冻结");
        ui->labelStatus->setStyleSheet("color: #00E5FF; font-weight: bold;");

        qDebug() << "状态：波形已冻结";
    }
    else {
        ui->btnFreeze->setText("冻结波形");
        ui->btnFreeze->setStyleSheet("");//恢复默认 QSS

        qDebug() << "解除冻结成功！";
        //解除冻结后，根据通信是否正常恢复对应状态
        if (m_isCommTimeout) {
            ui->labelStatus->setText("状态：🟥 通信中断 / 离线");
            ui->labelStatus->setStyleSheet("color: #FF4D4F; font-weight: bold;");

            qDebug() << "状态：通信中断 / 离线";
        }
        else
        {
            ui->labelStatus->setText("状态：🟩 实时推流");
            ui->labelStatus->setStyleSheet("");

            qDebug() << "状态：实时推流";
        }
    }
}

void MainWindow::on_btnRecording_clicked()
{
    RecordingDialog record(this);

    record.exec();
}

void MainWindow::on_btnMore_clicked()
{
    SettingDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted)
    {
        StationConfig latestConfig = ConfigManager::instance()->getCongfig();

        //更新顶部状态栏的病人信息
        ui->labelName->setText("姓名: " + latestConfig.person.name);
        ui->labelBedNo->setText("床号: " + QString::number(latestConfig.person.benNo));
        ui->labelGender->setText("性别: " + latestConfig.person.gender);

        //更新右侧报警信息
        ui->HR_upper->setText("上限: " + QString::number(latestConfig.alarmLimits["HR"].upperLimit));
        ui->HR_lower->setText("下限: " + QString::number(latestConfig.alarmLimits["HR"].lowerLimit));

        ui->SpO2_upper->setText("上限: " + QString::number(latestConfig.alarmLimits["SpO2"].upperLimit));
        ui->SpO2_lower->setText("下限: " + QString::number(latestConfig.alarmLimits["SpO2"].lowerLimit));

        ui->RESP_upper->setText("上限: " + QString::number(latestConfig.alarmLimits["RESP"].upperLimit));
        ui->RESP_lower->setText("下限: " + QString::number(latestConfig.alarmLimits["RESP"].lowerLimit));

        qDebug() << "主界面已根据最新配置更新UI！";
    }
}

void MainWindow::onBlinkTimerTimeout()
{
    m_blinkState = !m_blinkState;
    if (m_blinkState) {
        //谁超标，谁变红
        if (m_alarmHR)   ui->HR_Num->setStyleSheet("color: #FF0000;");
        if (m_alarmSpO2) ui->SpO2_Num->setStyleSheet("color: #FF0000;");
        if (m_alarmRESP) ui->RESP_Num->setStyleSheet("color: #FF0000;");
        if (m_alarmTEMP) ui->lblTEMP->setStyleSheet("color: #FF0000; font-weight: bold; font-size: 11pt;");

        ui->lblAlarmStatus->setText("阈值报警: 🟥");
    }
    else {
        //熄灭/恢复阶段：恢复各自在 QSS 里的默认主题色
        if (m_alarmHR)   ui->HR_Num->setStyleSheet("color: #00FF7F;");   //恢复心率绿
        if (m_alarmSpO2) ui->SpO2_Num->setStyleSheet("color: #00E5FF;"); //恢复血氧蓝
        if (m_alarmRESP) ui->RESP_Num->setStyleSheet("color: #FFD700;"); //恢复呼吸黄
        if (m_alarmTEMP) ui->lblTEMP->setStyleSheet("");                 //恢复体温默认白

        ui->lblAlarmStatus->setText("阈值报警: 🟩");
    }
}

void MainWindow::onMuteTimeout()
{
    m_isMuted = false;
    ui->btnMuteAlarm->setText("报警消音");
    ui->btnMuteAlarm->setStyleSheet("");
}

void MainWindow::onCommWatchdogTimeout()
{
    m_isCommTimeout = true;

    //如果没有被冻结，才展示断开警告
    if (!m_isFrozen) {
        ui->labelStatus->setText("状态：🟥 通信中断 / 离线");
        qDebug() << "状态：通信中断 / 离线";
        ui->labelStatus->setStyleSheet("color: #FF4D4F; font-weight: bold;");
    }
}
