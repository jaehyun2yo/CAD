#include "logger.h"
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QTextStream>
#include <iostream>

namespace yjcad {

QFile Logger::s_logFile;
QMutex Logger::s_mutex;
bool Logger::s_initialized = false;

void Logger::install() {
    if (s_initialized) return;

    // Create log directory: %APPDATA%/YJ-CAD/logs/
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir logDir(appDataPath + "/logs");
    if (!logDir.exists()) logDir.mkpath(".");

    // Log file: yjcad_YYYYMMDD_HHMMSS.log
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString logPath = logDir.filePath(QString("yjcad_%1.log").arg(timestamp));

    s_logFile.setFileName(logPath);
    if (s_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        s_initialized = true;
        qInstallMessageHandler(messageHandler);

        // Write header
        QTextStream out(&s_logFile);
        out << "=== YJ-CAD Log Start: "
            << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
            << " ===" << Qt::endl;
    }

    // Clean up old logs (keep last 10)
    QStringList logFiles = logDir.entryList({"yjcad_*.log"}, QDir::Files, QDir::Time);
    for (int i = 10; i < logFiles.size(); ++i) {
        logDir.remove(logFiles[i]);
    }
}

QString Logger::logFilePath() {
    return s_logFile.fileName();
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext& context,
                            const QString& msg) {
    QMutexLocker locker(&s_mutex);

    QString level;
    switch (type) {
    case QtDebugMsg:    level = "DEBUG"; break;
    case QtInfoMsg:     level = "INFO "; break;
    case QtWarningMsg:  level = "WARN "; break;
    case QtCriticalMsg: level = "ERROR"; break;
    case QtFatalMsg:    level = "FATAL"; break;
    }

    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString logLine = QString("[%1] %2: %3").arg(timestamp, level, msg);

    // Add source location for warnings and above
    if (type >= QtWarningMsg && context.file) {
        logLine += QString(" (%1:%2)").arg(context.file).arg(context.line);
    }

    // Write to file
    if (s_logFile.isOpen()) {
        QTextStream out(&s_logFile);
        out << logLine << Qt::endl;
        s_logFile.flush();
    }

    // Also write to stderr
    std::cerr << logLine.toStdString() << std::endl;

    if (type == QtFatalMsg) {
        abort();
    }
}

} // namespace yjcad
