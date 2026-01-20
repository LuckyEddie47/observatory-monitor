#include "SimulatorConfig.h"
#include <yaml-cpp/yaml.h>
#include <QFile>
#include <QDebug>

namespace ObservatoryMonitor {

SimulatorConfig::SimulatorConfig()
{
    setDefaults();
}

void SimulatorConfig::setDefaults()
{
    // Broker defaults
    m_broker = BrokerConfig();
    m_broker.host = "localhost";
    m_broker.port = 1883;
    m_broker.username = "";
    m_broker.password = "";
    
    // Default simulated controller
    m_controllers.clear();
    SimulatedController defaultController;
    defaultController.prefix = "OCS";
    
    CommandResponse domeAzimuth;
    domeAzimuth.command = ":DZ#";
    domeAzimuth.response = "Received: :DZ#, Response: 306.640#, Source: MQTT";
    domeAzimuth.delayMs = 50;
    defaultController.commands.append(domeAzimuth);
    
    CommandResponse roofStatus;
    roofStatus.command = ":RS#";
    roofStatus.response = "Received: :RS#, Response: OPEN#, Source: MQTT";
    roofStatus.delayMs = 50;
    defaultController.commands.append(roofStatus);
    
    m_controllers.append(defaultController);
}

bool SimulatorConfig::loadFromFile(const QString& filePath, QString& errorMessage)
{
    try {
        YAML::Node config = YAML::LoadFile(filePath.toStdString());
        
        if (!config["simulator"]) {
            errorMessage = QString("Error in config file '%1'.\n"
                                  "Missing required section: 'simulator'")
                                  .arg(filePath);
            return false;
        }
        
        YAML::Node sim = config["simulator"];
        
        // Parse broker settings
        if (sim["broker"]) {
            YAML::Node broker = sim["broker"];
            if (broker["host"]) m_broker.host = QString::fromStdString(broker["host"].as<std::string>());
            if (broker["port"]) m_broker.port = broker["port"].as<int>();
            if (broker["username"]) m_broker.username = QString::fromStdString(broker["username"].as<std::string>());
            if (broker["password"]) m_broker.password = QString::fromStdString(broker["password"].as<std::string>());
        }
        
        // Parse controllers
        if (sim["controllers"]) {
            m_controllers.clear();
            YAML::Node controllers = sim["controllers"];
            
            for (std::size_t i = 0; i < controllers.size(); i++) {
                YAML::Node ctrlNode = controllers[i];
                SimulatedController controller;
                
                if (ctrlNode["prefix"]) {
                    controller.prefix = QString::fromStdString(ctrlNode["prefix"].as<std::string>());
                }
                
                if (ctrlNode["commands"]) {
                    YAML::Node commands = ctrlNode["commands"];
                    for (std::size_t j = 0; j < commands.size(); j++) {
                        YAML::Node cmdNode = commands[j];
                        CommandResponse cmd;
                        
                        if (cmdNode["command"]) {
                            cmd.command = QString::fromStdString(cmdNode["command"].as<std::string>());
                        }
                        if (cmdNode["response"]) {
                            cmd.response = QString::fromStdString(cmdNode["response"].as<std::string>());
                        }
                        if (cmdNode["delay_ms"]) {
                            cmd.delayMs = cmdNode["delay_ms"].as<int>();
                        }
                        
                        controller.commands.append(cmd);
                    }
                }
                
                m_controllers.append(controller);
            }
        }
        
        return true;
        
    } catch (const YAML::ParserException& e) {
        errorMessage = QString("Error parsing simulator config file '%1'.\n"
                              "YAML Parser Error at line %2, column %3: %4")
                              .arg(filePath)
                              .arg(e.mark.line + 1)
                              .arg(e.mark.column + 1)
                              .arg(QString::fromStdString(e.msg));
        return false;
        
    } catch (const YAML::BadFile& e) {
        errorMessage = QString("Error opening simulator config file '%1'.\n"
                              "File cannot be read: %2")
                              .arg(filePath)
                              .arg(QString::fromStdString(e.msg));
        return false;
        
    } catch (const YAML::Exception& e) {
        errorMessage = QString("Error in simulator config file '%1'.\n"
                              "YAML Error: %2")
                              .arg(filePath)
                              .arg(QString::fromStdString(e.msg));
        return false;
        
    } catch (const std::exception& e) {
        errorMessage = QString("Unexpected error loading simulator config file '%1'.\n"
                              "Error: %2")
                              .arg(filePath)
                              .arg(QString::fromStdString(e.what()));
        return false;
    }
}

bool SimulatorConfig::saveToFile(const QString& filePath, QString& errorMessage)
{
    try {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "simulator";
        out << YAML::Value << YAML::BeginMap;
        
        // Broker section
        out << YAML::Key << "broker";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "host" << YAML::Value << m_broker.host.toStdString();
        out << YAML::Key << "port" << YAML::Value << m_broker.port;
        out << YAML::Key << "username" << YAML::Value << m_broker.username.toStdString();
        out << YAML::Key << "password" << YAML::Value << m_broker.password.toStdString();
        out << YAML::EndMap;
        
        // Controllers section
        out << YAML::Key << "controllers";
        out << YAML::Value << YAML::BeginSeq;
        for (const auto& ctrl : m_controllers) {
            out << YAML::BeginMap;
            out << YAML::Key << "prefix" << YAML::Value << ctrl.prefix.toStdString();
            out << YAML::Key << "commands" << YAML::Value << YAML::BeginSeq;
            
            for (const auto& cmd : ctrl.commands) {
                out << YAML::BeginMap;
                out << YAML::Key << "command" << YAML::Value << cmd.command.toStdString();
                out << YAML::Key << "response" << YAML::Value << cmd.response.toStdString();
                out << YAML::Key << "delay_ms" << YAML::Value << cmd.delayMs;
                out << YAML::EndMap;
            }
            
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        
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
        errorMessage = QString("Error saving simulator config file '%1'.\n"
                              "Error: %2")
                              .arg(filePath)
                              .arg(QString::fromStdString(e.what()));
        return false;
    }
}

bool SimulatorConfig::validate(QString& errorMessage) const
{
    QStringList errors;
    
    // Validate broker
    if (m_broker.host.isEmpty()) {
        errors << "MQTT broker host is empty (simulator.broker.host)";
    }
    
    if (m_broker.port < 1 || m_broker.port > 65535) {
        errors << QString("MQTT broker port is invalid: %1 (simulator.broker.port)\n"
                         "Valid range: 1-65535")
                         .arg(m_broker.port);
    }
    
    // Validate controllers
    if (m_controllers.isEmpty()) {
        errors << "No controllers defined (simulator.controllers section is empty)\n"
                  "At least one controller must be configured";
    }
    
    for (int i = 0; i < m_controllers.size(); i++) {
        const auto& ctrl = m_controllers[i];
        QString prefix = QString("Controller #%1").arg(i + 1);
        
        if (ctrl.prefix.isEmpty()) {
            errors << QString("%1: MQTT prefix is empty (simulator.controllers[%2].prefix)")
                             .arg(prefix).arg(i);
    }
    
    if (ctrl.commands.isEmpty()) {
        errors << QString("%1: No commands defined (simulator.controllers[%2].commands is empty)")
                         .arg(prefix).arg(i);
    }
    
    // Validate commands
    for (int j = 0; j < ctrl.commands.size(); j++) {
        const auto& cmd = ctrl.commands[j];
        
        if (cmd.command.isEmpty()) {
            errors << QString("%1: Command #%2 has empty command string (simulator.controllers[%3].commands[%4].command)")
                             .arg(prefix).arg(j + 1).arg(i).arg(j);
        }
        
        if (cmd.delayMs < 0 || cmd.delayMs > 10000) {
            errors << QString("%1: Command #%2 has invalid delay: %3ms (simulator.controllers[%4].commands[%5].delay_ms)\n"
                             "Valid range: 0-10000 milliseconds")
                             .arg(prefix).arg(j + 1).arg(cmd.delayMs).arg(i).arg(j);
        }
    }
}

if (!errors.isEmpty()) {
    errorMessage = "Simulator configuration validation failed:\n\n" + errors.join("\n\n");
    return false;
}

return true;
}
const CommandResponse* SimulatorConfig::findResponse(const QString& prefix, const QString& command) const
{
	for (const auto& ctrl : m_controllers) {
		if (ctrl.prefix == prefix) {
			for (const auto& cmd : ctrl.commands) {
				if (cmd.command == command) {
					return &cmd;
				}
			}
		}
	}
	return nullptr;
}

} // namespace ObservatoryMonitor
