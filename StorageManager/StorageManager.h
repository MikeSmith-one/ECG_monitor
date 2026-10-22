#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include "./Network/StationData.h"
#include "./Network/ThreadSafeQueue.h"
#include "./ConfigManager/ConfigManager.h"

class StorageManager : public QObject
{
	Q_OBJECT
public:
	explicit StorageManager(QObject* parent = nullptr);
	~StorageManager();

	//全局静态日志队列与拦截器函数
	static ThreadSafeQueue<QString> s_logQueue;
	static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

public slots:
	//在子线程中初始化数据库和日志
	void initStorage();

	//接收主线程发来的生理数据并存入数据库
	void saveVitalSigns(const TablesData& data);

private slots:
	//定时将队列中的日志写入磁盘
	void flushLogs();

	//定时清理过期数据 (24h数据库, 7天日志)
	void cleanUpOldData();

private:
	QSqlDatabase m_db;
	QTimer* m_logFlushTimer;
	QTimer* m_cleanupTimer;
};