#pragma once

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QDebug>
#include "StationData.h"
#include "ThreadSafeQueue.h"
#include "./ConfigManager/ConfigManager.h"

// =============================================
// 全局 Network 管理器（运行在独立线程）
// =============================================

class NetworkManager : public QObject {
	Q_OBJECT
public:
	explicit NetworkManager(QObject* parent = nullptr);
	~NetworkManager();

	//暴露给外部获取数据的线程安全队列
	ThreadSafeQueue<ChartsData> chartsQueue;
	ThreadSafeQueue<TablesData> tablesQueue;

public slots:
	//启动/停止串口
	void startConnection();
	void stopConnection();

	//接收主线程的发送指令请求
	void sendCommand(quint8 cmdId);

private slots:
	//串口接收中断
	void onReadyRead();

private:
	//解析具体的 Payload 数据
	void prasePayload(quint8 type, const QByteArray& payload);

	QSerialPort* m_serialPort;
	QByteArray m_buffer; //环形缓冲区，用于解决半包/粘包问题

	//状态机枚举
	enum ParseState {
		STATE_FIND_HEAD1,   //寻找帧头1 (0xAA)
		STATE_FIND_HEAD2,   //寻找帧头2 (0x55)
		STATE_READ_TYPE,    //读取包类型
		STATE_READ_LENGTH,  //读取数据长度
		STATE_READ_DATA     //读取数据体和校验和
	};

	ParseState m_state;
	quint8 m_currentType;	//当前包的类型
	quint8 m_currentLength;	//当前包数据位的长度
};