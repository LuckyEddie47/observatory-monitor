#include "MqttClient.h"
#include "Logger.h"
#include <QDebug>
#include <QDateTime>
#include <QRegularExpression>

namespace ObservatoryMonitor {

MqttClient::MqttClient(QObject* parent)
    : QObject(parent)
    , m_client(new QMqttClient(this))
    , m_commandTimeout(2000)  // 2 seconds default
    , m_reconnectInterval(10000)  // 10 seconds default
    , m_echoSubscription(nullptr)
    , m_reconnectTimer(new QTimer(this))
    , m_autoReconnect(true)
{
    // Connect client signals
    connect(m_client, &QMqttClient::connected, this, &MqttClient::onConnected);
    connect(m_client, &QMqttClient::disconnected, this, &MqttClient::onDisconnected);
    connect(m_client, &QMqttClient::stateChanged, this, &MqttClient::onStateChanged);
    connect(m_client, &QMqttClient::errorChanged, this, &MqttClient::onErrorChanged);
    
    // Setup reconnect timer
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &MqttClient::onReconnectTimer);
}

MqttClient::~MqttClient()
{
    // Cancel all pending commands
    for (auto it = m_pendingCommands.begin(); it != m_pendingCommands.end(); ++it) {
        if (it->timeoutTimer) {
            it->timeoutTimer->stop();
            delete it->timeoutTimer;
        }
    }
    m_pendingCommands.clear();
    
    if (m_client->state() == QMqttClient::Connected) {
        m_client->disconnectFromHost();
    }
}

void MqttClient::setHostname(const QString& hostname)
{
    m_client->setHostname(hostname);
}

void MqttClient::setPort(quint16 port)
{
    m_client->setPort(port);
}

void MqttClient::setUsername(const QString& username)
{
    m_client->setUsername(username);
}

void MqttClient::setPassword(const QString& password)
{
    m_client->setPassword(password);
}

void MqttClient::setTopicPrefix(const QString& prefix)
{
    m_topicPrefix = prefix;
}

void MqttClient::setCommandTimeout(int timeoutMs)
{
    m_commandTimeout = timeoutMs;
}

void MqttClient::setReconnectInterval(int intervalMs)
{
    m_reconnectInterval = intervalMs;
}

void MqttClient::connectToHost()
{
    if (m_client->state() == QMqttClient::Connected) {
        Logger::instance().warning(QString("MQTT: Already connected to %1:%2")
                                  .arg(m_client->hostname())
                                  .arg(m_client->port()));
        return;
    }
    
    Logger::instance().info(QString("MQTT: Connecting to %1:%2 (prefix: %3)")
                           .arg(m_client->hostname())
                           .arg(m_client->port())
                           .arg(m_topicPrefix));
    
    m_client->connectToHost();
}

void MqttClient::disconnectFromHost()
{
    m_autoReconnect = false;
    m_reconnectTimer->stop();
    
    if (m_client->state() != QMqttClient::Disconnected) {
        Logger::instance().info("MQTT: Disconnecting...");
        m_client->disconnectFromHost();
    }
}

bool MqttClient::isConnected() const
{
    return m_client->state() == QMqttClient::Connected;
}

void MqttClient::sendCommand(const QString& command, ResponseCallback callback)
{
    if (!isConnected()) {
        Logger::instance().error(QString("MQTT: Cannot send command '%1' - not connected").arg(command));
        if (callback) {
            callback(command, "", false);
        }
        return;
    }
    
    // Check if command already pending
    if (m_pendingCommands.contains(command)) {
        Logger::instance().warning(QString("MQTT: Command '%1' already pending, ignoring duplicate").arg(command));
        return;
    }
    
    // Publish to cmd topic
    QString cmdTopic = m_topicPrefix + "/cmd";
    QByteArray message = command.toUtf8();
    
    Logger::instance().debug(QString("MQTT: Publishing to %1: %2").arg(cmdTopic, command));
    
    qint64 msgId = m_client->publish(cmdTopic, message, 1);  // QoS 1
    
    if (msgId == -1) {
        Logger::instance().error(QString("MQTT: Failed to publish command '%1'").arg(command));
        if (callback) {
            callback(command, "", false);
        }
        return;
    }
    
    // Track pending command
    PendingCommand pending;
    pending.command = command;
    pending.callback = callback;
    pending.sentTime = QDateTime::currentMSecsSinceEpoch();
    
    // Setup timeout timer
    pending.timeoutTimer = new QTimer(this);
    pending.timeoutTimer->setSingleShot(true);
    pending.timeoutTimer->setInterval(m_commandTimeout);
    
    connect(pending.timeoutTimer, &QTimer::timeout, this, [this, command]() {
        handleCommandTimeout(command);
    });
    
    pending.timeoutTimer->start();
    
    m_pendingCommands.insert(command, pending);
}

QMqttClient::ClientState MqttClient::state() const
{
    return m_client->state();
}

void MqttClient::onConnected()
{
    Logger::instance().info(QString("MQTT: Connected to %1:%2")
                           .arg(m_client->hostname())
                           .arg(m_client->port()));
    
    m_reconnectTimer->stop();
    
    // Subscribe to echo topic
    setupSubscription();
    
    emit connected();
}

void MqttClient::onDisconnected()
{
    Logger::instance().warning("MQTT: Disconnected");
    
    // Cancel all pending commands
    for (auto it = m_pendingCommands.begin(); it != m_pendingCommands.end(); ++it) {
        if (it->timeoutTimer) {
            it->timeoutTimer->stop();
            delete it->timeoutTimer;
        }
        if (it->callback) {
            it->callback(it->command, "", false);
        }
    }
    m_pendingCommands.clear();
    
    emit disconnected();
    
    // Schedule reconnect
    if (m_autoReconnect) {
        Logger::instance().info(QString("MQTT: Reconnecting in %1 seconds...").arg(m_reconnectInterval / 1000));
        m_reconnectTimer->start(m_reconnectInterval);
    }
}

