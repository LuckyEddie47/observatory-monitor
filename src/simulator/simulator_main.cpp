#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QDebug>
#include "SimulatorConfig.h"

using namespace ObservatoryMonitor;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Set application metadata
    QCoreApplication::setApplicationName("mqtt-simulator");
    QCoreApplication::setApplicationVersion("0.1.0");
    
    qDebug() << "MQTT Simulator starting...";
    qDebug() << "Qt version:" << QT_VERSION_STR;
    
    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("Observatory Monitor MQTT Simulator");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption configOption(QStringList() << "c" << "config",
                                     "Path to simulator configuration file",
                                     "config");
    parser.addOption(configOption);
    
    parser.process(app);
    
    // Check if config file was specified
    if (!parser.isSet(configOption)) {
        qCritical() << "Error: Configuration file not specified";
        qCritical() << "Usage: mqtt-simulator --config <path/to/simulator.yaml>";
        qCritical() << "\nExample config file location:";
        qCritical() << "  ~/Data/Code/observatory-monitor/config/simulator.yaml.example";
        return 1;
    }
    
    QString configPath = parser.value(configOption);
    qDebug() << "Config file:" << configPath;
    
    // Check if file exists
    QFile configFile(configPath);
    if (!configFile.exists()) {
        qCritical() << "Error: Config file does not exist:" << configPath;
        qCritical() << "\nTo create an example config file, copy:";
        qCritical() << "  cp ~/Data/Code/observatory-monitor/config/simulator.yaml.example" << configPath;
        return 1;
    }
    
    // Load configuration
    SimulatorConfig config;
    QString errorMessage;
    
    qDebug() << "Loading simulator config...";
    if (!config.loadFromFile(configPath, errorMessage)) {
        qCritical() << "Failed to load simulator config:";
        qCritical().noquote() << errorMessage;
        return 1;
    }
    
    qDebug() << "Simulator config loaded successfully";
    
    // Validate configuration
    if (!config.validate(errorMessage)) {
        qCritical() << "Simulator configuration validation failed:";
        qCritical().noquote() << errorMessage;
        qCritical() << "\nPlease fix the configuration file at:" << configPath;
        return 1;
    }
    
    qDebug() << "Simulator configuration validated successfully";
    
    // Display loaded configuration
    qDebug() << "\n=== Simulator Configuration ===";
    qDebug() << "MQTT Broker:" << config.broker().host << ":" << config.broker().port;
    qDebug() << "\nSimulated Controllers:" << config.controllers().size();
    for (const auto& ctrl : config.controllers()) {
        qDebug() << "  - Prefix:" << ctrl.prefix;
        qDebug() << "    Commands:" << ctrl.commands.size();
        for (const auto& cmd : ctrl.commands) {
            qDebug() << "      *" << cmd.command << "->" 
                     << (cmd.response.isEmpty() ? "(no response)" : cmd.response.left(50) + "...");
        }
    }
    qDebug() << "===============================\n";
    
    qDebug() << "Phase 1: Configuration system - OK";
    
    return 0; // Exit for now, MQTT logic in Phase 3
}
