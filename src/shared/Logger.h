#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <QtMessageHandler>

namespace ObservatoryMonitor {

// Log levels
enum class LogLevel {
    Debug,    // Verbose debug information (only when debug logging enabled)
    Info,     // General information
    Warning,  // Warning messages
    Error,    // Error messages
    Critical  // Critical errors
};

// Logger class - singleton
class Logger {
public:
    // Get singleton instance
    static Logger& instance();
    
    // Initialize logger
    // logDir: directory for log files
    // enableDebug: enable debug logging to file
    // enableConsole: enable console output
    // maxTotalSizeMB: maximum total size of all log files in MB
    bool initialize(const QString& logDir, 
                   bool enableDebug = false,
                   bool enableConsole = true,
                   int maxTotalSizeMB = 100);
    
    // Shutdown logger (flush and close files)
    void shutdown();
    
    // Log a message
    void log(LogLevel level, const QString& message);
    void log(LogLevel level, const QString& category, const QString& message);
    
    // Convenience methods
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);
    void critical(const QString& message);
    
    // Enable/disable debug logging at runtime
    void setDebugEnabled(bool enabled);
    bool isDebugEnabled() const { return m_debugEnabled; }
    
    // Get current log file paths
    QString userLogPath() const;
    QString debugLogPath() const;
    
    // Qt message handler
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    
private:
    Logger();
    ~Logger();
    
    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Helper methods
    bool openLogFiles();
    void closeLogFiles();
    void rotateLogsIfNeeded();
    void enforceMaxTotalSize();
    QString formatMessage(LogLevel level, const QString& message);
    QString levelToString(LogLevel level);
    QList<QString> getLogFiles(const QString& pattern);
    qint64 getTotalLogSize();
    
    // Member variables
    QString m_logDir;
    bool m_debugEnabled;
    bool m_consoleEnabled;
    int m_maxTotalSizeMB;
    
    QFile m_userLogFile;
    QFile m_debugLogFile;
    QTextStream m_userLogStream;
    QTextStream m_debugLogStream;
    
    QDate m_currentDate;
    QMutex m_mutex;
    bool m_initialized;
};

} // namespace ObservatoryMonitor

#endif // LOGGER_H
