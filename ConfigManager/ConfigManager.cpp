#include "ConfigManager.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

ConfigManager* ConfigManager::instance()
{
	static ConfigManager inst;
	return &inst;
}

QString ConfigManager::getIniPath() const
{
	QReadLocker locker(&m_lock);
	return m_iniPath;
}

QString ConfigManager::getDatabasePath() const
{
	QReadLocker locker(&m_lock);
	return m_DatabasePath;
}

void ConfigManager::loadConfigFromIni()
{
	QWriteLocker locker(&m_lock);

	//确保 Config 和 Database 目录存在
	QFileInfo iniInfo(m_iniPath);
	QDir dir = iniInfo.absoluteDir();
	if (!dir.exists()) {
		dir.mkdir(".");
	}
	QDir dbDir(m_DatabasePath);
	if (!dbDir.exists()) {
		dbDir.mkpath(".");
	}

	//读取 ini 文件
	QSettings settings(m_iniPath, QSettings::IniFormat);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	settings.setIniCodec("UTF-8");
#endif

	// ===============================
	// 解析 [Network] 节点
	// ===============================
	settings.beginGroup("Network");
	m_config.PortName = settings.value("PortName", "COM1").toString();
	m_config.BaudRate = settings.value("BaudRate", 115200).toInt();

	//立即回写，确保文件中有这些字段
	settings.setValue("PortName", m_config.PortName);
	settings.setValue("BaudRate", m_config.BaudRate);
	settings.endGroup();

	// ===============================
	// 解析 [Patient] 节点
	// ===============================
	settings.beginGroup("Patient");
	m_config.person.benNo = settings.value("BedNo", 01).toInt();
	m_config.person.name = settings.value("Name", "张三").toString();
	m_config.person.gender = settings.value("Gender", "男").toString();

	settings.setValue("BendNo", m_config.person.benNo);
	settings.setValue("Name", m_config.person.name);
	settings.setValue("Gender", m_config.person.gender);
	settings.endGroup();

	// =================================
	// 解析 [Alarms] 节点，存入QMap
	// =================================
	settings.beginGroup("Alarms");
	m_config.alarmLimits.clear();

	//默认上下限
	struct DefaultLimit {
		QString key;
		double min;
		double max;
	};

	QList<DefaultLimit> defaultLimits = {
		{"HR",   50.0, 120.0},
		{"SpO2", 90.0, 100.0},
		{"RESP", 10.0, 30.0},
		{"Sys",  90.0, 140.0}, //血压高压
		{"Dia",  60.0, 90.0},  //血压低压
		{"TEMP", 35.0, 37.5}   //体温
	};

	//遍历读取并存入 QMap
	for (const auto& def: defaultLimits) {
		AlarmLimit limit;
		//拼凑键名
		limit.upperLimit = settings.value(def.key + "_Max", def.max).toDouble();
		limit.lowerLimit = settings.value(def.key + "_Min", def.min).toDouble();

		//存入内存的 Map 中
		m_config.alarmLimits.insert(def.key, limit);

		//回写到 ini 文件中
		settings.setValue(def.key + "_Max", limit.upperLimit);
		settings.setValue(def.key + "_Min", limit.lowerLimit);
	}
	settings.endGroup();

	//强制同步写入磁盘
	settings.sync();

	qDebug() << "Configuration loaded and synced to:" << m_iniPath;
}

// =====================================
// 供界面调用配置的接口
// =====================================
StationConfig ConfigManager::getCongfig() const
{
	QReadLocker locker(&m_lock);
	return m_config;
}

void ConfigManager::saveConfig(const StationConfig& newConfig)
{
	QWriteLocker locker(&m_lock);

	//更新内存中的配置
	m_config = newConfig;

	//准备写入 ini 文件
	QSettings settings(m_iniPath, QSettings::IniFormat);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	settings.setIniCodec("UTF-8");
#endif

	// ==================================
	// 覆写 [Netwoek] 节点
	// ==================================
	settings.beginGroup("Network");
	settings.setValue("PortName", m_config.PortName);
	settings.setValue("BaudRate", m_config.BaudRate);
	settings.endGroup();

	// ====================================
	// 覆写 [Patient] 节点
	// ====================================
	settings.beginGroup("Patient");
	settings.setValue("BedNo", m_config.person.benNo);
	settings.setValue("Name", m_config.person.name);
	settings.setValue("Gender", m_config.person.gender);
	settings.endGroup();

	// ====================================
	// 覆写 [Alarms] 节点
	// ====================================
	settings.beginGroup("Alarms");
	
	for (auto it = m_config.alarmLimits.constBegin();it != m_config.alarmLimits.constEnd();++it) {
		QString keyName = it.key();
		settings.setValue(keyName + "_Max", it.value().upperLimit);
		settings.setValue(keyName + "_Min", it.value().lowerLimit);
	}
	settings.endGroup();

	//强制同步到磁盘
	settings.sync();

	qDebug() << "Configuration saved and synced to:" << m_iniPath;
}
