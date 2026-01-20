#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <iostream>
#include "SimulatorConfig.h"
#include "Logger.h"

using namespace ObservatoryMonitor;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Set application metadata
    QCoreApplication::setApplicationName("mqtt-simulator");
    QCoreApplication::setApplicationVersion("0.1.0");
    
    std::cout << "MQTT Simulator starting..." << std::endl;
    std::cout << "Qt version: " << QT_VERSION_STR << std::endl;
    
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
        std::cerr << "Error: Configuration file not specified" << std::endl;
        std::cerr << "Usage: mqtt-simulator --config <path/to/simulator.yaml>" << std::endl;
        std::cerr << "\nExample config file location:" << std::endl;
        std::cerr << "  ~/Data/Code/observatory-monitor/config/simulator.yaml.example" << std::endl;
        return 1;
    }
    
    QString configPath = parser.value(configOption);
    std::cout << "Config file: " << configPath.toStdString() << std::endl;
    
    // Check if file exists
    QFile configFile(configPath);
    if (!configFile.exists()) {
        std::cerr << "Error: Config file does not exist: " << configPath.toStdString() << std::endl;
        std::cerr << "\nTo create an example config file, copy:" << std::endl;
        std::cerr << "  cp ~/Data/Code/observatory-monitor/config/simulator.yaml.example " 
                  << configPath.toStdString() << std::endl;
        return 1;
    }
    
    // Determine log directory
    QFileInfo configInfo(configPath);
    QString logDir = configInfo.absolutePath() + "/logs";
    
    std::cout << "Log directory: " << logDir.toStdString() << std::endl;
    
    // Create log directory if it doesn't exist
    QDir logDirObj;
    if (!logDirObj.exists(logDir)) {
        if (!logDirObj.mkpath(logDir)) {
            std::cerr << "Failed to create log directory: " << logDir.toStdString() << std::endl;
            return 1;
        }
    }
    
    // Load configuration
    SimulatorConfig config;
    QString errorMessage;
    
    std::cout << "Loading simulator config..." << std::endl;
    if (!config.loadFromFile(configPath, errorMessage)) {
        std::cerr << "Failed to load simulator config:" << std::endl;
        std::cerr << errorMessage.toStdString() << std::endl;
        return 1;
    }
    
    std::cout << "Simulator config loaded successfully" << std::endl;
    
    // Validate configuration
    if (!config.validate(errorMessage)) {
        std::cerr << "Simulator configuration validation failed:" << std::endl;
        std::cerr << errorMessage.toStdString() << std::endl;
        std::cerr << "\nPlease fix the configuration file at: " << configPath.toStdString() << std::endl;
        return 1;
    }
    
    std::cout << "Simulator configuration validated successfully" << std::endl;
    
    // Initialize logger (always enable console, no debug for simulator)
    Logger& logger = Logger::instance();
    if (!logger.initialize(logDir, false, true, 50)) {  // 50 MB max for simulator
        std::cerr << "Failed to initialize logger" << std::endl;
        return 1;
    }
    
    logger.info("=================================================");
    logger.info("MQTT Simulator initialized");
    logger.info(QString("Qt version: %1").arg(QT_VERSION_STR));
    logger.info("=================================================");
    
    // Display loaded configuration
    logger.info("");
    logger.info("=== Simulator Configuration ===");
    logger.info(QString("MQTT Broker: %1:%2").arg(config.broker().host).arg(config.broker().port));
    logger.info("");
    logger.info(QString("Simulated Controllers: %1").arg(config.controllers().size()));
    for (const auto& ctrl : config.controllers()) {
        logger.info(QString("  - Prefix: %1").arg(ctrl.prefix));
        logger.info(QString("    Commands: %1").arg(ctrl.commands.size()));
        for (const auto& cmd : ctrl.commands) {
            QString response = cmd.response.isEmpty() ? "(no response)" : cmd.response.left(50) + "...";
            logger.info(QString("      * %1 -> %2").arg(cmd.command, response));
        }
    }
    logger.info("===============================");
    logger.info("");
    
    logger.info("Phase 2: Logging system - OK");
    
    // Shutdown logger
    logger.shutdown();
    
    return 0; // Exit for now, MQTT logic in Phase 3
}
