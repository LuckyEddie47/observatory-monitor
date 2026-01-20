#ifndef SIMULATORCONFIG_H
#define SIMULATORCONFIG_H

#include <QString>
#include <QList>
#include "Config.h"

namespace ObservatoryMonitor {

// Structure for a command/response pair
struct CommandResponse {
    QString command;
    QString response;
    int delayMs;
    
    CommandResponse() : delayMs(50) {}
};

// Structure for a simulated controller
struct SimulatedController {
    QString prefix;
    QList<CommandResponse> commands;
};

// Simulator configuration class
class SimulatorConfig {
public:
    SimulatorConfig();
    
    // Load configuration from file
    bool loadFromFile(const QString& filePath, QString& errorMessage);
    
    // Save configuration to file
    bool saveToFile(const QString& filePath, QString& errorMessage);
    
    // Generate default configuration
    void setDefaults();
    
    // Validate loaded configuration
    bool validate(QString& errorMessage) const;
    
    // Getters
    BrokerConfig broker() const { return m_broker; }
    QList<SimulatedController> controllers() const { return m_controllers; }
    
    // Find command response for a given prefix and command
    // Returns nullptr if not found
    const CommandResponse* findResponse(const QString& prefix, const QString& command) const;
    
private:
    BrokerConfig m_broker;
    QList<SimulatedController> m_controllers;
};

} // namespace ObservatoryMonitor

#endif // SIMULATORCONFIG_H
