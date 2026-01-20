#include <QGuiApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include "Config.h"

using namespace ObservatoryMonitor;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
    // Set application metadata for QStandardPaths
    // Using only ApplicationName, no OrganizationName to keep path simple
    QCoreApplication::setApplicationName("observatory-monitor");
    
    qDebug() << "Observatory Monitor starting...";
    qDebug() << "Qt version:" << QT_VERSION_STR;
    
    // Determine config file path
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configPath = configDir + "/config.yaml";
    
    qDebug() << "Config directory:" << configDir;
    qDebug() << "Config file:" << configPath;
    
    // Create config directory if it doesn't exist
    QDir dir;
    if (!dir.exists(configDir)) {
        qDebug() << "Creating config directory...";
        if (!dir.mkpath(configDir)) {
            qCritical() << "Failed to create config directory:" << configDir;
            return 1;
        }
    }
    
    // Load or create configuration
    Config config;
    QString errorMessage;
    
    QFile configFile(configPath);
    if (!configFile.exists()) {
        qDebug() << "Config file does not exist, creating default...";
        config.setDefaults();
        
        if (!config.saveToFile(configPath, errorMessage)) {
            qCritical() << "Failed to create default config file:";
            qCritical().noquote() << errorMessage;
            return 1;
        }
        
        qDebug() << "Default config file created at:" << configPath;
    } else {
        qDebug() << "Loading config file...";
        if (!config.loadFromFile(configPath, errorMessage)) {
            qCritical() << "Failed to load config file:";
            qCritical().noquote() << errorMessage;
            return 1;
        }
        
        qDebug() << "Config file loaded successfully";
    }
    
    // Validate configuration
    if (!config.validate(errorMessage)) {
        qCritical() << "Configuration validation failed:";
        qCritical().noquote() << errorMessage;
        qCritical() << "\nPlease fix the configuration file at:" << configPath;
        return 1;
    }
    
    qDebug() << "Configuration validated successfully";
    
    // Display loaded configuration
    qDebug() << "\n=== Configuration ===";
    qDebug() << "MQTT Broker:" << config.broker().host << ":" << config.broker().port;
    qDebug() << "MQTT Timeout:" << config.mqttTimeout() << "seconds";
    qDebug() << "Reconnect Interval:" << config.reconnectInterval() << "seconds";
    qDebug() << "\nControllers:" << config.controllers().size();
    for (const auto& ctrl : config.controllers()) {
        qDebug() << "  -" << ctrl.name << "(" << ctrl.type << ")"
                 << "prefix:" << ctrl.prefix
                 << "enabled:" << ctrl.enabled;
    }
    qDebug() << "\nEquipment Types:" << config.equipmentTypes().size();
    for (const auto& type : config.equipmentTypes()) {
        qDebug() << "  -" << type.name << "controllers:" << type.controllers;
    }
    qDebug() << "====================\n";
    
    qDebug() << "Phase 1: Configuration system - OK";
    
    return 0; // Exit for now, GUI in Phase 7
}
