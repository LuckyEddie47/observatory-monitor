#include <QGuiApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QEventLoop>
#include <iostream>
#include <unistd.h>
#include "Config.h"
#include "Logger.h"
#include "ControllerManager.h"

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
    
    // Test multi-controller support
    logger.info("Phase 6: Multi-Controller Support - Testing");
    logger.info("");
    
    if (config.controllers().isEmpty()) {
        logger.error("No controllers configured");
        return 1;
    }
    
    logger.info(QString("Testing ControllerManager with %1 controller(s)").arg(config.controllers().size()));
    
    // Create controller manager - MUST be declared here to live long enough for timers
    ControllerManager manager;
    
    // Track system status changes
    int statusChangeCount = 0;
    
    // Connect signals
    QObject::connect(&manager, &ControllerManager::systemStatusChanged, [&logger, &statusChangeCount](SystemStatus status) {
        QString statusStr;
        QString emoji;
        switch (status) {
            case SystemStatus::AllConnected:
                statusStr = "GREEN - All controllers connected";
                emoji = "🟢";
                break;
            case SystemStatus::PartiallyConnected:
                statusStr = "YELLOW - Partially connected";
                emoji = "🟡";
                break;
            case SystemStatus::Disconnected:
                statusStr = "RED - Disconnected";
                emoji = "🔴";
                break;
        }
        logger.info(QString("%1 System Status: %2").arg(emoji, statusStr));
        statusChangeCount++;
    });
    
    QObject::connect(&manager, &ControllerManager::controllerStatusChanged, [&logger](const QString& name, ControllerStatus status) {
        QString statusStr;
        switch (status) {
            case ControllerStatus::Disconnected: statusStr = "Disconnected"; break;
            case ControllerStatus::Connecting: statusStr = "Connecting"; break;
            case ControllerStatus::Connected: statusStr = "Connected"; break;
            case ControllerStatus::Error: statusStr = "Error"; break;
        }
        logger.info(QString("  Controller '%1': %2").arg(name, statusStr));
    });
    
    QObject::connect(&manager, &ControllerManager::controllerDataUpdated, [&logger](const QString& controllerName, const QString& command, const QString& value) {
        logger.debug(QString("  [%1] %2 = %3").arg(controllerName, command, value));
    });
    
    QObject::connect(&manager, &ControllerManager::controllerError, [&logger](const QString& controllerName, const QString& error) {
        logger.error(QString("  [%1] Error: %2").arg(controllerName, error));
    });
    
    // Load controllers from config
    manager.loadControllersFromConfig(config);
    
    logger.info("");
    logger.info("Connecting all controllers...");
    
    // Connect all
    manager.connectAll();
    
    // Wait for connections
    QTimer::singleShot(3000, [&]() {
        logger.info("");
        logger.info("=== Connection Status ===");
        logger.info(QString("Enabled controllers: %1").arg(manager.getEnabledControllerCount()));
        logger.info(QString("Connected controllers: %1").arg(manager.getConnectedControllerCount()));
        logger.info("");
        logger.info("Connected:");
        for (const QString& name : manager.getConnectedControllers()) {
            logger.info(QString("  ✓ %1").arg(name));
        }
        logger.info("");
        logger.info("Disconnected:");
        for (const QString& name : manager.getDisconnectedControllers()) {
            logger.info(QString("  ✗ %1").arg(name));
        }
        logger.info("========================");
        logger.info("");
        
        // Start polling if we have connected controllers
        if (manager.getConnectedControllerCount() > 0) {
            logger.info("Starting polling engine...");
            logger.info("Fast poll: 1s, Slow poll: 5s");
            logger.info("Will run for 15 seconds...");
            logger.info("");
            
            manager.startPolling(1000, 5000);
        } else {
            logger.warning("No controllers connected - skipping polling test");
            logger.info("Ensure Mosquitto is running: sudo systemctl status mosquitto");
            logger.info("Ensure simulator is running with matching prefix");
        }
    });
    
    // Stop polling and show statistics after 18 seconds (3s connect + 15s poll)
    QTimer::singleShot(18000, [&]() {
        manager.stopPolling();
        
        logger.info("");
        logger.info("=== Final Statistics ===");
        logger.info(QString("System status changes: %1").arg(statusChangeCount));
        logger.info("");
        
        for (const QString& name : manager.getControllerNames()) {
            logger.info(QString("Controller: %1").arg(name));
            
            auto values = manager.getAllControllerValues(name);
            if (values.isEmpty()) {
                logger.info("  No data cached");
            } else {
                logger.info("  Cached values:");
                for (auto it = values.begin(); it != values.end(); ++it) {
                    QString status = it->valid ? "VALID" : "INVALID";
                    logger.info(QString("    %1 = %2 [%3] (%4)")
                               .arg(it.key())
                               .arg(it->value)
                               .arg(it->timestamp.toString("HH:mm:ss"))
                               .arg(status));
                }
            }
            logger.info("");
        }
        logger.info("=======================");
        
        // Disconnect and exit
        manager.disconnectAll();
        QTimer::singleShot(500, &app, &QGuiApplication::quit);
    });
    
    int result = app.exec();
    
    logger.info("");
    logger.info("Phase 6: Multi-Controller Support - OK");
    
    // Shutdown logger before exit
    logger.shutdown();
    
    return result;
}
