#pragma once

#include <QtWidgets/QWidget>
#include <QTimer>
#include <QDateTime>
#include "ui_MainWindow.h"
#include "./ConfigManager/ConfigManager.h"

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

private slots:
    void on_btnMore_clicked();

private:
    Ui::MainWindowClass *ui;
};

