#include "ControllerManager.h"
#include "Logger.h"
#include <QDebug>

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
    // Clean up all controllers
    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        if (it->poller) {
            delete it->poller;
        }
        if (it->mqttClient) {
            delete it->mqttClient;
        }
    }
    m_controllers.clear();
}

void ControllerManager::loadControllersFromConfig(const Config& config)
{
    Logger::instance().info("ControllerManager: Loading controllers from configuration");
    
    // Clear existing controllers
    disconnectAll();
    stopPolling();
    
    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        if (it->poller) delete it->poller;
        if (it->mqttClient) delete it->mqttClient;
    }
    m_controllers.clear();
    
    // Add controllers from config
    for (const auto& ctrl : config.controllers()) {
        addController(ctrl, config.broker(), config.mqttTimeout(), config.reconnectInterval());
    }
    
    Logger::instance().info(QString("ControllerManager: Loaded %1 controller(s), %2 enabled")
                           .arg(m_controllers.size())
                           .arg(getEnabledControllerCount()));
}

void ControllerManager::addController(const ControllerConfig& config, const BrokerConfig& broker, double timeout, int reconnectInterval)
{
    if (m_controllers.contains(config.name)) {
        Logger::instance().warning(QString("ControllerManager: Controller '%1' already exists, skipping").arg(config.name));
        return;
    }
    
    Logger::instance().info(QString("ControllerManager: Adding controller '%1' (prefix: %2, enabled: %3)")
                           .arg(config.name)
                           .arg(config.prefix)
                           .arg(config.enabled ? "true" : "false"));
    
    ControllerInfo info;
    info.name = config.name;
    info.prefix = config.prefix;
    info.enabled = config.enabled;
    info.status = ControllerStatus::Disconnected;
    
    // Create MQTT client
    info.mqttClient = new MqttClient(this);
    info.mqttClient->setHostname(broker.host);
    info.mqttClient->setPort(broker.port);
    if (!broker.username.isEmpty()) {
        info.mqttClient->setUsername(broker.username);
        info.mqttClient->setPassword(broker.password);
    }
    info.mqttClient->setTopicPrefix(config.prefix);
    info.mqttClient->setCommandTimeout(timeout * 1000);  // Convert to ms
    info.mqttClient->setReconnectInterval(reconnectInterval * 1000);
    
    // Connect MQTT signals
    connect(info.mqttClient, &MqttClient::connected, this, &ControllerManager::onControllerConnected);
    connect(info.mqttClient, &MqttClient::disconnected, this, &ControllerManager::onControllerDisconnected);
    connect(info.mqttClient, &MqttClient::errorOccurred, this, &ControllerManager::onControllerError);
    
    // Create poller
    info.poller = new ControllerPoller(info.mqttClient, this);
    info.poller->setControllerName(config.name);
    
    // Connect poller signals
    connect(info.poller, &ControllerPoller::dataUpdated, this, &ControllerManager::onControllerDataUpdated);
    connect(info.poller, &ControllerPoller::pollError, this, &ControllerManager::onControllerPollError);
    
    m_controllers.insert(config.name, info);
    
    Logger::instance().debug(QString("ControllerManager: Controller '%1' added, enabled=%2, total controllers=%3")
                            .arg(config.name)
                            .arg(info.enabled ? "true" : "false")
                            .arg(m_controllers.size()));
    
    updateSystemStatus();
}

void ControllerManager::removeController(const QString& name)
{
    if (!m_controllers.contains(name)) {
        Logger::instance().warning(QString("ControllerManager: Controller '%1' not found").arg(name));
        return;
    }
    
    Logger::instance().info(QString("ControllerManager: Removing controller '%1'").arg(name));
    
    ControllerInfo info = m_controllers.take(name);
    
    if (info.poller) {
        info.poller->stopPolling();
        delete info.poller;
    }
    
    if (info.mqttClient) {
        info.mqttClient->disconnectFromHost();
        delete info.mqttClient;
    }
    
    updateSystemStatus();
}

void ControllerManager::enableController(const QString& name, bool enable)
{
    if (!m_controllers.contains(name)) {
        Logger::instance().warning(QString("ControllerManager: Controller '%1' not found").arg(name));
        return;
    }
    
    ControllerInfo& info = m_controllers[name];
    
    if (info.enabled == enable) {
        return;  // No change
    }
    
    Logger::instance().info(QString("ControllerManager: %1 controller '%2'")
                           .arg(enable ? "Enabling" : "Disabling")
                           .arg(name));
    
    info.enabled = enable;
    
    if (enable) {
        // Connect if currently disconnected
        if (info.mqttClient->state() == QMqttClient::Disconnected) {
            info.mqttClient->connectToHost();
        }
        
        // Start polling if manager is polling
        if (m_isPolling) {
            info.poller->startPolling();
        }
    } else {
        // Stop polling and disconnect
        info.poller->stopPolling();
        info.mqttClient->disconnectFromHost();
    }
    
    updateSystemStatus();
}

