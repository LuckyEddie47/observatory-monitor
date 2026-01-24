#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H

#include "AbstractController.h"
#include "Config.h"

namespace ObservatoryMonitor {

// Structure to track a single controller
struct ControllerInfo {
    QString name;
    bool enabled;
    AbstractController* controller;
    ControllerStatus status;
    
    ControllerInfo() 
        : enabled(false)
        , controller(nullptr)
        , status(ControllerStatus::Disconnected) 
    {}
};

class ControllerManager : public QObject
{
    Q_OBJECT

public:
    explicit ControllerManager(QObject* parent = nullptr);
    ~ControllerManager();
    
    // Configuration
    void loadControllersFromConfig(const Config& config);
    void updateBrokerConfig(const BrokerConfig& broker, double timeout, int reconnectInterval);
    
    // Controller management
    void addController(const ControllerConfig& config, const BrokerConfig& broker, double timeout, int reconnectInterval);
    void removeController(const QString& name);
    void enableController(const QString& name, bool enable);
    
    // Connection management
    void connectAll();
    void disconnectAll();
    void connectController(const QString& name);
    void disconnectController(const QString& name);
    
    // Polling management
    void startPolling(int fastPollMs = 1000, int slowPollMs = 10000);
    void stopPolling();
    void startControllerPolling(const QString& name);
    void stopControllerPolling(const QString& name);
    
    // Status queries
    ControllerStatus getControllerStatus(const QString& name) const;
    bool isControllerEnabled(const QString& name) const;
    QString getControllerType(const QString& name) const;
    SystemStatus getSystemStatus() const;
    QStringList getControllerNames() const;
    QStringList getConnectedControllers() const;
    QStringList getDisconnectedControllers() const;
    int getEnabledControllerCount() const;
    int getConnectedControllerCount() const;
    
    // Data access
    CachedValue getControllerValue(const QString& controllerName, const QString& command) const;
    QHash<QString, CachedValue> getAllControllerValues(const QString& controllerName) const;
    
signals:
    void controllerStatusChanged(const QString& name, ControllerStatus status);
    void controllerEnabledChanged(const QString& name, bool enabled);
    void systemStatusChanged(SystemStatus status);
    void controllerDataUpdated(const QString& controllerName, const QString& command, const QString& value);
    void controllerError(const QString& controllerName, const QString& error);
    
private slots:
    void onControllerConnected();
    void onControllerDisconnected();
    void onControllerError(const QString& error);
    void onControllerDataUpdated(const QString& command, const QString& value);
    void onControllerPollError(const QString& command, const QString& error);
    
private:
    void updateControllerStatus(const QString& name, ControllerStatus status);
    void updateSystemStatus();
    QString getControllerNameFromSender() const;
    
    QHash<QString, ControllerInfo> m_controllers;
    SystemStatus m_systemStatus;
    
    int m_fastPollInterval;
    int m_slowPollInterval;
    bool m_isPolling;
};

} // namespace ObservatoryMonitor

#endif // CONTROLLERMANAGER_H
