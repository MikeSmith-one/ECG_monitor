#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindowClass())
{
    ui->setupUi(this);

    initCharts();
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
