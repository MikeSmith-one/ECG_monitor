#pragma once

#include <QtWidgets/Qwidget>
#include <QDialog>
#include <QSerialPortInfo>
#include "ui_SettingDialog.h"
#include "./ConfigManager/ConfigManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SettingDialogClass; };
QT_END_NAMESPACE

class SettingDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SettingDialog(QWidget* parent = nullptr);
	~SettingDialog();

private:
	void loadConfigToUI();

private:
	void onSaveClicked();

	void onResetClicked();

private:
	Ui::SettingDialogClass* ui;
};