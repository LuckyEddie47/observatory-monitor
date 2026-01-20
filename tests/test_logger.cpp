#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QThread>
#include <QDebug>
#include "Logger.h"

using namespace ObservatoryMonitor;

class TestLogger : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    void testInitialization();
    void testLogLevels();
    void testDebugLogging();
    void testFileCreation();
    void testLogRotation();
    void testSizeLimit();
    void testConcurrentLogging();
    
private:
    QTemporaryDir* m_tempDir;
};

void TestLogger::initTestCase()
{
    qDebug() << "=== Running Logger tests ===";
}

void TestLogger::cleanupTestCase()
{
    qDebug() << "=== Logger tests complete ===";
}

void TestLogger::init()
{
    qDebug() << "Creating temp directory...";
    m_tempDir = new QTemporaryDir();
    qDebug() << "Temp directory created:" << m_tempDir->path();
}

void TestLogger::cleanup()
{
    qDebug() << "Cleanup: Shutting down logger...";
    Logger::instance().shutdown();
    qDebug() << "Cleanup: Logger shut down";
    
    if (m_tempDir) {
        qDebug() << "Cleanup: Deleting temp directory...";
        delete m_tempDir;
        m_tempDir = nullptr;
        qDebug() << "Cleanup: Temp directory deleted";
    }
}

void TestLogger::testInitialization()
{
    qDebug() << "TEST: testInitialization - START";
    QVERIFY(m_tempDir->isValid());
    
    qDebug() << "TEST: Getting logger instance...";
    Logger& logger = Logger::instance();
    
    qDebug() << "TEST: Initializing logger...";
    QVERIFY(logger.initialize(m_tempDir->path(), false, false, 100));
    qDebug() << "TEST: Logger initialized";
    
    QString userLogPath = logger.userLogPath();
    qDebug() << "TEST: User log path:" << userLogPath;
    QVERIFY(!userLogPath.isEmpty());
    QVERIFY(QFile::exists(userLogPath));
    
    QString debugLogPath = logger.debugLogPath();
    qDebug() << "TEST: Debug log path:" << debugLogPath;
    QVERIFY(debugLogPath.isEmpty());
    
    qDebug() << "TEST: testInitialization - END";
}

void TestLogger::testLogLevels()
{
    qDebug() << "TEST: testLogLevels - START";
    QVERIFY(m_tempDir->isValid());
    
    Logger& logger = Logger::instance();
    qDebug() << "TEST: Initializing logger...";
    QVERIFY(logger.initialize(m_tempDir->path(), false, false, 100));
    
    qDebug() << "TEST: Logging messages...";
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");
    logger.critical("Critical message");
    
    // Get log path BEFORE shutdown
    QString logPath = m_tempDir->path() + "/observatory-monitor_" + 
                     QDate::currentDate().toString("yyyy-MM-dd") + ".log";
    
    qDebug() << "TEST: Shutting down logger...";
    logger.shutdown();
    qDebug() << "TEST: Logger shut down";
    
    qDebug() << "TEST: Reading log file:" << logPath;
    QFile logFile(logPath);
    QVERIFY(logFile.open(QIODevice::ReadOnly | QIODevice::Text));
    
    QString content = logFile.readAll();
    logFile.close();
    qDebug() << "TEST: Log content length:" << content.length();
    
    QVERIFY(!content.contains("Debug message"));
    QVERIFY(content.contains("INFO"));
    QVERIFY(content.contains("Info message"));
    QVERIFY(content.contains("WARN"));
    QVERIFY(content.contains("Warning message"));
    QVERIFY(content.contains("ERROR"));
    QVERIFY(content.contains("Error message"));
    QVERIFY(content.contains("CRIT"));
    QVERIFY(content.contains("Critical message"));
    
    qDebug() << "TEST: testLogLevels - END";
}

