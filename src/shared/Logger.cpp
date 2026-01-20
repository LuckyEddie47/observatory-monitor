#include "Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <iostream>

namespace ObservatoryMonitor {

Logger::Logger()
    : m_debugEnabled(false)
    , m_consoleEnabled(true)
    , m_maxTotalSizeMB(100)
    , m_initialized(false)
{
}

Logger::~Logger()
{
    shutdown();
}

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

bool Logger::initialize(const QString& logDir, bool enableDebug, bool enableConsole, int maxTotalSizeMB)
{
    {
        QMutexLocker locker(&m_mutex);
        
        if (m_initialized) {
            std::cerr << "Logger already initialized" << std::endl;
            return false;
        }
        
        m_logDir = logDir;
        m_debugEnabled = enableDebug;
        m_consoleEnabled = enableConsole;
        m_maxTotalSizeMB = maxTotalSizeMB;
        
        // Create log directory if it doesn't exist
        QDir dir;
        if (!dir.exists(m_logDir)) {
            if (!dir.mkpath(m_logDir)) {
                std::cerr << "Failed to create log directory: " 
                         << m_logDir.toStdString() << std::endl;
                return false;
            }
        }
        
        // Set current date for rotation checking
        m_currentDate = QDate::currentDate();
        
        // Open log files
        if (!openLogFiles()) {
            return false;
        }
        
        // Enforce size limits
        enforceMaxTotalSize();
        
        m_initialized = true;
    } // Release mutex before installing message handler
    
    // Install Qt message handler OUTSIDE the mutex lock
    // This prevents deadlock when Qt tries to log during handler installation
    qInstallMessageHandler(Logger::messageHandler);
    
    // Log initialization (now safe to use the logger)
    info("Logger initialized");
    if (m_debugEnabled) {
        debug("Debug logging enabled");
    }
    
    return true;
}

void Logger::shutdown()
{
    // Restore default Qt message handler FIRST, outside mutex
    qInstallMessageHandler(nullptr);
    
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    closeLogFiles();
    
    m_initialized = false;
}

bool Logger::openLogFiles()
{
    // Close any existing files
    closeLogFiles();
    
    // Generate filename with date
    QString dateStr = m_currentDate.toString("yyyy-MM-dd");
    
    // User log file
    QString userLogPath = m_logDir + "/observatory-monitor_" + dateStr + ".log";
    m_userLogFile.setFileName(userLogPath);
    if (!m_userLogFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        std::cerr << "Failed to open user log file: " 
                 << userLogPath.toStdString() << std::endl;
        return false;
    }
    m_userLogStream.setDevice(&m_userLogFile);
    
    // Debug log file (if enabled)
    if (m_debugEnabled) {
        QString debugLogPath = m_logDir + "/observatory-monitor_" + dateStr + "_debug.log";
        m_debugLogFile.setFileName(debugLogPath);
        if (!m_debugLogFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            std::cerr << "Failed to open debug log file: " 
                     << debugLogPath.toStdString() << std::endl;
            m_userLogFile.close();
            return false;
        }
        m_debugLogStream.setDevice(&m_debugLogFile);
    }
    
    return true;
}

void Logger::closeLogFiles()
{
    if (m_userLogFile.isOpen()) {
        m_userLogStream.flush();
        m_userLogFile.close();
    }
    
    if (m_debugLogFile.isOpen()) {
        m_debugLogStream.flush();
        m_debugLogFile.close();
    }
}

void Logger::rotateLogsIfNeeded()
{
    QDate today = QDate::currentDate();
    if (today != m_currentDate) {
        // Date has changed, rotate logs
        log(LogLevel::Info, "Daily log rotation");
        closeLogFiles();
        m_currentDate = today;
        openLogFiles();
        log(LogLevel::Info, "Log rotation complete");
    }
}

void Logger::enforceMaxTotalSize()
{
    qint64 totalSize = getTotalLogSize();
    qint64 maxSize = static_cast<qint64>(m_maxTotalSizeMB) * 1024 * 1024;
    
    if (totalSize <= maxSize) {
        return;
    }
    
    // Get all log files sorted by modification time (oldest first)
    QList<QString> userLogs = getLogFiles("observatory-monitor_*[0-9].log");
    QList<QString> debugLogs = getLogFiles("observatory-monitor_*_debug.log");
    
    QList<QPair<QString, QDateTime>> allLogs;
    
    for (const QString& logPath : userLogs) {
        QFileInfo info(logPath);
        allLogs.append(qMakePair(logPath, info.lastModified()));
    }
    
    for (const QString& logPath : debugLogs) {
        QFileInfo info(logPath);
        allLogs.append(qMakePair(logPath, info.lastModified()));
    }
    
    // Sort by modification time (oldest first)
    std::sort(allLogs.begin(), allLogs.end(), 
              [](const QPair<QString, QDateTime>& a, const QPair<QString, QDateTime>& b) {
                  return a.second < b.second;
              });
    
    // Delete oldest files until under size limit
    int deletedCount = 0;
    for (const auto& logPair : allLogs) {
        if (totalSize <= maxSize) {
            break;
        }
        
        QString logPath = logPair.first;
        
        // Don't delete current log files
        if (logPath == userLogPath() || logPath == debugLogPath()) {
            continue;
        }
        
        QFileInfo info(logPath);
        qint64 fileSize = info.size();
        
        if (QFile::remove(logPath)) {
            totalSize -= fileSize;
            deletedCount++;
        }
    }
}

QString Logger::formatMessage(LogLevel level, const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString levelStr = levelToString(level);
    return QString("[%1] [%2] %3").arg(timestamp, levelStr, message);
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO ";
        case LogLevel::Warning:  return "WARN ";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRIT ";
        default:                 return "?????";
    }
}

