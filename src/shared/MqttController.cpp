#include "MqttController.h"
#include "Logger.h"

namespace ObservatoryMonitor {

MqttController::MqttController(const ControllerConfig& config, const BrokerConfig& broker,
                               double timeout, int reconnectInterval, QObject* parent)
    : AbstractController(config.name, config.type, parent)
    , m_mqttClient(new MqttClient(this))
    , m_poller(new ControllerPoller(config.name, config.type, m_mqttClient, this))
    , m_status(ControllerStatus::Disconnected)
{
    m_mqttClient->setHostname(broker.host);
    m_mqttClient->setPort(broker.port);
    if (!broker.username.isEmpty()) {
        m_mqttClient->setUsername(broker.username);
        m_mqttClient->setPassword(broker.password);
    }
    m_mqttClient->setTopicPrefix(config.prefix);
    m_mqttClient->setCommandTimeout(static_cast<int>(timeout * 1000));
    m_mqttClient->setReconnectInterval(reconnectInterval * 1000);

    // Poller is already setup in initializer list

    // Connect signals
    QObject::connect(m_mqttClient, &MqttClient::connected, this, &MqttController::onMqttConnected);
    QObject::connect(m_mqttClient, &MqttClient::disconnected, this, &MqttController::onMqttDisconnected);
    QObject::connect(m_mqttClient, &MqttClient::errorOccurred, this, &MqttController::onMqttError);
    QObject::connect(m_poller, &ControllerPoller::dataUpdated, this, &MqttController::onDataUpdated);
}

MqttController::~MqttController()
{
    m_mqttClient->disconnectFromHost();
}

void MqttController::connect()
{
    updateStatus(ControllerStatus::Connecting);
    m_mqttClient->connectToHost();
}

void MqttController::disconnect()
{
    m_mqttClient->disconnectFromHost();
    updateStatus(ControllerStatus::Disconnected);
}

void MqttController::sendCommand(const QString& command, ResponseCallback callback)
{
    m_mqttClient->sendCommand(command, callback);
}

void MqttController::updateConfig(const BrokerConfig& broker, double timeout, int reconnectInterval)
{
    m_mqttClient->setHostname(broker.host);
    m_mqttClient->setPort(static_cast<quint16>(broker.port));
    m_mqttClient->setUsername(broker.username);
    m_mqttClient->setPassword(broker.password);
    m_mqttClient->setCommandTimeout(static_cast<int>(timeout * 1000));
    m_mqttClient->setReconnectInterval(reconnectInterval * 1000);

    // If we are currently connected or connecting, we should reconnect with new settings
    if (m_status == ControllerStatus::Connected || m_status == ControllerStatus::Connecting) {
        disconnect();
        connect();
    }
}

void MqttController::startPolling(int fastPollMs, int slowPollMs)
{
    m_poller->setFastPollInterval(fastPollMs);
    m_poller->setSlowPollInterval(slowPollMs);
    m_poller->startPolling();
}

void MqttController::stopPolling()
{
    m_poller->stopPolling();
}

CachedValue MqttController::getCachedValue(const QString& command) const
{
    return m_poller->getCachedValue(command);
}

QHash<QString, CachedValue> MqttController::getAllCachedValues() const
{
    return m_poller->getAllCachedValues();
}

void MqttController::onMqttConnected()
{
    updateStatus(ControllerStatus::Connected);
}

void MqttController::onMqttDisconnected()
{
    updateStatus(ControllerStatus::Disconnected);
}

void MqttController::onMqttError(const QString& error)
{
    emit errorOccurred(error);
}

void MqttController::onDataUpdated(const QString& command, const QString& value)
{
    emit dataUpdated(command, value);
}

void MqttController::updateStatus(ControllerStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged(m_status);
    }
}

} // namespace ObservatoryMonitor