void ControllerManager::connectAll()
{
    Logger::instance().info("ControllerManager: Connecting all enabled controllers");
    
    int connectedCount = 0;
    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        Logger::instance().debug(QString("ControllerManager: Checking controller '%1', enabled=%2")
                                .arg(it.key())
                                .arg(it->enabled ? "true" : "false"));
        if (it->enabled) {
            it->mqttClient->connectToHost();
            connectedCount++;
        }
    }
    
    Logger::instance().info(QString("ControllerManager: Initiated connection for %1 enabled controller(s)").arg(connectedCount));
}

void ControllerManager::disconnectAll()
{
    Logger::instance().info("ControllerManager: Disconnecting all controllers");
    
    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        it->mqttClient->disconnectFromHost();
    }
}

void ControllerManager::connectController(const QString& name)
{
    if (!m_controllers.contains(name)) {
        Logger::instance().warning(QString("ControllerManager: Controller '%1' not found").arg(name));
        return;
    }
    
    ControllerInfo& info = m_controllers[name];
    
    if (!info.enabled) {
        Logger::instance().warning(QString("ControllerManager: Controller '%1' is disabled, cannot connect").arg(name));
        return;
    }
    
    Logger::instance().info(QString("ControllerManager: Connecting controller '%1'").arg(name));
    info.mqttClient->connectToHost();
}

void ControllerManager::disconnectController(const QString& name)
{
    if (!m_controllers.contains(name)) {
        Logger::instance().warning(QString("ControllerManager: Controller '%1' not found").arg(name));
        return;
    }
    
    Logger::instance().info(QString("ControllerManager: Disconnecting controller '%1'").arg(name));
    m_controllers[name].mqttClient->disconnectFromHost();
}

void ControllerManager::startPolling(int fastPollMs, int slowPollMs)
{
    m_fastPollInterval = fastPollMs;
    m_slowPollInterval = slowPollMs;
    m_isPolling = true;
    
    Logger::instance().info(QString("ControllerManager: Starting polling (fast: %1ms, slow: %2ms)")
                           .arg(fastPollMs)
                           .arg(slowPollMs));
    
    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        if (it->enabled) {
            it->poller->setFastPollInterval(fastPollMs);
            it->poller->setSlowPollInterval(slowPollMs);
            it->poller->startPolling();
        }
    }
}

void ControllerManager::stopPolling()
{
    if (!m_isPolling) {
        return;
    }
    
    Logger::instance().info("ControllerManager: Stopping polling");
    
    m_isPolling = false;
    
    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        it->poller->stopPolling();
    }
}

void ControllerManager::startControllerPolling(const QString& name)
{
    if (!m_controllers.contains(name)) {
        Logger::instance().warning(QString("ControllerManager: Controller '%1' not found").arg(name));
        return;
    }
    
    ControllerInfo& info = m_controllers[name];
    
    if (!info.enabled) {
        Logger::instance().warning(QString("ControllerManager: Controller '%1' is disabled").arg(name));
        return;
    }
    
    Logger::instance().info(QString("ControllerManager: Starting polling for '%1'").arg(name));
    info.poller->startPolling();
}

void ControllerManager::stopControllerPolling(const QString& name)
{
    if (!m_controllers.contains(name)) {
        Logger::instance().warning(QString("ControllerManager: Controller '%1' not found").arg(name));
        return;
    }
    
    Logger::instance().info(QString("ControllerManager: Stopping polling for '%1'").arg(name));
    m_controllers[name].poller->stopPolling();
}

ControllerStatus ControllerManager::getControllerStatus(const QString& name) const
{
    if (!m_controllers.contains(name)) {
        return ControllerStatus::Disconnected;
    }
    
    return m_controllers[name].status;
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
    
    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        Logger::instance().debug(QString("getEnabledControllerCount: Checking '%1', enabled=%2")
                                .arg(it.key())
                                .arg(it->enabled ? "true" : "false"));
        if (it->enabled) {
            count++;
        }
    }
    
    Logger::instance().debug(QString("getEnabledControllerCount: returning %1").arg(count));
    return count;
}