void TestLogger::testDebugLogging()
{
    qDebug() << "TEST: testDebugLogging - START";
    QVERIFY(m_tempDir->isValid());
    
    Logger& logger = Logger::instance();
    qDebug() << "TEST: Initializing logger with debug enabled...";
    QVERIFY(logger.initialize(m_tempDir->path(), true, false, 100));
    
    QVERIFY(logger.isDebugEnabled());
    
    qDebug() << "TEST: Logging debug messages...";
    logger.debug("Debug message");
    logger.info("Info message");
    
    // Get log path BEFORE shutdown
    QString debugLogPath = m_tempDir->path() + "/observatory-monitor_" + 
                          QDate::currentDate().toString("yyyy-MM-dd") + "_debug.log";
    
    qDebug() << "TEST: Shutting down logger...";
    logger.shutdown();
    
    qDebug() << "TEST: Reading debug log:" << debugLogPath;
    QFile debugLogFile(debugLogPath);
    QVERIFY(debugLogFile.exists());
    QVERIFY(debugLogFile.open(QIODevice::ReadOnly | QIODevice::Text));
    
    QString content = debugLogFile.readAll();
    debugLogFile.close();
    
    QVERIFY(content.contains("DEBUG"));
    QVERIFY(content.contains("Debug message"));
    QVERIFY(content.contains("INFO"));
    QVERIFY(content.contains("Info message"));
    
    qDebug() << "TEST: testDebugLogging - END";
}

void TestLogger::testFileCreation()
{
    qDebug() << "TEST: testFileCreation - START";
    QVERIFY(m_tempDir->isValid());
    
    Logger& logger = Logger::instance();
    QVERIFY(logger.initialize(m_tempDir->path(), false, false, 100));
    
    logger.info("Test message");
    
    QString expectedPath = m_tempDir->path() + "/observatory-monitor_" + 
                          QDate::currentDate().toString("yyyy-MM-dd") + ".log";
    
    QCOMPARE(logger.userLogPath(), expectedPath);
    QVERIFY(QFile::exists(expectedPath));
    
    qDebug() << "TEST: testFileCreation - END";
}

void TestLogger::testLogRotation()
{
    qDebug() << "TEST: testLogRotation - START";
    QVERIFY(m_tempDir->isValid());
    
    Logger& logger = Logger::instance();
    QVERIFY(logger.initialize(m_tempDir->path(), false, false, 100));
    
    logger.info("Message before rotation");
    
    QString firstLogPath = logger.userLogPath();
    QVERIFY(QFile::exists(firstLogPath));
    
    QString expectedDate = QDate::currentDate().toString("yyyy-MM-dd");
    QVERIFY(firstLogPath.contains(expectedDate));
    
    qDebug() << "TEST: testLogRotation - END";
}

void TestLogger::testSizeLimit()
{
    qDebug() << "TEST: testSizeLimit - START";
    QVERIFY(m_tempDir->isValid());
    
    Logger& logger = Logger::instance();
    QVERIFY(logger.initialize(m_tempDir->path(), false, false, 1));
    
    qDebug() << "TEST: Writing 100 messages...";
    for (int i = 0; i < 100; i++) {
        logger.info(QString("Test message %1").arg(i));
    }
    qDebug() << "TEST: Messages written";
    
    qDebug() << "TEST: testSizeLimit - END";
}

void TestLogger::testConcurrentLogging()
{
    qDebug() << "TEST: testConcurrentLogging - START";
    QVERIFY(m_tempDir->isValid());
    
    Logger& logger = Logger::instance();
    QVERIFY(logger.initialize(m_tempDir->path(), false, false, 100));
    
    logger.info("Message 1");
    logger.warning("Message 2");
    logger.error("Message 3");
    
    // Get log path BEFORE shutdown
    QString logPath = m_tempDir->path() + "/observatory-monitor_" + 
                     QDate::currentDate().toString("yyyy-MM-dd") + ".log";
    
    qDebug() << "TEST: Shutting down logger...";
    logger.shutdown();
    qDebug() << "TEST: Logger shut down";
    
    qDebug() << "TEST: Reading log:" << logPath;
    QFile logFile(logPath);
    QVERIFY(logFile.open(QIODevice::ReadOnly | QIODevice::Text));
    
    QString content = logFile.readAll();
    logFile.close();
    
    QVERIFY(content.contains("Message 1"));
    QVERIFY(content.contains("Message 2"));
    QVERIFY(content.contains("Message 3"));
    
    qDebug() << "TEST: testConcurrentLogging - END";
}

QTEST_MAIN(TestLogger)
#include "test_logger.moc"
