#pragma once

#include <QtWidgets/QWidget>
#include "ui_MainWindow.h"

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

private:
    Ui::MainWindowClass *ui;
};

