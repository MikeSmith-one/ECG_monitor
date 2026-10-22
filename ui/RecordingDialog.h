#pragma once

#include <QtWidgets/Qwidget>
#include <QDialog>
#include <QDate>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "ui_RecordingDialog.h"
#include "./ConfigManager/ConfigManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class RecordingDialogClass; };
QT_END_NAMESPACE

class RecordingDialog : public QDialog
{
	Q_OBJECT

public:
	explicit RecordingDialog(QWidget* parent = nullptr);
	~RecordingDialog();

private slots:
	//浏览保存路径按钮 
	void on_logLocationChoice_clicked();
	void on_dataLocationChoice_clicked();

	//导出操作按钮
	void on_outPutLog_clicked();
	void on_outPutData_clicked();

	//日期联动防呆
	void on_startDate_dateChanged(const QDate& date);
	void on_endDate_dateChanged(const QDate& date);

private:
	void initUI();

private:
	Ui::RecordingDialogClass* ui;
};
