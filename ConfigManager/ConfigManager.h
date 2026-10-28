#pragma once

#include <QObject>
#include <QVector>
#include <QMap>
#include <QString>
#include <QSettings>
#include <QReadWriteLock>
#include <QCoreApplication>
#include <QSet>
#include <QDebug>

// =======================================
// 数据结构定义
// =======================================

//报警阈值结构体
struct AlarmLimit {
	double lowerLimit = 0;//报警下限
	double upperLimit = 0;//报警上限
};

//病人信息结构体
struct Person {
	int benNo = 0;
	QString name;
	QString gender;
};

//日志和生理信息存储路径结构体
struct StoragePath {
	QString logPath;       //日志存放目录
	QString dbPath;        //数据库存放目录
};

//完整配置
struct StationConfig {
	QString PortName;
	int BaudRate = 115200;
	Person person;
	QMap<QString, AlarmLimit> alarmLimits;
	StoragePath storage;
};

// ========================================
// CConfigManager：全局线程安全配置单例类
// ========================================

class ConfigManager : public QObject {
	Q_OBJECT

public:
	static ConfigManager* instance();

	//获取文件路径
	QString getIniPath() const;
	QString getLogPath() const;
	QString getDatabasePath() const;

	//加载配置
	void loadConfigFromIni();

	//获取当前配置副本供界面显示
	StationConfig getConfig() const;
	//界面修改后，调用此函数保存并写入 ini
	void saveConfig(const StationConfig& newConfig);


private:
	ConfigManager() = default;
	~ConfigManager() = default;
	ConfigManager(const ConfigManager&) = delete;
	ConfigManager& operator=(const ConfigManager&) = delete;

	QString m_iniPath = QCoreApplication::applicationDirPath() + "/config/config.ini";
	QString m_DatabasePath = QCoreApplication::applicationDirPath() + "/database/";
	QString m_logPath = QCoreApplication::applicationDirPath() + "/logs/";

	StationConfig m_config;
	mutable QReadWriteLock m_lock;
};