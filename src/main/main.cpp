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
#include "MqttClient.h"
#include "ControllerPoller.h"

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
    
    // Test polling engine with first enabled controller
    logger.info("Phase 5: Polling Engine - Testing");
    logger.info("");
    
    if (config.controllers().isEmpty()) {
        logger.error("No controllers configured");
    } else {
        // Get first enabled controller
        ControllerConfig ctrl = config.controllers().first();
        
        logger.info(QString("Testing polling engine with controller: %1").arg(ctrl.name));
        
        // Create MQTT client
        MqttClient mqttClient;
        mqttClient.setHostname(config.broker().host);
        mqttClient.setPort(config.broker().port);
        if (!config.broker().username.isEmpty()) {
            mqttClient.setUsername(config.broker().username);
            mqttClient.setPassword(config.broker().password);
        }
        mqttClient.setTopicPrefix(ctrl.prefix);
        mqttClient.setCommandTimeout(config.mqttTimeout() * 1000);  // Convert to ms
        mqttClient.setReconnectInterval(config.reconnectInterval() * 1000);
        
        // Create controller poller
        ControllerPoller poller(&mqttClient);
        poller.setControllerName(ctrl.name);
        poller.setFastPollInterval(1000);   // 1 second for testing (normally from config)
        poller.setSlowPollInterval(5000);   // 5 seconds for testing (normally 10s)
        poller.setStaleDataMultiplier(3);   // Data stale after 3x poll interval
        
        // Track connection state
        bool connectionEstablished = false;
        int dataUpdateCount = 0;
        
        // Connect signals
        QObject::connect(&mqttClient, &MqttClient::connected, [&logger, &connectionEstablished, &poller]() {
            logger.info("MQTT: Connection established");
            connectionEstablished = true;
        });
        
        QObject::connect(&mqttClient, &MqttClient::disconnected, [&logger]() {
            logger.warning("MQTT: Connection lost");
        });
        
        QObject::connect(&mqttClient, &MqttClient::errorOccurred, [&logger](const QString& error) {
            logger.error(QString("MQTT: %1").arg(error));
        });
        
        QObject::connect(&poller, &ControllerPoller::dataUpdated, [&logger, &dataUpdateCount](const QString& command, const QString& value) {
            logger.info(QString("📊 Data updated: %1 = %2").arg(command, value));
            dataUpdateCount++;
        });
        
        QObject::connect(&poller, &ControllerPoller::dataStale, [&logger](const QString& command) {
            logger.warning(QString("⚠ Data stale: %1").arg(command));
        });
        
        QObject::connect(&poller, &ControllerPoller::pollError, [&logger](const QString& command, const QString& error) {
            logger.error(QString("✗ Poll error for %1: %2").arg(command, error));
        });
        
        // Connect to broker
        mqttClient.connectToHost();
        
        // Wait for connection
        QEventLoop connectionLoop;
        QTimer connectionTimeout;
        connectionTimeout.setSingleShot(true);
        connectionTimeout.setInterval(5000);
        
        QObject::connect(&mqttClient, &MqttClient::connected, &connectionLoop, &QEventLoop::quit);
        QObject::connect(&connectionTimeout, &QTimer::timeout, &connectionLoop, &QEventLoop::quit);
        
        connectionTimeout.start();
        connectionLoop.exec();
        
        if (connectionEstablished) {
            logger.info("");
            logger.info("Starting polling engine...");
            logger.info("Fast poll: 1s, Slow poll: 5s");
            logger.info("Will run for 15 seconds...");
            logger.info("");
            
            // Start polling
            poller.startPolling();
            
            // Run for 15 seconds to observe multiple poll cycles
            QTimer testDuration;
            testDuration.setSingleShot(true);
            testDuration.setInterval(15000);
            
            QEventLoop testLoop;
            QObject::connect(&testDuration, &QTimer::timeout, &testLoop, &QEventLoop::quit);
            
            testDuration.start();
            testLoop.exec();
            
            // Stop polling
            poller.stopPolling();
            
            logger.info("");
            logger.info("=== Polling Statistics ===");
            logger.info(QString("Data updates received: %1").arg(dataUpdateCount));
            logger.info(QString("Successful polls: %1").arg(poller.successfulPolls()));
            logger.info(QString("Failed polls: %1").arg(poller.failedPolls()));
            logger.info("");
            logger.info("=== Cached Values ===");
            auto cachedValues = poller.getAllCachedValues();
            for (auto it = cachedValues.begin(); it != cachedValues.end(); ++it) {
                QString staleStatus = poller.isDataStale(it.key()) ? " (STALE)" : " (FRESH)";
                logger.info(QString("%1 = %2 [%3]%4")
                           .arg(it.key())
                           .arg(it->value)
                           .arg(it->timestamp.toString("HH:mm:ss"))
                           .arg(staleStatus));
            }
            logger.info("==========================");
            
        } else {
            logger.error("Failed to connect to MQTT broker");
            logger.info("Ensure Mosquitto is running: sudo systemctl status mosquitto");
            logger.info("Ensure simulator is running with matching prefix");
        }
        
        // Disconnect
        mqttClient.disconnectFromHost();
        
        // Wait a moment for clean disconnect
        QTimer::singleShot(500, &app, &QGuiApplication::quit);
    }
    
    int result = app.exec();
    
    logger.info("");
    logger.info("Phase 5: Polling Engine - OK");
    
    // Shutdown logger before exit
    logger.shutdown();
    
    return result;
}
