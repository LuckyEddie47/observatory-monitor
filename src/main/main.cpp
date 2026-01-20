#include <QGuiApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <iostream>
#include <unistd.h>
#include "Config.h"
#include "Logger.h"

using namespace ObservatoryMonitor;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    // Set application metadata for QStandardPaths
    QCoreApplication::setApplicationName("observatory-monitor");
    
    // Note: Can't use logger yet as it needs config to initialize
    std::cout << "Observatory Monitor starting..." << std::endl;
    std::cout << "Qt version: " << QT_VERSION_STR << std::endl;
    
    // Determine config file path
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configPath = configDir + "/config.yaml";
    
    // Determine log directory - Linux standard /var/log
    // If we don't have write permissions, fall back to user's home
    QString logDir;
    if (getuid() == 0) {
        // Running as root, use system log directory
        logDir = "/var/log/observatory-monitor";
    } else {
        // Not root, check if /var/log/observatory-monitor exists and is writable
        QDir varLogDir("/var/log/observatory-monitor");
        QFileInfo varLogInfo("/var/log/observatory-monitor");
        
        if (varLogDir.exists() && varLogInfo.isWritable()) {
            logDir = "/var/log/observatory-monitor";
        } else {
            // Fall back to user's home directory
            logDir = QDir::homePath() + "/.local/share/observatory-monitor/logs";
        }
    }
    
    std::cout << "Config directory: " << configDir.toStdString() << std::endl;
    std::cout << "Config file: " << configPath.toStdString() << std::endl;
    std::cout << "Log directory: " << logDir.toStdString() << std::endl;
    
    // Create log directory if it doesn't exist
    QDir logDirObj;
    if (!logDirObj.exists(logDir)) {
        if (!logDirObj.mkpath(logDir)) {
            std::cerr << "Failed to create log directory: " << logDir.toStdString() << std::endl;
            std::cerr << "Note: For system-wide logging, run: sudo mkdir -p /var/log/observatory-monitor && sudo chown $USER /var/log/observatory-monitor" << std::endl;
            return 1;
        }
    }
    
    // Create config directory if it doesn't exist
    QDir dir;
    if (!dir.exists(configDir)) {
        std::cout << "Creating config directory..." << std::endl;
        if (!dir.mkpath(configDir)) {
            std::cerr << "Failed to create config directory: " << configDir.toStdString() << std::endl;
            return 1;
        }
    }
    
    // Load or create configuration
    Config config;
    QString errorMessage;
    
    QFile configFile(configPath);
    if (!configFile.exists()) {
        std::cout << "Config file does not exist, creating default..." << std::endl;
        config.setDefaults();
        
        if (!config.saveToFile(configPath, errorMessage)) {
            std::cerr << "Failed to create default config file:" << std::endl;
            std::cerr << errorMessage.toStdString() << std::endl;
            return 1;
        }
        
        std::cout << "Default config file created at: " << configPath.toStdString() << std::endl;
    } else {
        std::cout << "Loading config file..." << std::endl;
        if (!config.loadFromFile(configPath, errorMessage)) {
            std::cerr << "Failed to load config file:" << std::endl;
            std::cerr << errorMessage.toStdString() << std::endl;
            return 1;
        }
        
        std::cout << "Config file loaded successfully" << std::endl;
    }
    
    // Validate configuration
    if (!config.validate(errorMessage)) {
        std::cerr << "Configuration validation failed:" << std::endl;
        std::cerr << errorMessage.toStdString() << std::endl;
        std::cerr << "\nPlease fix the configuration file at: " << configPath.toStdString() << std::endl;
        return 1;
    }
    
    std::cout << "Configuration validated successfully" << std::endl;
    
    // Initialize logger now that we have configuration
    Logger& logger = Logger::instance();
    if (!logger.initialize(logDir, 
                          config.logging().debugEnabled,
                          true,  // Enable console output
                          config.logging().maxTotalSizeMB)) {
        std::cerr << "Failed to initialize logger" << std::endl;
        return 1;
    }
    
    // From this point forward, use logger instead of qDebug/qCritical
    logger.info("=================================================");
    logger.info("Observatory Monitor initialized");
    logger.info(QString("Qt version: %1").arg(QT_VERSION_STR));
    logger.info("=================================================");
    
    // Display loaded configuration
    logger.info("");
    logger.info("=== Configuration ===");
    logger.info(QString("MQTT Broker: %1:%2").arg(config.broker().host).arg(config.broker().port));
    logger.info(QString("MQTT Timeout: %1 seconds").arg(config.mqttTimeout()));
    logger.info(QString("Reconnect Interval: %1 seconds").arg(config.reconnectInterval()));
    logger.info("");
    logger.info(QString("Controllers: %1").arg(config.controllers().size()));
    for (const auto& ctrl : config.controllers()) {
        logger.info(QString("  - %1 (%2) prefix: %3 enabled: %4")
                   .arg(ctrl.name)
                   .arg(ctrl.type)
                   .arg(ctrl.prefix)
                   .arg(ctrl.enabled ? "true" : "false"));
    }
    logger.info("");
    logger.info(QString("Equipment Types: %1").arg(config.equipmentTypes().size()));
    for (const auto& type : config.equipmentTypes()) {
        logger.info(QString("  - %1 controllers: %2")
                   .arg(type.name)
                   .arg(type.controllers.join(", ")));
    }
    logger.info("");
    logger.info(QString("Logging: debug_enabled=%1, max_size=%2 MB")
               .arg(config.logging().debugEnabled ? "true" : "false")
               .arg(config.logging().maxTotalSizeMB));
    logger.info("====================");
    logger.info("");
    
    logger.info("Phase 2: Logging system - OK");
    
    // Test different log levels
    logger.debug("This is a debug message (only visible if debug enabled)");
    logger.info("This is an info message");
    logger.warning("This is a warning message");
    logger.error("This is an error message");
    
    // Shutdown logger before exit
    logger.shutdown();
    
    return 0; // Exit for now, GUI in Phase 7
}
