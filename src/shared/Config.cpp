#include "Config.h"
#include <yaml-cpp/yaml.h>
#include <QFile>
#include <QDebug>
#include <stdexcept>

namespace ObservatoryMonitor {

Config::Config()
    : m_mqttTimeout(2.0)
    , m_reconnectInterval(10)
{
    setDefaults();
}

void Config::setDefaults()
{
    // Broker defaults
    m_broker = BrokerConfig();
    m_broker.host = "localhost";
    m_broker.port = 1883;
    m_broker.username = "";
    m_broker.password = "";
    
    // MQTT settings
    m_mqttTimeout = 2.0;
    m_reconnectInterval = 10;
    
    // Logging defaults
    m_logging = LoggingConfig();
    m_logging.debugEnabled = false;
    m_logging.maxTotalSizeMB = 100;
    
    // Default controllers
    m_controllers.clear();
    
    ControllerConfig observatoryController;
    observatoryController.name = "Observatory";
    observatoryController.type = "Observatory";
    observatoryController.prefix = "OCS";
    observatoryController.enabled = true;
    m_controllers.append(observatoryController);

    ControllerConfig telescopeController;
    telescopeController.name = "Telescope";
    telescopeController.type = "Telescope";
    telescopeController.prefix = "OnStepX";
    telescopeController.enabled = true;
    m_controllers.append(telescopeController);
    
    // Default equipment types
    m_equipmentTypes.clear();
    
    EquipmentType observatory;
    observatory.name = "Observatory";
    observatory.controllers << "OCS";
    m_equipmentTypes.append(observatory);
    
    EquipmentType telescope;
    telescope.name = "Telescope";
    telescope.controllers << "OnStepX";
    m_equipmentTypes.append(telescope);
    
    EquipmentType auxiliary;
    auxiliary.name = "Auxiliary";
    m_equipmentTypes.append(auxiliary);
    
    EquipmentType other;
    other.name = "Other";
    m_equipmentTypes.append(other);
}

bool Config::loadFromFile(const QString& filePath, QString& errorMessage)
{
    try {
        YAML::Node config = YAML::LoadFile(filePath.toStdString());
        
        // Parse MQTT broker settings
        if (config["mqtt"]) {
            YAML::Node mqtt = config["mqtt"];
            
            if (mqtt["broker"]) {
                YAML::Node broker = mqtt["broker"];
                if (broker["host"]) m_broker.host = QString::fromStdString(broker["host"].as<std::string>());
                if (broker["port"]) m_broker.port = broker["port"].as<int>();
                if (broker["username"]) m_broker.username = QString::fromStdString(broker["username"].as<std::string>());
                if (broker["password"]) m_broker.password = QString::fromStdString(broker["password"].as<std::string>());
            }
            
            if (mqtt["timeout"]) m_mqttTimeout = mqtt["timeout"].as<double>();
            if (mqtt["reconnect_interval"]) m_reconnectInterval = mqtt["reconnect_interval"].as<int>();
        }
        
        // Parse controllers
        if (config["controllers"]) {
            m_controllers.clear();
            YAML::Node controllers = config["controllers"];
            
            for (std::size_t i = 0; i < controllers.size(); i++) {
                YAML::Node ctrl = controllers[i];
                ControllerConfig controller;
                
                if (ctrl["name"]) controller.name = QString::fromStdString(ctrl["name"].as<std::string>());
                if (ctrl["type"]) controller.type = QString::fromStdString(ctrl["type"].as<std::string>());
                if (ctrl["prefix"]) controller.prefix = QString::fromStdString(ctrl["prefix"].as<std::string>());
                if (ctrl["enabled"]) controller.enabled = ctrl["enabled"].as<bool>();
                
                m_controllers.append(controller);
            }
        }
        
        // Parse equipment types
        if (config["equipment_types"]) {
            m_equipmentTypes.clear();
            YAML::Node types = config["equipment_types"];
            
            for (std::size_t i = 0; i < types.size(); i++) {
                YAML::Node typeNode = types[i];
                EquipmentType equipType;
                
                if (typeNode["name"]) {
                    equipType.name = QString::fromStdString(typeNode["name"].as<std::string>());
                }
                
                if (typeNode["controllers"]) {
                    YAML::Node controllers = typeNode["controllers"];
                    for (std::size_t j = 0; j < controllers.size(); j++) {
                        equipType.controllers << QString::fromStdString(controllers[j].as<std::string>());
                    }
                }
                
                m_equipmentTypes.append(equipType);
            }
        }
        
        // Parse logging settings
        if (config["logging"]) {
            YAML::Node logging = config["logging"];
            if (logging["debug_enabled"]) m_logging.debugEnabled = logging["debug_enabled"].as<bool>();
            if (logging["max_total_size_mb"]) m_logging.maxTotalSizeMB = logging["max_total_size_mb"].as<int>();
        }
        
        return true;
        
    } catch (const YAML::ParserException& e) {
        errorMessage = QString("Error parsing config file '%1'.\n"
                              "YAML Parser Error at line %2, column %3: %4")
                              .arg(filePath)
                              .arg(e.mark.line + 1)
                              .arg(e.mark.column + 1)
                              .arg(QString::fromStdString(e.msg));
        return false;
        
    } catch (const YAML::BadFile& e) {
        errorMessage = QString("Error opening config file '%1'.\n"
                              "File cannot be read: %2")
                              .arg(filePath)
                              .arg(QString::fromStdString(e.msg));
        return false;
        
    } catch (const YAML::Exception& e) {
        errorMessage = QString("Error in config file '%1'.\n"
                              "YAML Error: %2")
                              .arg(filePath)
                              .arg(QString::fromStdString(e.msg));
        return false;
        
    } catch (const std::exception& e) {
        errorMessage = QString("Unexpected error loading config file '%1'.\n"
                              "Error: %2")
                              .arg(filePath)
                              .arg(QString::fromStdString(e.what()));
        return false;
    }
}

bool Config::saveToFile(const QString& filePath, QString& errorMessage)
{
    try {
        YAML::Emitter out;
        out << YAML::BeginMap;
        
        // MQTT section
        out << YAML::Key << "mqtt";
        out << YAML::Value << YAML::BeginMap;
        
        out << YAML::Key << "broker";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "host" << YAML::Value << m_broker.host.toStdString();
        out << YAML::Key << "port" << YAML::Value << m_broker.port;
        out << YAML::Key << "username" << YAML::Value << m_broker.username.toStdString();
        out << YAML::Key << "password" << YAML::Value << m_broker.password.toStdString();
        out << YAML::EndMap;
        
        out << YAML::Key << "timeout" << YAML::Value << m_mqttTimeout;
        out << YAML::Key << "reconnect_interval" << YAML::Value << m_reconnectInterval;
        
        out << YAML::EndMap;
        
        // Controllers section
        out << YAML::Key << "controllers";
        out << YAML::Value << YAML::BeginSeq;
        for (const auto& ctrl : m_controllers) {
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << ctrl.name.toStdString();
            out << YAML::Key << "type" << YAML::Value << ctrl.type.toStdString();
            out << YAML::Key << "prefix" << YAML::Value << ctrl.prefix.toStdString();
            out << YAML::Key << "enabled" << YAML::Value << ctrl.enabled;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        
        // Equipment types section
        out << YAML::Key << "equipment_types";
        out << YAML::Value << YAML::BeginSeq;
        for (const auto& type : m_equipmentTypes) {
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << type.name.toStdString();
            out << YAML::Key << "controllers" << YAML::Value << YAML::BeginSeq;
            for (const auto& ctrl : type.controllers) {
                out << ctrl.toStdString();
            }
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        
        // Logging section
        out << YAML::Key << "logging";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "debug_enabled" << YAML::Value << m_logging.debugEnabled;
        out << YAML::Key << "max_total_size_mb" << YAML::Value << m_logging.maxTotalSizeMB;
        out << YAML::EndMap;
        
        out << YAML::EndMap;
        
        // Write to file
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            errorMessage = QString("Cannot write to file '%1'.\n"
                                  "Error: %2")
                                  .arg(filePath)
                                  .arg(file.errorString());
            return false;
        }
        
        file.write(out.c_str());
        file.close();
        
        return true;
        
    } catch (const std::exception& e) {
        errorMessage = QString("Error saving config file '%1'.\n"
                              "Error: %2")
                              .arg(filePath)
                              .arg(QString::fromStdString(e.what()));
        return false;
    }
}

bool Config::validate(QString& errorMessage) const
{
    QStringList errors;
    
    if (!validateBroker(errorMessage)) {
        errors << errorMessage;
    }
    
    QString ctrlError;
    if (!validateControllers(ctrlError)) {
        errors << ctrlError;
    }
    
    QString typeError;
    if (!validateEquipmentTypes(typeError)) {
        errors << typeError;
    }
    
    QString logError;
    if (!validateLogging(logError)) {
        errors << logError;
    }
    
    if (!errors.isEmpty()) {
        errorMessage = "Configuration validation failed:\n\n" + errors.join("\n\n");
        return false;
    }
    
    return true;
}

bool Config::validateBroker(QString& errorMessage) const
{
    QStringList errors;
    
    // Validate host
    if (m_broker.host.isEmpty()) {
        errors << "MQTT broker host is empty (mqtt.broker.host)";
    }
    
    // Validate port
    if (m_broker.port < 1 || m_broker.port > 65535) {
        errors << QString("MQTT broker port is invalid: %1 (mqtt.broker.port)\n"
                         "Valid range: 1-65535")
			.arg(m_broker.port);
	}
// Validate timeout
if (m_mqttTimeout < 0.5 || m_mqttTimeout > 30.0) {
    errors << QString("MQTT timeout is out of range: %1 seconds (mqtt.timeout)\n"
                     "Valid range: 0.5-30.0 seconds")
                     .arg(m_mqttTimeout);
}

// Validate reconnect interval
if (m_reconnectInterval < 1 || m_reconnectInterval > 300) {
    errors << QString("MQTT reconnect interval is out of range: %1 seconds (mqtt.reconnect_interval)\n"
                     "Valid range: 1-300 seconds")
                     .arg(m_reconnectInterval);
}

if (!errors.isEmpty()) {
    errorMessage = "Broker configuration errors:\n" + errors.join("\n");
    return false;
}

return true;
}
bool Config::validateControllers(QString& errorMessage) const
{
QStringList errors;
// Check if at least one controller is defined
if (m_controllers.isEmpty()) {
    errors << "No controllers defined (controllers section is empty)\n"
              "At least one controller must be configured";
}

// Validate each controller
for (int i = 0; i < m_controllers.size(); i++) {
    const auto& ctrl = m_controllers[i];
    QString prefix = QString("Controller #%1").arg(i + 1);
    
    if (ctrl.name.isEmpty()) {
        errors << QString("%1: name is empty (controllers[%2].name)").arg(prefix).arg(i);
    }
    
    if (ctrl.type.isEmpty()) {
        errors << QString("%1: type is empty (controllers[%2].type)").arg(prefix).arg(i);
    }
    
    if (ctrl.prefix.isEmpty()) {
        errors << QString("%1: MQTT prefix is empty (controllers[%2].prefix)").arg(prefix).arg(i);
    }
    
    // Check for duplicate prefixes
    for (int j = i + 1; j < m_controllers.size(); j++) {
        if (ctrl.prefix == m_controllers[j].prefix && !ctrl.prefix.isEmpty()) {
            errors << QString("%1: duplicate MQTT prefix '%2' found at controllers[%3] and controllers[%4]")
                             .arg(prefix)
                             .arg(ctrl.prefix)
                             .arg(i)
                             .arg(j);
        }
    }
}

if (!errors.isEmpty()) {
    errorMessage = "Controller configuration errors:\n" + errors.join("\n");
    return false;
}

return true;
}
bool Config::validateEquipmentTypes(QString& errorMessage) const
{
QStringList errors;
// Check if at least one equipment type is defined
if (m_equipmentTypes.isEmpty()) {
    errors << "No equipment types defined (equipment_types section is empty)\n"
              "At least one equipment type must be configured";
}

// Validate each equipment type
for (int i = 0; i < m_equipmentTypes.size(); i++) {
    const auto& type = m_equipmentTypes[i];
    
    if (type.name.isEmpty()) {
        errors << QString("Equipment type #%1: name is empty (equipment_types[%2].name)")
                         .arg(i + 1)
                         .arg(i);
    }
    
    // Check for duplicate names
    for (int j = i + 1; j < m_equipmentTypes.size(); j++) {
        if (type.name == m_equipmentTypes[j].name && !type.name.isEmpty()) {
            errors << QString("Duplicate equipment type name '%1' found at equipment_types[%2] and equipment_types[%3]")
                             .arg(type.name)
                             .arg(i)
                             .arg(j);
        }
    }
}

if (!errors.isEmpty()) {
    errorMessage = "Equipment type configuration errors:\n" + errors.join("\n");
    return false;
}

return true;
}
bool Config::validateLogging(QString& errorMessage) const
{
QStringList errors;
// Validate max total size
if (m_logging.maxTotalSizeMB < 1 || m_logging.maxTotalSizeMB > 10000) {
    errors << QString("Logging max total size is out of range: %1 MB (logging.max_total_size_mb)\n"
                     "Valid range: 1-10000 MB")
                     .arg(m_logging.maxTotalSizeMB);
}

if (!errors.isEmpty()) {
    errorMessage = "Logging configuration errors:\n" + errors.join("\n");
    return false;
}

return true;
}
} // namespace ObservatoryMonitor
