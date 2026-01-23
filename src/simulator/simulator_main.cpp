#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QMqttClient>
#include <QMqttSubscription>
#include <QMqttMessage>
#include <iostream>
#include "SimulatorConfig.h"
#include "Logger.h"

using namespace ObservatoryMonitor;

class MqttSimulator : public QObject
{
    Q_OBJECT

public:
    MqttSimulator(const SimulatorConfig& config, QObject* parent = nullptr)
        : QObject(parent)
        , m_config(config)
        , m_client(new QMqttClient(this))
        , m_azimuth(0.0)
        , m_azimuthIncreasing(true)
        , m_altitude(0.0)
        , m_altitudeIncreasing(true)
    {
        // Setup MQTT client
        m_client->setHostname(m_config.broker().host);
        m_client->setPort(m_config.broker().port);
        
        if (!m_config.broker().username.isEmpty()) {
            m_client->setUsername(m_config.broker().username);
            m_client->setPassword(m_config.broker().password);
        }
        
        // Connect signals
        connect(m_client, &QMqttClient::connected, this, &MqttSimulator::onConnected);
        connect(m_client, &QMqttClient::disconnected, this, &MqttSimulator::onDisconnected);
        connect(m_client, &QMqttClient::errorChanged, this, &MqttSimulator::onErrorChanged);

        // Update movement timer: 100ms
        m_updateTimer = new QTimer(this);
        connect(m_updateTimer, &QTimer::timeout, this, &MqttSimulator::updateMovement);
        m_updateTimer->start(100);
    }
    
    void start()
    {
        Logger::instance().info("Simulator: Connecting to MQTT broker...");
        m_client->connectToHost();
    }
    
private slots:
    void updateMovement()
    {
        // Update Azimuth (0-360)
        if (m_azimuthIncreasing) {
            m_azimuth += 0.5;
            if (m_azimuth >= 360.0) {
                m_azimuth = 360.0;
                m_azimuthIncreasing = false;
            }
        } else {
            m_azimuth -= 0.5;
            if (m_azimuth <= 0.0) {
                m_azimuth = 0.0;
                m_azimuthIncreasing = true;
            }
        }

        // Update Altitude (0-90)
        if (m_altitudeIncreasing) {
            m_altitude += 0.2;
            if (m_altitude >= 90.0) {
                m_altitude = 90.0;
                m_altitudeIncreasing = false;
            }
        } else {
            m_altitude -= 0.2;
            if (m_altitude <= 0.0) {
                m_altitude = 0.0;
                m_altitudeIncreasing = true;
            }
        }
    }

    void onConnected()
    {
        Logger::instance().info(QString("Simulator: Connected to %1:%2")
                               .arg(m_client->hostname())
                               .arg(m_client->port()));
        
        // Subscribe to all controller cmd topics
        for (const auto& ctrl : m_config.controllers()) {
            QString cmdTopic = ctrl.prefix + "/cmd";
            
            Logger::instance().info(QString("Simulator: Subscribing to %1").arg(cmdTopic));
            
            auto subscription = m_client->subscribe(cmdTopic, 1);  // QoS 1
            if (subscription) {
                m_subscriptions.append(subscription);
                connect(subscription, &QMqttSubscription::messageReceived,
                       this, [this, ctrl](const QMqttMessage& msg) {
                    handleCommand(ctrl.prefix, msg.payload());
                });
            } else {
                Logger::instance().error(QString("Simulator: Failed to subscribe to %1").arg(cmdTopic));
            }
        }
    }
    
    void onDisconnected()
    {
        Logger::instance().warning("Simulator: Disconnected from broker");
    }
    
    void onErrorChanged(QMqttClient::ClientError error)
    {
        if (error == QMqttClient::NoError) {
            return;
        }
        
        Logger::instance().error(QString("Simulator: MQTT Error - %1").arg(static_cast<int>(error)));
    }
    
    void handleCommand(const QString& prefix, const QByteArray& message)
    {
        QString command = QString::fromUtf8(message);
        
        Logger::instance().debug(QString("Simulator: Received command on %1/cmd: %2").arg(prefix, command));
        
        // Special case for dynamic values
        if (command == ":DZ#" || command == ":GZ#") {
            QString responseValue = QString("%1#").arg(m_azimuth, 0, 'f', 3);
            QString response = QString("Received: %1, Response: %2, Source: MQTT").arg(command, responseValue);
            sendResponse(prefix, command, response);
            return;
        }

        if (command == ":GA#") {
            QString responseValue = QString("%1#").arg(m_altitude, 0, 'f', 3);
            QString response = QString("Received: :GA#, Response: %1, Source: MQTT").arg(responseValue);
            sendResponse(prefix, command, response);
            return;
        }

        // Find response for this command
        const CommandResponse* cmdResp = m_config.findResponse(prefix, command);
        
        if (!cmdResp) {
            // Default response to prevent timeouts for common commands
            QString responseValue;
            if (command == ":RS#") responseValue = "0#"; // Ready
            else if (command == ":GR#") responseValue = "12:34:56#";
            else if (command == ":GD#") responseValue = "+45*30'00#";
            else if (command == ":GS#") responseValue = "0#";
            else {
                Logger::instance().warning(QString("Simulator: No configured response for command: %1").arg(command));
                return;
            }

            QString defaultResponse = QString("Received: %1, Response: %2, Source: MQTT").arg(command, responseValue);
            sendResponse(prefix, command, defaultResponse);
            return;
        }
        
        // Delay response if configured
        if (cmdResp->delayMs > 0) {
            QTimer::singleShot(cmdResp->delayMs, this, [this, prefix, command, cmdResp]() {
                sendResponse(prefix, command, cmdResp->response);
            });
        } else {
            sendResponse(prefix, command, cmdResp->response);
        }
    }
    
    void sendResponse(const QString& prefix, const QString& command, const QString& response)
    {
        if (response.isEmpty()) {
            Logger::instance().debug(QString("Simulator: No response configured for %1").arg(command));
            return;
        }
        
        // Ensure the response follows the expected OCS format if it doesn't already
        QString fullResponse = response;
        if (!response.startsWith("Received:")) {
            fullResponse = QString("Received: %1, Response: %2, Source: MQTT").arg(command, response);
        }
        
        QString echoTopic = prefix + "/echo";
        QByteArray message = fullResponse.toUtf8();
        
        Logger::instance().debug(QString("Simulator: Publishing to %1: %2").arg(echoTopic, fullResponse));
        
        qint64 msgId = m_client->publish(echoTopic, message, 0);  // QoS 0 for echo
        
        if (msgId == -1) {
            Logger::instance().error(QString("Simulator: Failed to publish response to %1").arg(echoTopic));
        }
    }
    
private:
    SimulatorConfig m_config;
    QMqttClient* m_client;
    QList<QMqttSubscription*> m_subscriptions;
    QTimer* m_updateTimer;
    double m_azimuth;
    bool m_azimuthIncreasing;
    double m_altitude;
    bool m_altitudeIncreasing;
};

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
    
    logger.info("Phase 3: MQTT Core - Simulator ready");
    logger.info("");
    
    // Create and start simulator
    MqttSimulator simulator(config);
    simulator.start();
    
    logger.info("Simulator running. Press Ctrl+C to stop.");
    
    int result = app.exec();
    
    // Shutdown logger
    logger.shutdown();
    
    return result;
}

#include "simulator_main.moc"
