#pragma once

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include "NetworkManager.h"
#include "./ConfigManager/ConfigManager.h"

// =============================================
// 全局 Network 管理器（运行在独立线程）
// =============================================

class NetworkManager : public QObject {
	Q_OBJECT
public:
	explicit NetworkManager(QObject* parent = nullptr);
	~NetworkManager();

public slots:
	

};