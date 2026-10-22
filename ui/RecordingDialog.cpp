#include "RecordingDialog.h"

RecordingDialog::RecordingDialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::RecordingDialogClass())
{
	ui->setupUi(this);
}

RecordingDialog::~RecordingDialog()
{
	delete ui;
}

void RecordingDialog::initUI()
{
    //初始化系统日志日期（默认过去 7 天到今天）
    QDate today = QDate::currentDate();
    ui->endDate->setDate(today);
    ui->endDate->setMaximumDate(today);

    ui->startDate->setDate(today.addDays(-7));
    ui->startDate->setMaximumDate(today);

    //默认导出文件命名与预设路径（预设到桌面）
    QString desktopPath = QDir::homePath() + "/Desktop";
    ui->outPutLocationLog->setText(desktopPath + QString("/System_Logs_%1_%2.txt")
        .arg(ui->startDate->date().toString("yyyyMMdd"))
        .arg(ui->endDate->date().toString("yyyyMMdd")));

    ui->outPutLocationData->setText(desktopPath + QString("/Patient_VitalSigns_24h_%1.csv")
        .arg(today.toString("yyyyMMdd")));
}

// -----------------------------------------
// 文件选择与防呆逻辑
// -----------------------------------------
void RecordingDialog::on_logLocationChoice_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "选择系统日志导出位置", ui->outPutLocationLog->text(), "文本文件 (*.txt);;日志文件 (*.log)");
    if (!filePath.isEmpty()) {
        ui->outPutLocationLog->setText(filePath);
    }
}

void RecordingDialog::on_dataLocationChoice_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "选择生理状态数据导出位置", ui->outPutLocationData->text(), "CSV 表格文件 (*.csv)");
    if(!filePath.isEmpty()){
        ui->outPutLocationData->setText(filePath);
    }
}

void RecordingDialog::on_startDate_dateChanged(const QDate& date)
{
    if (date > ui->endDate->date()) {
        ui->endDate->setDate(date);
    }
}

void RecordingDialog::on_endDate_dateChanged(const QDate& date)
{
    if (date < ui->startDate->date()) {
        ui->startDate->setDate(date);
    }
}

// ----------------------------------------------------
// 导出系统日志 (合并所选日期内的 .log)
// ----------------------------------------------------
void RecordingDialog::on_outPutLog_clicked()
{
    QString targetPath = ui->outPutLocationLog->text().trimmed();
    if (targetPath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请指定日志导出文件路径！");
        qDebug() << "未指定日志导出路径";
        return;
    }

    QFile outFile(targetPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法创建目标文件，请检查写入权限！");
        qDebug() << "导出日志失败";
        return;
    }

    QTextStream out(&outFile);
    out << "// ==========================================================\n";
    out << "// 心电监护仪系统日志导出文件\n";
    out << "// 导出范围: " << ui->startDate->date().toString("yyyy-MM-dd")
        << " 至 " << ui->endDate->date().toString("yyyy-MM-dd") << "\n";
    out << "// 导出时间: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    out << "// ==========================================================\n\n";

    QString logDir = ConfigManager::instance()->getLogPath();
    int mergedDays = 0;

    //遍历所选日期范围
    for (QDate d = ui->startDate->date(); d <= ui->endDate->date(); d = d.addDays(1)) {
        QString fileName = logDir + d.toString("yyyy-MM-dd") + ".log";
        QFile logFile(fileName);
        if (logFile.exists() && logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            out << QString("----- 日志文件 [%1.log] 开始 -----\n").arg(d.toString("yyyy-MM-dd"));
            out << logFile.readAll();
            out << QString("\n----- 日志文件 [%1.log] 结束 -----\n\n").arg(d.toString("yyyy-MM-dd"));
            logFile.close();
            mergedDays++;
        }
    }

    outFile.close();

    if (mergedDays > 0) {
        QMessageBox::information(this, "成功", QString("日志导出成功！共合并了 %1 天的系统运行日志。").arg(mergedDays));
        qDebug() << QString("日志导出成功！共合并了 %1 天的系统运行日志。").arg(mergedDays);
    }
    else {
        QMessageBox::warning(this, "提示", "所选日期范围内暂无系统运行日志文件！");
        qDebug() << "未找到系统运行日志文件！";
    }
}

// ----------------------------------------------------
// 导出过去 24 小时生理状态数据 (转 CSV)
// ----------------------------------------------------
void RecordingDialog::on_outPutData_clicked()
{
    QString targetPath = ui->outPutLocationData->text().trimmed();
    if (targetPath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请指定生理数据导出文件路径！");
        qDebug() << "未指定生理数据导出路径";
        return;
    }

    QFile csvFile(targetPath);
    if (!csvFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法创建 CSV 文件，请检查路径权限！");
        qDebug() << "创建 CSV 文件失败";
        return;
    }

    QTextStream out(&csvFile);
//写入 UTF-8 BOM 头，防止 Excel 打开乱码
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#else
    out.setCodec("UTF-8");
#endif
    out.setGenerateByteOrderMark(true);

    //写入 CSV 标准表头
    out << "记录时间,床号,姓名,心率(bpm),血氧(%),呼吸率(rpm),体温(℃),收缩压(mmHg),舒张压(mmHg)\n";

    //连接本地 SQLite 读取过去 24 小时数据
    QString dbPath = ConfigManager::instance()->getDatabasePath() + "medical_data.db";

    //使用独立连接名避免多线程冲突
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "export_conn");
    db.setDatabaseName(dbPath);

    int rowCount = 0;
    if (db.open()) {
        QSqlQuery query(db);
        //查询过去 24 小时内的数据
        query.prepare("SELECT timestamp, bed_no, patient_name, hr, spo2, resp, temp, sys, dia "
            "FROM VitalSigns "
            "WHERE timestamp >= datetime('now', '-24 hours', 'localtime') "
            "ORDER BY timestamp ASC");

        if (query.exec()) {
            while (query.next()) {
                QString timestampStr = query.value(0).toString() + "\t";//将时间戳强制转化为纯文本，避免乱码

                out << timestampStr << ","
                    << query.value(1).toInt() << ","
                    << query.value(2).toString() << ","
                    << query.value(3).toInt() << ","
                    << query.value(4).toInt() << ","
                    << query.value(5).toInt() << ","
                    << QString::number(query.value(6).toDouble(), 'f', 1) << ","
                    << (query.value(7).toInt() > 0 ? QString::number(query.value(7).toInt()) : "--") << ","
                    << (query.value(8).toInt() > 0 ? QString::number(query.value(8).toInt()) : "--") << "\n";
                rowCount++;
            }
        }
        db.close();
    }
    QSqlDatabase::removeDatabase("export_conn");
    csvFile.close();

    if (rowCount > 0) {
        QMessageBox::information(this, "导出完成", QString("成功导出最近 24 小时的生理数据！\n共计: %1 条记录。").arg(rowCount));
        qDebug() << QString("成功导出最近 24 小时的生理数据！\n共计: %1 条记录。").arg(rowCount);
    }
    else {
        //如果数据库尚未建好或无数据，给出提示
        QMessageBox::information(this, "提示", "CSV 表头已生成！\n当前本地数据库暂无最近 24 小时的生理体征记录。");
        qDebug() << "当前数据库无 24 小时内的数据";
    }
}