#include "ControllerManager.h"
#include "MqttController.h"
#include "Logger.h"

namespace ObservatoryMonitor {

ControllerManager::ControllerManager(QObject* parent)
    : QObject(parent)
    , m_systemStatus(SystemStatus::Disconnected)
    , m_fastPollInterval(1000)
    , m_slowPollInterval(10000)
    , m_isPolling(false)
{
}

ControllerManager::~ControllerManager()
{
    for (auto& info : m_controllers) {
        delete info.controller;
    }
    m_controllers.clear();
}

void ControllerManager::loadControllersFromConfig(const Config& config)
{
    Logger::instance().info("ControllerManager: Loading controllers from configuration");
    
    stopPolling();
    disconnectAll();
    
    for (auto& info : m_controllers) {
        delete info.controller;
    }
    m_controllers.clear();
    
    for (const auto& ctrl : config.controllers()) {
        addController(ctrl, config.broker(), config.mqttTimeout(), config.reconnectInterval());
    }
}

void ControllerManager::addController(const ControllerConfig& config, const BrokerConfig& broker, double timeout, int reconnectInterval)
{
    if (m_controllers.contains(config.name)) {
        return;
    }
    
    ControllerInfo info;
    info.name = config.name;
    info.enabled = config.enabled;
    
    // Create MQTT controller
    MqttController* mqttCtrl = new MqttController(config, broker, timeout, reconnectInterval, this);
    info.controller = mqttCtrl;
    info.status = mqttCtrl->status();
    
    // Connect signals
    connect(mqttCtrl, &AbstractController::statusChanged, this, [this, name = config.name](ControllerStatus status) {
        updateControllerStatus(name, status);
    });
    
    connect(mqttCtrl, &AbstractController::dataUpdated, this, [this, name = config.name](const QString& command, const QString& value) {
        emit controllerDataUpdated(name, command, value);
    });
    
    connect(mqttCtrl, &AbstractController::errorOccurred, this, [this, name = config.name](const QString& error) {
        emit controllerError(name, error);
    });
    
    m_controllers.insert(config.name, info);
    updateSystemStatus();
}

void ControllerManager::removeController(const QString& name)
{
    if (!m_controllers.contains(name)) return;
    
    ControllerInfo info = m_controllers.take(name);
    delete info.controller;
    
    updateSystemStatus();
}

void ControllerManager::enableController(const QString& name, bool enable)
{
    if (!m_controllers.contains(name)) return;
    
    ControllerInfo& info = m_controllers[name];
    if (info.enabled == enable) return;
    
    info.enabled = enable;
    
    if (enable) {
        info.controller->connect();
        if (m_isPolling) {
            static_cast<MqttController*>(info.controller)->startPolling(m_fastPollInterval, m_slowPollInterval);
        }
    } else {
        static_cast<MqttController*>(info.controller)->stopPolling();
        info.controller->disconnect();
    }
    
    updateSystemStatus();
}

void ControllerManager::connectAll()
{
    for (auto& info : m_controllers) {
        if (info.enabled) {
            info.controller->connect();
        }
    }
}

void ControllerManager::disconnectAll()
{
    for (auto& info : m_controllers) {
        info.controller->disconnect();
    }
}

void ControllerManager::connectController(const QString& name)
{
    if (m_controllers.contains(name) && m_controllers[name].enabled) {
        m_controllers[name].controller->connect();
    }
}

void ControllerManager::disconnectController(const QString& name)
{
    if (m_controllers.contains(name)) {
        m_controllers[name].controller->disconnect();
    }
}

void ControllerManager::startPolling(int fastPollMs, int slowPollMs)
{
    m_fastPollInterval = fastPollMs;
    m_slowPollInterval = slowPollMs;
    m_isPolling = true;
    
    for (auto& info : m_controllers) {
        if (info.enabled) {
            static_cast<MqttController*>(info.controller)->startPolling(fastPollMs, slowPollMs);
        }
    }
}

void ControllerManager::stopPolling()
{
    m_isPolling = false;
    for (auto& info : m_controllers) {
        static_cast<MqttController*>(info.controller)->stopPolling();
    }
}

void ControllerManager::startControllerPolling(const QString& name)
{
    if (m_controllers.contains(name) && m_controllers[name].enabled) {
        static_cast<MqttController*>(m_controllers[name].controller)->startPolling(m_fastPollInterval, m_slowPollInterval);
    }
}

void ControllerManager::stopControllerPolling(const QString& name)
{
    if (m_controllers.contains(name)) {
        static_cast<MqttController*>(m_controllers[name].controller)->stopPolling();
    }
}

ControllerStatus ControllerManager::getControllerStatus(const QString& name) const
{
    return m_controllers.contains(name) ? m_controllers[name].status : ControllerStatus::Disconnected;
}

QString ControllerManager::getControllerType(const QString& name) const
{
    if (m_controllers.contains(name) && m_controllers[name].controller) {
        return m_controllers[name].controller->type();
    }
    return "Unknown";
}

SystemStatus ControllerManager::getSystemStatus() const
{
    return m_systemStatus;
}

QStringList ControllerManager::getControllerNames() const
{
    return m_controllers.keys();
}

QStringList ControllerManager::getConnectedControllers() const
{
    QStringList connected;
    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        if (it->enabled && it->status == ControllerStatus::Connected) {
            connected.append(it.key());
        }
    }
    return connected;
}

