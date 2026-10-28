#include "StorageManager.h"

StorageManager::StorageManager(QObject* parent) :QObject(parent)
{}

StorageManager::~StorageManager()
{
    flushLogs();//退出前最后刷一次日志
    if (m_db.isOpen()) {
        m_db.close();
    }
}


//静态成员变量初始化
ThreadSafeQueue<QString> StorageManager::s_logQueue;

//全局日志拦截器
void StorageManager::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context)
        QString typeStr;
    switch (type) {
    case QtDebugMsg:    typeStr = "[DEBUG]"; break;
    case QtWarningMsg:  typeStr = "[WARN ]"; break;
    case QtCriticalMsg: typeStr = "[ERROR]"; break;
    case QtFatalMsg:    typeStr = "[FATAL]"; break;
    case QtInfoMsg:     typeStr = "[INFO ]"; break;
    }
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString logLine = QString("%1 %2 %3\n").arg(timeStr).arg(typeStr).arg(msg);

    //压入线程安全队列，让存储子线程去慢慢写磁盘
    s_logQueue.enqueue(logLine);
}

void StorageManager::initStorage()
{
    // ---------------------------------
    // 初始化 SQLite 数据库
    // ---------------------------------
    QString dbPath = ConfigManager::instance()->getDatabasePath() + "medical_data.db";

    //在子线程中创建连接，命名为 "worker_conn" 避免冲突
    m_db = QSqlDatabase::addDatabase("QSQLITE", "worker_conn");
    m_db.setDatabaseName(dbPath);

    if (m_db.open()) {
        QSqlQuery query(m_db);
        //开启 WAL 模式，读写分离不用加锁
        query.exec("PRAGMA journal_mode=WAL;");

        //创建病体征表
        query.exec("CREATE TABLE IF NOT EXISTS VitalSigns ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "timestamp DATETIME DEFAULT (datetime('now', 'localtime')), "
            "bed_no INTEGER, "
            "patient_name TEXT, "
            "hr INTEGER, "
            "spo2 INTEGER, "
            "resp INTEGER, "
            "temp REAL, "
            "sys INTEGER, "
            "dia INTEGER)");
        qDebug() << "数据库初始化成功，WAL模式已开启。";
    }
    else {
        qDebug() << "数据库打开失败:" << m_db.lastError().text();
    }

    // --------------------------------------
    // 初始化定时器
    // --------------------------------------
    //每 500ms 把队列里的日志刷入磁盘
    m_logFlushTimer = new QTimer(this);
    connect(m_logFlushTimer, &QTimer::timeout, this, &StorageManager::flushLogs);
    m_logFlushTimer->start(500);

    //每 1 小时清理一次过期数据
    m_cleanupTimer = new QTimer(this);
    connect(m_cleanupTimer, &QTimer::timeout, this, &StorageManager::cleanUpOldData);
    m_cleanupTimer->start(60 * 60 * 1000);

    //启动时先清理一次
    cleanUpOldData();
}

void StorageManager::saveVitalSigns(const TablesData& data)
{
    if (!m_db.isOpen()) return;

    StationConfig config = ConfigManager::instance()->getConfig();

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO VitalSigns (bed_no, patient_name, hr, spo2, resp, temp, sys, dia) "
        "VALUES (:bed, :name, :hr, :spo2, :resp, :temp, :sys, :dia)");
    query.bindValue(":bed", config.person.benNo);
    query.bindValue(":name", config.person.name);
    query.bindValue(":hr", data.HR_Value);
    query.bindValue(":spo2", data.SpO2_Value);
    query.bindValue(":resp", data.RESP_Value);
    query.bindValue(":temp", data.TEMP_value / 10.0);
    query.bindValue(":sys", data.NIBP_Sys);
    query.bindValue(":dia", data.NIBP_Dia);

    if (!query.exec()) {
        qDebug() << "写入生理数据失败:" << query.lastError().text();
    }
}

void StorageManager::flushLogs()
{
    if (s_logQueue.isEmpty()) return;

    //按天生成日志文件名，如 2026-10-19.log
    QString logFileName = ConfigManager::instance()->getLogPath() + QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log";

    QFile file(logFileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setCodec("UTF-8");
#endif
        QString msg;
        while (s_logQueue.dequeue(msg)) {
            out << msg;
        }
        file.close();
    }
}

void StorageManager::cleanUpOldData()
{
    //清理超过 24h 的数据库记录
    if (m_db.isOpen()) {
        QSqlQuery query(m_db);
        query.exec("DELETE FROM VitalSigns WHERE timestamp <= datetime('now', '-24 hours', 'localtime')");
    }

    //清理超过 7 天的 .log 文件
    QDir logDir(ConfigManager::instance()->getLogPath());
    QStringList filters;
    filters << "*.log";
    QFileInfoList fileList = logDir.entryInfoList(filters, QDir::Files);

    QDateTime sevenDaysAgo = QDateTime::currentDateTime().addDays(-7);
    for (const QFileInfo& fileInfo : fileList) {
        if (fileInfo.lastModified() < sevenDaysAgo) {
            QFile::remove(fileInfo.absoluteFilePath());
            qDebug() << "已自动清理过期日志:" << fileInfo.fileName();
        }
    }
}