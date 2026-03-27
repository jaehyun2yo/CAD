#pragma once

#include <QString>
#include <QFile>
#include <QMutex>

namespace yjcad {

/// Application logger — writes Qt messages to file + console
/// Logs stored in %APPDATA%/YJ-CAD/logs/
class Logger {
public:
    /// Initialize logging (call once at app start)
    static void install();

    /// Get the current log file path
    static QString logFilePath();

private:
    static void messageHandler(QtMsgType type, const QMessageLogContext& context,
                               const QString& msg);

    static QFile s_logFile;
    static QMutex s_mutex;
    static bool s_initialized;
};

} // namespace yjcad
