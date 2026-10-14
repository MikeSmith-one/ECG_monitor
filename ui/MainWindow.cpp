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
}

MainWindow::~MainWindow()
{
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
