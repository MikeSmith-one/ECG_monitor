#include "SettingDialog.h"

SettingDialog::SettingDialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::SettingDialogClass())
{
	ui->setupUi(this);

	this->setWindowTitle("系统设置");
	this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

	//初始化串口列表 (扫描电脑上现有的串口)
	ui->portName->clear();
	QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
	for (const QSerialPortInfo& info : ports) {
		ui->portName->addItem(info.portName());
	}
	//测试防呆设计：如果电脑目前一个物理/虚拟串口都没有，强行塞两个进去占位。
	if (ui->portName->count() == 0) {
		ui->portName->addItems({ "COM1", "COM2" });
	}

	//加载当前配置到界面
	loadConfigToUI();

	//绑定按钮信号槽
	connect(ui->saveBtn, &QPushButton::clicked, this, &SettingDialog::onSaveClicked);
	connect(ui->resetBtn, &QPushButton::clicked, this, &SettingDialog::onResetClicked);
}

SettingDialog::~SettingDialog()
{
	delete ui;
}

void SettingDialog::loadConfigToUI()
{
	//获取当前配置
	StationConfig config = ConfigManager::instance()->getCongfig();

	//患者信息
	ui->personName->setText(config.person.name);
	ui->bedNo->setValue(config.person.benNo);
	ui->personGender->setCurrentText(config.person.gender);

	//报警设置
	ui->HR_upper->setValue(config.alarmLimits["HR"].upperLimit);
	ui->HR_lower->setValue(config.alarmLimits["HR"].lowerLimit);

	ui->SpO2_upper->setValue(config.alarmLimits["SpO2"].upperLimit);
	ui->SpO2_lower->setValue(config.alarmLimits["SpO2"].lowerLimit);

	ui->RESP_upper->setValue(config.alarmLimits["RESP"].upperLimit);
	ui->RESP_lower->setValue(config.alarmLimits["RESP"].lowerLimit);

	ui->H_BP_upper->setValue(config.alarmLimits["Sys"].upperLimit);
	ui->H_BP_lower->setValue(config.alarmLimits["Sys"].lowerLimit);

	ui->L_BP_upper->setValue(config.alarmLimits["Dia"].upperLimit);
	ui->L_BP_lower->setValue(config.alarmLimits["Dia"].lowerLimit);

	ui->TEMP_upper->setValue(config.alarmLimits["TEMP"].upperLimit);
	ui->TEMP_lower->setValue(config.alarmLimits["TEMP"].lowerLimit);

	//系统设置
	//如果扫描到的串口列表里，没有之前保存在 ini 里的那个串口，
	//把它强行加进去，防止显示为空白。
	if (ui->portName->findText(config.PortName) == -1) {
		ui->portName->addItem(config.PortName);
	}
	ui->portName->setCurrentText(config.PortName);
}

void SettingDialog::onSaveClicked()
{
	StationConfig newConfig;

	//保存配置信息
	newConfig.person.name = ui->personName->text();
	newConfig.person.benNo = ui->bedNo->value();
	newConfig.person.gender = ui->personGender->currentText();

	newConfig.alarmLimits["HR"].upperLimit = ui->HR_upper->value();
	newConfig.alarmLimits["HR"].lowerLimit = ui->HR_lower->value();

	newConfig.alarmLimits["SpO2"].upperLimit = ui->SpO2_upper->value();
	newConfig.alarmLimits["SpO2"].lowerLimit = ui->SpO2_lower->value();

	newConfig.alarmLimits["RESP"].upperLimit = ui->RESP_upper->value();
	newConfig.alarmLimits["RESP"].lowerLimit = ui->RESP_lower->value();

	newConfig.alarmLimits["Sys"].upperLimit = ui->H_BP_upper->value();
	newConfig.alarmLimits["Sys"].lowerLimit = ui->H_BP_lower->value();

	newConfig.alarmLimits["Dia"].upperLimit = ui->L_BP_upper->value();
	newConfig.alarmLimits["Dia"].lowerLimit = ui->L_BP_lower->value();

	newConfig.alarmLimits["TEMP"].upperLimit = ui->TEMP_upper->value();
	newConfig.alarmLimits["TEMP"].lowerLimit = ui->TEMP_lower->value();

	newConfig.PortName = ui->portName->currentText();

	//写入配置文件
	ConfigManager::instance()->saveConfig(newConfig);

	//关闭弹窗并返回 Accepted 状态到主界面
	this->accept();
}

void SettingDialog::onResetClicked()
{
	// 恢复出厂默认设置（硬编码）
	ui->personName->setText("张三");
	ui->bedNo->setValue(1);
	ui->personGender->setCurrentText("男");

	ui->HR_lower->setValue(50.0);
	ui->HR_upper->setValue(120.0);

	ui->SpO2_lower->setValue(90.0);
	ui->SpO2_upper->setValue(100.0);

	ui->RESP_lower->setValue(10.0);
	ui->RESP_upper->setValue(30.0);

	ui->H_BP_lower->setValue(90.0);
	ui->H_BP_upper->setValue(140.0);

	ui->L_BP_lower->setValue(60.0);
	ui->L_BP_upper->setValue(90.0);

	ui->TEMP_lower->setValue(35.0);
	ui->TEMP_upper->setValue(37.5);

	// 串口通常重置为 COM1
	if (ui->portName->findText("COM1") == -1) {
		ui->portName->addItem("COM1");
	}
	ui->portName->setCurrentText("COM1");
}