void MqttClient::onStateChanged(QMqttClient::ClientState state)
{
    QString stateStr;
    switch (state) {
        case QMqttClient::Disconnected: stateStr = "Disconnected"; break;
        case QMqttClient::Connecting:   stateStr = "Connecting"; break;
        case QMqttClient::Connected:    stateStr = "Connected"; break;
        default:                        stateStr = "Unknown"; break;
    }
    
    Logger::instance().debug(QString("MQTT: State changed to %1").arg(stateStr));
    emit stateChanged(state);
}

void MqttClient::onErrorChanged(QMqttClient::ClientError error)
{
    if (error == QMqttClient::NoError) {
        return;
    }
    
    QString errorStr;
    switch (error) {
        case QMqttClient::InvalidProtocolVersion:
            errorStr = "Invalid protocol version";
            break;
        case QMqttClient::IdRejected:
            errorStr = "Client ID rejected";
            break;
        case QMqttClient::ServerUnavailable:
            errorStr = "Server unavailable";
            break;
        case QMqttClient::BadUsernameOrPassword:
            errorStr = "Bad username or password";
            break;
        case QMqttClient::NotAuthorized:
            errorStr = "Not authorized";
            break;
        case QMqttClient::TransportInvalid:
            errorStr = "Transport invalid";
            break;
        case QMqttClient::ProtocolViolation:
            errorStr = "Protocol violation";
            break;
        case QMqttClient::UnknownError:
            errorStr = "Unknown error";
            break;
        default:
            errorStr = QString("Error code %1").arg(static_cast<int>(error));
            break;
    }
    
    Logger::instance().error(QString("MQTT: Error - %1").arg(errorStr));
    emit errorOccurred(errorStr);
}

void MqttClient::onMessageReceived(const QMqttMessage& msg)
{
    QString topicStr = msg.topic().name();
    QString messageStr = QString::fromUtf8(msg.payload());
    
    Logger::instance().debug(QString("MQTT: Received on %1: %2").arg(topicStr, messageStr));
    
    // Should be on echo topic
    if (!topicStr.endsWith("/echo")) {
        Logger::instance().warning(QString("MQTT: Unexpected topic: %1").arg(topicStr));
        return;
    }
    
    parseResponse(messageStr);
}

void MqttClient::onReconnectTimer()
{
    Logger::instance().info("MQTT: Attempting reconnect...");
    connectToHost();
}

void MqttClient::setupSubscription()
{
    QString echoTopic = m_topicPrefix + "/echo";
    
    Logger::instance().info(QString("MQTT: Subscribing to %1").arg(echoTopic));
    
    m_echoSubscription = m_client->subscribe(echoTopic, 0);  // QoS 0 for echo
    
    if (!m_echoSubscription) {
        Logger::instance().error(QString("MQTT: Failed to subscribe to %1").arg(echoTopic));
        return;
    }
    
    connect(m_echoSubscription, &QMqttSubscription::messageReceived, 
            this, &MqttClient::onMessageReceived);
}

void MqttClient::parseResponse(const QString& response)
{
    // Expected format: "Received: :DZ#, Response: 306.640#, Source: MQTT"
    // Extract the command and response value
    
    QRegularExpression cmdRegex("Received:\\s*([^,]+)");
    QRegularExpressionMatch cmdMatch = cmdRegex.match(response);
    
    if (!cmdMatch.hasMatch()) {
        Logger::instance().warning(QString("MQTT: Could not parse command from response: %1").arg(response));
        return;
    }
    
    QString command = cmdMatch.captured(1).trimmed();
    QString responseValue = extractResponseValue(response);
    
    Logger::instance().debug(QString("MQTT: Parsed command='%1', response='%2'").arg(command, responseValue));
    
    // Find pending command
    if (!m_pendingCommands.contains(command)) {
        Logger::instance().debug(QString("MQTT: Received response for non-pending command: %1").arg(command));
        return;
    }
    
    PendingCommand pending = m_pendingCommands.take(command);
    
    // Stop timeout timer
    if (pending.timeoutTimer) {
        pending.timeoutTimer->stop();
        delete pending.timeoutTimer;
    }
    
    // Calculate response time
    qint64 responseTime = QDateTime::currentMSecsSinceEpoch() - pending.sentTime;
    Logger::instance().debug(QString("MQTT: Command '%1' completed in %2 ms").arg(command).arg(responseTime));
    
    // Call callback
    if (pending.callback) {
        pending.callback(command, responseValue, true);
    }
}

QString MqttClient::extractResponseValue(const QString& fullResponse)
{
    // Extract value between "Response: " and next "#" or ","
    QRegularExpression respRegex("Response:\\s*([^,#]+)#?");
    QRegularExpressionMatch respMatch = respRegex.match(fullResponse);
    
    if (respMatch.hasMatch()) {
        return respMatch.captured(1).trimmed();
    }
    
    return "";
}

void MqttClient::handleCommandTimeout(const QString& command)
{
    Logger::instance().warning(QString("MQTT: Command '%1' timed out after %2 ms").arg(command).arg(m_commandTimeout));
    
    if (!m_pendingCommands.contains(command)) {
        return;
    }
    
    PendingCommand pending = m_pendingCommands.take(command);
    
    // Timer will be deleted automatically when command is removed
    if (pending.timeoutTimer) {
        delete pending.timeoutTimer;
    }
    
    // Call callback with failure
    if (pending.callback) {
        pending.callback(command, "", false);
    }
}

} // namespace ObservatoryMonitor
