#include "MainWindow.h"
#include "./ConfigManager/ConfigManager.h"
#include "SettingDialog.h"

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
}

MainWindow::~MainWindow()
{
    if (m_netThread && m_netThread->isRunning()) {
        //先让 NetworkManager 停止记录，强制把内存里的数据落盘，防止丢数据
        QMetaObject::invokeMethod(m_netManager, "stopRecording", Qt::BlockingQueuedConnection);

        m_netThread->quit(); //通知线程退出事件循环
        m_netThread->wait(); //阻塞主线程，等子线程完全结束
    }

    delete ui;
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

        //配置网格线为暗颜色虚线
        plot->xAxis->grid()->setVisible(true);
        plot->xAxis->grid()->setPen(QPen(QColor("#1F2431"), 1, Qt::DashLine));
        plot->yAxis->grid()->setVisible(true);
        plot->yAxis->grid()->setPen(QPen(QColor("#1F2431"), 1, Qt::DashLine));

        //禁用抗锯齿（监护仪波形关闭抗锯齿可大幅提升实时刷新性能）
        plot->setAntialiasedElements(QCP::aeNone);
    }

    //为每个控件创建一条对应颜色的波形 Graph 线条
    ui->plotECG->addGraph()->setPen(QPen(QColor("#00FF7F"), 2));  // 心电：心率绿
    ui->plotSpO2->addGraph()->setPen(QPen(QColor("#00E5FF"), 2)); // 血氧：高亮蓝
    ui->plotRESP->addGraph()->setPen(QPen(QColor("#FFD700"), 2)); // 呼吸：暖金黄

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
}

//UI 数据提取逻辑
void MainWindow::onUiTimerTimeout()
{
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
        ui->lblTEMP->setText(QString("[TEMP 体温 : %1]").arg((tData.TEMP_value) / 10));
        ui->lblTEMP->setText(QString("[TEMP 体温: %1 ℃]").arg(tData.TEMP_value / 10.0, 0, 'f', 1));
        //判断血压数据是否有效
        if (tData.NIBP_Sys > 0 && tData.NIBP_Dia > 0) {
            //收到了有效的血压数据，更新界面
            ui->lblNIBP->setText(QString("[NIBP 血压: %1 / %2 mmHg]").arg(tData.NIBP_Sys).arg(tData.NIBP_Dia));
            //启动/重置 5 秒清空定时器
            m_nibpClearTimer->start(5000);
        }
    }

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

        //检测到 R 波标志位，触发心跳反馈（利用位运算进行判断）
        if (cData.Status_Flag & 0x01) {

            qDebug() << "♥ [心跳检测] R波出现!";
        }

        //检测脱离/归零标志位
        if (cData.Status_Flag & 0x02) {
            qDebug() << "！ [报警] 导联脱落或数据归零!";
        }
    }

    if (hasWaveData) {
        //批量添加数据到 Gtaph
        ui->plotECG->graph(0)->addData(keys, ecgValues);
        ui->plotSpO2->graph(0)->addData(keys, spo2Values);
        ui->plotRESP->graph(0)->addData(keys, respValues);

        //X轴平移滚动逻辑
        if (m_runningTime > 5.0) {
            ui->plotECG->xAxis->setRange(m_runningTime - 5.0, m_runningTime);
            ui->plotSpO2->xAxis->setRange(m_runningTime - 5.0, m_runningTime);
            ui->plotRESP->xAxis->setRange(m_runningTime - 5.0, m_runningTime);
        }
        
        //内存清理逻辑：剔除跑到屏幕左边不可见的历史数据，防止内存溢出和卡顿
        double removeTime = m_runningTime - 6.0;//多保留1秒的余量
        if (removeTime > 0) {
            ui->plotECG->graph(0)->data()->removeBefore(removeTime);
            ui->plotSpO2->graph(0)->data()->removeBefore(removeTime);
            ui->plotRESP->graph(0)->data()->removeBefore(removeTime);
        }

        //触发重绘 (使用 rpQueuedReplot 避免阻塞主线程)
        ui->plotECG->replot(QCustomPlot::rpQueuedReplot);
        ui->plotSpO2->replot(QCustomPlot::rpQueuedReplot);
        ui->plotRESP->replot(QCustomPlot::rpQueuedReplot);
    }
}

//血压 5 秒清空逻辑
void MainWindow::onNibpClearTimeout()
{
    ui->lblNIBP->setText("[NIBP 血压: -- / -- mmHg]");
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