QList<QString> Logger::getLogFiles(const QString& pattern)
{
    QDir dir(m_logDir);
    QStringList filters;
    filters << pattern;
    
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Time);
    
    QList<QString> result;
    for (const QFileInfo& info : fileList) {
        result.append(info.absoluteFilePath());
    }
    
    return result;
}

qint64 Logger::getTotalLogSize()
{
    QList<QString> userLogs = getLogFiles("observatory-monitor_*.log");
    QList<QString> debugLogs = getLogFiles("observatory-monitor_*_debug.log");
    
    qint64 total = 0;
    
    for (const QString& logPath : userLogs) {
        QFileInfo info(logPath);
        total += info.size();
    }
    
    for (const QString& logPath : debugLogs) {
        QFileInfo info(logPath);
        total += info.size();
    }
    
    return total;
}

void Logger::log(LogLevel level, const QString& message)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    // Check if we need to rotate logs
    rotateLogsIfNeeded();
    
    QString formattedMsg = formatMessage(level, message);
    
    // Console output
    if (m_consoleEnabled) {
        if (level == LogLevel::Debug && !m_debugEnabled) {
            // Don't output debug to console if debug disabled
        } else {
            std::cout << formattedMsg.toStdString() << std::endl;
        }
    }
    
    // Always write to user log (except debug messages)
    if (level != LogLevel::Debug) {
        m_userLogStream << formattedMsg << "\n";
        m_userLogStream.flush();
    }
    
    // Write to debug log if enabled
    if (m_debugEnabled && m_debugLogFile.isOpen()) {
        m_debugLogStream << formattedMsg << "\n";
        m_debugLogStream.flush();
    }
}

void Logger::log(LogLevel level, const QString& category, const QString& message)
{
    QString fullMessage = QString("[%1] %2").arg(category, message);
    log(level, fullMessage);
}

void Logger::debug(const QString& message)
{
    log(LogLevel::Debug, message);
}

void Logger::info(const QString& message)
{
    log(LogLevel::Info, message);
}

void Logger::warning(const QString& message)
{
    log(LogLevel::Warning, message);
}

void Logger::error(const QString& message)
{
    log(LogLevel::Error, message);
}

void Logger::critical(const QString& message)
{
    log(LogLevel::Critical, message);
}

void Logger::setDebugEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_debugEnabled == enabled) {
        return;
    }
    
    m_debugEnabled = enabled;
    
    if (m_initialized) {
        // Reopen log files to create/close debug log
        closeLogFiles();
        openLogFiles();
        
        if (enabled) {
            log(LogLevel::Info, "Debug logging enabled");
        } else {
            log(LogLevel::Info, "Debug logging disabled");
        }
    }
}

QString Logger::userLogPath() const
{
    if (!m_initialized) {
        return QString();
    }
    
    QString dateStr = m_currentDate.toString("yyyy-MM-dd");
    return m_logDir + "/observatory-monitor_" + dateStr + ".log";
}

QString Logger::debugLogPath() const
{
    if (!m_initialized || !m_debugEnabled) {
        return QString();
    }
    
    QString dateStr = m_currentDate.toString("yyyy-MM-dd");
    return m_logDir + "/observatory-monitor_" + dateStr + "_debug.log";
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Logger& logger = Logger::instance();
    
    // Prevent recursive logging - if logger is not initialized, just output to stderr
    if (!logger.m_initialized) {
        std::cerr << msg.toStdString() << std::endl;
        return;
    }
    
    // Convert Qt message type to our LogLevel
    LogLevel level;
    switch (type) {
        case QtDebugMsg:
            level = LogLevel::Debug;
            break;
        case QtInfoMsg:
            level = LogLevel::Info;
            break;
        case QtWarningMsg:
            level = LogLevel::Warning;
            break;
        case QtCriticalMsg:
            level = LogLevel::Error;
            break;
        case QtFatalMsg:
            level = LogLevel::Critical;
            break;
        default:
            level = LogLevel::Info;
            break;
    }
    
    // Format message with context if available and debug enabled
    QString fullMessage = msg;
    if (context.file && logger.isDebugEnabled()) {
        fullMessage = QString("%1 (%2:%3, %4)")
                      .arg(msg)
                      .arg(context.file)
                      .arg(context.line)
                      .arg(context.function);
    }
    
    logger.log(level, fullMessage);
    
    // For fatal messages, abort after logging
    if (type == QtFatalMsg) {
        abort();
    }
}

} // namespace ObservatoryMonitor
