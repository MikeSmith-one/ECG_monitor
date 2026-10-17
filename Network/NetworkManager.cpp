#include "NetworkManager.h"

NetworkManager::NetworkManager(QObject* parent)
	: QObject(parent)
	, m_serialPort(nullptr)
	, m_state(STATE_FIND_HEAD1)
{
}

NetworkManager::~NetworkManager()
{
	if (m_serialPort && m_serialPort->isOpen()) {
		m_serialPort->close();
	}
}

void NetworkManager::startConnection()
{
	if (!m_serialPort) {
		m_serialPort = new QSerialPort(this);
		connect(m_serialPort, &QSerialPort::readyRead, this, &NetworkManager::onReadyRead);
	}

	if (m_serialPort->isOpen()) {
		m_serialPort->close();
	}

	//从配置管理器获取最新配置
	StationConfig config = ConfigManager::instance()->getCongfig();
	m_serialPort->setPortName(config.PortName);
	m_serialPort->setBaudRate(config.BaudRate);
	m_serialPort->setDataBits(QSerialPort::Data8);
	m_serialPort->setParity(QSerialPort::NoParity);
	m_serialPort->setStopBits(QSerialPort::OneStop);
	m_serialPort->setFlowControl(QSerialPort::NoFlowControl);//？？？？？？不懂这是做什么的

	if (m_serialPort->open(QIODevice::ReadWrite)) {
		qDebug() << "[NetworkManager] 串口打开成功:" << config.PortName << "@" << config.BaudRate;
	}
	else {
		qDebug() << "[NetworkManager] 串口打开失败:" << config.PortName << m_serialPort->errorString();
	}
}

void NetworkManager::stopConnection()
{
	if (m_serialPort && m_serialPort->isOpen()) {
		m_serialPort->close();
		qDebug() << "[NetworkManager] 串口已关闭.";
	}
}

void NetworkManager::onReadyRead()
{
	m_buffer.append(m_serialPort->readAll());

	//状态机开始工作
	while (!m_buffer.isEmpty()) {
		switch (m_state) {
		//将 m_buffer 普通字节数组强转为 quint8 类型的字节数组，便于“切分”
		case STATE_FIND_HEAD1:
			if (static_cast<quint8>(m_buffer.at(0)) == 0xAA) {
				m_state = STATE_FIND_HEAD2;//找到头1，进入下一状态
			}
			m_buffer.remove(0, 1);//无论是否为0xAA都当做当前字节已处理，踢掉
			break;

		case STATE_FIND_HEAD2:
			if (static_cast<quint8>(m_buffer.at(0)) == 0x55) {
				m_state = STATE_READ_TYPE;
				m_buffer.remove(0, 1);
			}
			else
			{
				//如果不是0x55，数据异常，退回寻找头1
				m_state = STATE_FIND_HEAD1;
				//这里不移除当前字节，因为它可能刚好是一个新的0xAA
				//m_buffer.remove(0, 1);
			}
			break;

		case STATE_READ_TYPE:
			m_currentType = static_cast<quint8>(m_buffer.at(0));
			m_state = STATE_READ_LENGTH;
			m_buffer.remove(0, 1);
			break;

		case STATE_READ_LENGTH:
			m_currentLength = static_cast<quint8>(m_buffer.at(0));
			m_state = STATE_READ_DATA;
			m_buffer.remove(0, 1);
			break;

		case STATE_READ_DATA:
			//检查缓冲区数据是否足够（Payload长度 + 1字节的校验和）
			if (m_buffer.size() < m_currentLength + 1) {
				return;//数据不够说明是半包，直接退出等待下一次 readyRead 补齐
			}

			//计算校验和（Type + length + Payload）
			quint8 calcSum = m_currentType + m_currentLength;
			for (int i = 0; i < m_currentLength; ++i) {
				calcSum += static_cast<quint8>(m_buffer.at(i));
			}

			quint8 receivedChecksum = static_cast<quint8>(m_buffer.at(m_currentLength));
			
			if (calcSum == receivedChecksum) {
				//校验通过，提取 Payload 进行解析
				prasePayload(m_currentType, m_buffer.left(m_currentLength));
			}
			else {
				qDebug() << "[NetworkManager] 校验和错误! 计算值: 0x" << QString::number(calcSum, 16).toUpper()
					<< " 接收值: 0x" << QString::number(receivedChecksum, 16).toUpper();
			}

			//移除已处理的一帧数据（Payload + Checksum）
			m_buffer.remove(0, m_currentLength + 1);
			m_state = STATE_FIND_HEAD1;//状态机复位，准备解析下一帧
			break;
		}
	}
}

void NetworkManager::prasePayload(quint8 type, const QByteArray& payload)
{
	if (type == 0x01 && payload.size() == 5) {
		ChartsData data;
		//小端模式解析
		data.ECG_Data = static_cast<qint16>(static_cast<quint8>(payload[0]) | (static_cast<quint8>(payload[1]) << 8));//通过位运输将拆分好的两个8位字节的数据拼成16位的整数，因为串口一次只能发一个字节的数据
		data.SpO2_Data = static_cast<quint8>(payload[2]);
		data.RESP_Data = static_cast<quint8>(payload[3]);
		data.Status_Flag = static_cast<quint8>(payload[4]);

		//加入队列
		chartsQueue.enqueue(data);

		//打印测试
#ifdef QT_DEBUG
		qDebug() << "[波形包] ECG:" << data.ECG_Data
			<< " SpO2:" << data.SpO2_Data
			<< " RESP:" << data.RESP_Data
			<< " Flag: 0x" << QString::number(data.Status_Flag, 16).toUpper();
#endif
	}
	else if (type == 0x02 && payload.size() == 7) {
		TablesData data;
		data.HR_Value = static_cast<quint8>(payload[0]);
		data.SpO2_Value = static_cast<quint8>(payload[1]);
		data.RESP_Value = static_cast<quint8>(payload[2]);
		data.TEMP_value = static_cast<quint16>(static_cast<quint8>(payload[3]) | (static_cast<quint8>(payload[4]) << 8));
		data.NIBP_Sys = static_cast<quint8>(payload[5]);
		data.NIBP_Dia = static_cast<quint8>(payload[6]);

		tablesQueue.enqueue(data);
	}
	else
	{
		qDebug() << "[NetworkManager] 未知包类型或长度不匹配. Type:" << type << "Len:" << payload.size();
	}
}

void NetworkManager::sendCommand(quint8 cmdId)
{
	if (!m_serialPort || !m_serialPort->isOpen()) return;

	QByteArray packet;
	packet.append(static_cast<char>(0xAA)); // 帧头1
	packet.append(static_cast<char>(0x55)); // 帧头2
	packet.append(static_cast<char>(0x03)); // 类型 Type = 0x03 (控制指令)
	packet.append(static_cast<char>(0x01)); // 长度 Length = 1
	packet.append(static_cast<char>(cmdId)); // 负载 Payload

	//计算校验和
	quint8 checksum = 0x03 + 0x01 + cmdId;
	packet.append(static_cast<char>(checksum));

	m_serialPort->write(packet);
	qDebug() << "[NetworkManager] 已向下位机发送指令:"; //<< QString::number(cmdId, 16).toUpper();
}