QStringList ControllerManager::getDisconnectedControllers() const
{
    QStringList disconnected;
    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        if (it->enabled && it->status != ControllerStatus::Connected) {
            disconnected.append(it.key());
        }
    }
    return disconnected;
}

int ControllerManager::getEnabledControllerCount() const
{
    int count = 0;
    for (const auto& info : m_controllers) {
        if (info.enabled) count++;
    }
    return count;
}

int ControllerManager::getConnectedControllerCount() const
{
    int count = 0;
    for (const auto& info : m_controllers) {
        if (info.enabled && info.status == ControllerStatus::Connected) count++;
    }
    return count;
}

CachedValue ControllerManager::getControllerValue(const QString& controllerName, const QString& command) const
{
    if (!m_controllers.contains(controllerName)) return CachedValue();
    return static_cast<MqttController*>(m_controllers[controllerName].controller)->getCachedValue(command);
}

QHash<QString, CachedValue> ControllerManager::getAllControllerValues(const QString& controllerName) const
{
    if (!m_controllers.contains(controllerName)) return QHash<QString, CachedValue>();
    return static_cast<MqttController*>(m_controllers[controllerName].controller)->getAllCachedValues();
}

void ControllerManager::updateControllerStatus(const QString& name, ControllerStatus status)
{
    if (m_controllers.contains(name)) {
        m_controllers[name].status = status;
        emit controllerStatusChanged(name, status);
        updateSystemStatus();
    }
}

void ControllerManager::updateSystemStatus()
{
    int enabledCount = getEnabledControllerCount();
    int connectedCount = getConnectedControllerCount();
    
    SystemStatus newStatus;
    if (enabledCount == 0) {
        newStatus = SystemStatus::Disconnected;
    } else if (connectedCount == enabledCount) {
        newStatus = SystemStatus::AllConnected;
    } else if (connectedCount > 0) {
        newStatus = SystemStatus::PartiallyConnected;
    } else {
        newStatus = SystemStatus::Disconnected;
    }
    
    if (m_systemStatus != newStatus) {
        m_systemStatus = newStatus;
        emit systemStatusChanged(m_systemStatus);
    }
}

void ControllerManager::onControllerConnected() {}
void ControllerManager::onControllerDisconnected() {}
void ControllerManager::onControllerError(const QString& error) {}
void ControllerManager::onControllerDataUpdated(const QString& command, const QString& value) {}
void ControllerManager::onControllerPollError(const QString& command, const QString& error) {}

QString ControllerManager::getControllerNameFromSender() const { return QString(); }

} // namespace ObservatoryMonitor
