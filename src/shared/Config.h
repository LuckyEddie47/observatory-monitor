#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QList>
#include <QVariantMap>

namespace ObservatoryMonitor {

// Structure for MQTT broker configuration
struct BrokerConfig {
    QString host;
    int port;
    QString username;
    QString password;
    
    BrokerConfig() : host("localhost"), port(1883) {}
};

// Structure for controller configuration
struct ControllerConfig {
    QString name;
    QString type;
    QString prefix;
    bool enabled;
    
    ControllerConfig() : enabled(true) {}
};

// Structure for equipment type configuration
struct EquipmentType {
    QString name;
    QStringList controllers;
};

// Structure for logging configuration
struct LoggingConfig {
    bool debugEnabled;
    int maxTotalSizeMB;
    
    LoggingConfig() : debugEnabled(false), maxTotalSizeMB(100) {}
};

// Main configuration class
class Config {
public:
    Config();
    
    // Load configuration from file
    // Returns true on success, false on error
    // errorMessage will contain detailed error information on failure
    bool loadFromFile(const QString& filePath, QString& errorMessage);
    
    // Save current configuration to file
    bool saveToFile(const QString& filePath, QString& errorMessage);
    
    // Generate default configuration
    void setDefaults();
    
    // Validate loaded configuration
    // Returns true if valid, false if invalid
    // errorMessage will contain detailed validation errors
    bool validate(QString& errorMessage) const;
    
    // Getters
    BrokerConfig broker() const { return m_broker; }
    double mqttTimeout() const { return m_mqttTimeout; }
    int reconnectInterval() const { return m_reconnectInterval; }
    QList<ControllerConfig> controllers() const { return m_controllers; }
    QList<EquipmentType> equipmentTypes() const { return m_equipmentTypes; }
    LoggingConfig logging() const { return m_logging; }
    
    // Setters (for testing)
    void setBroker(const BrokerConfig& broker) { m_broker = broker; }
    void setMqttTimeout(double timeout) { m_mqttTimeout = timeout; }
    void setReconnectInterval(int interval) { m_reconnectInterval = interval; }
    void addController(const ControllerConfig& controller) { m_controllers.append(controller); }
    void addEquipmentType(const EquipmentType& type) { m_equipmentTypes.append(type); }
    void setLogging(const LoggingConfig& logging) { m_logging = logging; }
    
private:
    BrokerConfig m_broker;
    double m_mqttTimeout;
    int m_reconnectInterval;
    QList<ControllerConfig> m_controllers;
    QList<EquipmentType> m_equipmentTypes;
    LoggingConfig m_logging;
    
    // Helper methods
    bool validateBroker(QString& errorMessage) const;
    bool validateControllers(QString& errorMessage) const;
    bool validateEquipmentTypes(QString& errorMessage) const;
    bool validateLogging(QString& errorMessage) const;
};

} // namespace ObservatoryMonitor

#endif // CONFIG_H
