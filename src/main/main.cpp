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
    
    // Test MQTT client with first enabled controller
    logger.info("Phase 3: MQTT Core - Testing");
    logger.info("");
    
    if (config.controllers().isEmpty()) {
        logger.error("No controllers configured");
    } else {
        // Get first enabled controller
        ControllerConfig ctrl = config.controllers().first();
        
        logger.info(QString("Testing MQTT with controller: %1").arg(ctrl.name));
        
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
        
        // Track connection state
        bool connectionEstablished = false;
        
        // Connect signals
        QObject::connect(&mqttClient, &MqttClient::connected, [&logger, &connectionEstablished]() {
            logger.info("MQTT: Connection established");
            connectionEstablished = true;
        });
        
        QObject::connect(&mqttClient, &MqttClient::disconnected, [&logger]() {
            logger.warning("MQTT: Connection lost");
        });
        
        QObject::connect(&mqttClient, &MqttClient::errorOccurred, [&logger](const QString& error) {
            logger.error(QString("MQTT: %1").arg(error));
        });
        
        // Connect to broker
        mqttClient.connectToHost();
        
        // Wait for connection with event loop
        QEventLoop connectionLoop;
        QTimer connectionTimeout;
        connectionTimeout.setSingleShot(true);
        connectionTimeout.setInterval(5000);  // 5 second timeout
        
        QObject::connect(&mqttClient, &MqttClient::connected, &connectionLoop, &QEventLoop::quit);
        QObject::connect(&connectionTimeout, &QTimer::timeout, &connectionLoop, &QEventLoop::quit);
        
        connectionTimeout.start();
        connectionLoop.exec();
        
        if (connectionEstablished) {
            logger.info("Sending test command: :DZ#");
            
            // Send command and wait for response
            bool responseReceived = false;
            QEventLoop responseLoop;
            QTimer responseTimeout;
            responseTimeout.setSingleShot(true);
            responseTimeout.setInterval(config.mqttTimeout() * 1000 + 1000);  // Command timeout + 1 second
            
            mqttClient.sendCommand(":DZ#", [&logger, &responseReceived, &responseLoop](const QString& cmd, const QString& response, bool success) {
                if (success) {
                    logger.info(QString("Command '%1' succeeded: %2").arg(cmd, response));
                } else {
                    logger.error(QString("Command '%1' failed").arg(cmd));
                }
                responseReceived = true;
                responseLoop.quit();
            });
            
            QObject::connect(&responseTimeout, &QTimer::timeout, &responseLoop, &QEventLoop::quit);
            responseTimeout.start();
            responseLoop.exec();
            
            if (!responseReceived) {
                logger.warning("Command response timed out");
            }
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
    logger.info("Phase 3: MQTT Core - OK");
    
    // Shutdown logger before exit
    logger.shutdown();
    
    return result;
}