int ControllerManager::getConnectedControllerCount() const
{
    int count = 0;
    
    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        if (it->enabled && it->status == ControllerStatus::Connected) {
            count++;
        }
    }
    
    return count;
}

CachedValue ControllerManager::getControllerValue(const QString& controllerName, const QString& command) const
{
    if (!m_controllers.contains(controllerName)) {
        return CachedValue();
    }
    
    return m_controllers[controllerName].poller->getCachedValue(command);
}

QHash<QString, CachedValue> ControllerManager::getAllControllerValues(const QString& controllerName) const
{
    if (!m_controllers.contains(controllerName)) {
        return QHash<QString, CachedValue>();
    }
    
    return m_controllers[controllerName].poller->getAllCachedValues();
}

void ControllerManager::onControllerConnected()
{
    QString name = getControllerNameFromSender();
    if (name.isEmpty()) return;
    
    Logger::instance().info(QString("ControllerManager: Controller '%1' connected").arg(name));
    updateControllerStatus(name, ControllerStatus::Connected);
}

void ControllerManager::onControllerDisconnected()
{
    QString name = getControllerNameFromSender();
    if (name.isEmpty()) return;
    
    Logger::instance().warning(QString("ControllerManager: Controller '%1' disconnected").arg(name));
    updateControllerStatus(name, ControllerStatus::Disconnected);
}

void ControllerManager::onControllerError(const QString& error)
{
    QString name = getControllerNameFromSender();
    if (name.isEmpty()) return;
    
    Logger::instance().error(QString("ControllerManager: Controller '%1' error: %2").arg(name, error));
    updateControllerStatus(name, ControllerStatus::Error);
    emit controllerError(name, error);
}

void ControllerManager::onControllerDataUpdated(const QString& command, const QString& value)
{
    QString name = getControllerNameFromSender();
    if (name.isEmpty()) return;
    
    emit controllerDataUpdated(name, command, value);
}

void ControllerManager::onControllerPollError(const QString& command, const QString& error)
{
    QString name = getControllerNameFromSender();
    if (name.isEmpty()) return;
    
    emit controllerError(name, QString("Poll error for %1: %2").arg(command, error));
}

void ControllerManager::updateControllerStatus(const QString& name, ControllerStatus status)
{
    if (!m_controllers.contains(name)) {
        return;
    }
    
    ControllerInfo& info = m_controllers[name];
    
    if (info.status == status) {
        return;  // No change
    }
    
    info.status = status;
    emit controllerStatusChanged(name, status);
    
    updateSystemStatus();
}

void ControllerManager::updateSystemStatus()
{
    int enabledCount = getEnabledControllerCount();
    int connectedCount = getConnectedControllerCount();
    
    Logger::instance().debug(QString("updateSystemStatus: enabled=%1, connected=%2").arg(enabledCount).arg(connectedCount));
    
    SystemStatus newStatus;
    
    if (enabledCount == 0) {
        newStatus = SystemStatus::Disconnected;  // No enabled controllers
    } else if (connectedCount == 0) {
        newStatus = SystemStatus::Disconnected;  // RED - none connected
    } else if (connectedCount == enabledCount) {
        newStatus = SystemStatus::AllConnected;  // GREEN - all connected
    } else {
        newStatus = SystemStatus::PartiallyConnected;  // YELLOW - some connected
    }
    
    if (m_systemStatus != newStatus) {
        m_systemStatus = newStatus;
        
        QString statusStr;
        switch (newStatus) {
            case SystemStatus::AllConnected:
                statusStr = "🟢 GREEN - All controllers connected";
                break;
            case SystemStatus::PartiallyConnected:
                statusStr = "🟡 YELLOW - Partially connected";
                break;
            case SystemStatus::Disconnected:
                statusStr = "🔴 RED - Disconnected";
                break;
        }
        
        Logger::instance().info(QString("ControllerManager: System status changed: %1 (%2/%3)")
                               .arg(statusStr)
                               .arg(connectedCount)
                               .arg(enabledCount));
        
        emit systemStatusChanged(newStatus);
    }
}

QString ControllerManager::getControllerNameFromSender() const
{
    QObject* senderObj = sender();
    if (!senderObj) {
        return QString();
    }
    
    // Find controller by matching MQTT client or poller
    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        if (it->mqttClient == senderObj || it->poller == senderObj) {
            return it.key();
        }
    }
    
    return QString();
}

} // namespace ObservatoryMonitor
