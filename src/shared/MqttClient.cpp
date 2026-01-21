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
    , m_queueProcessInterval(100)  // 100ms between commands
    , m_maxQueueSize(100)
    , m_echoSubscription(nullptr)
    , m_reconnectTimer(new QTimer(this))
    , m_queueProcessTimer(new QTimer(this))
    , m_commandSequence(0)
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
    
    // Setup queue process timer
    m_queueProcessTimer->setInterval(m_queueProcessInterval);
    connect(m_queueProcessTimer, &QTimer::timeout, this, &MqttClient::onQueueProcessTimer);
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
    m_commandQueue.clear();
    
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

void MqttClient::setQueueProcessInterval(int intervalMs)
{
    m_queueProcessInterval = intervalMs;
    m_queueProcessTimer->setInterval(intervalMs);
}

void MqttClient::setMaxQueueSize(int maxSize)
{
    m_maxQueueSize = maxSize;
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
    m_queueProcessTimer->stop();
    
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
        Logger::instance().error(QString("MQTT: Cannot queue command '%1' - not connected").arg(command));
        if (callback) {
            callback(command, "", false, -1);
        }
        return;
    }
    
    // Check queue size
    if (m_commandQueue.size() >= m_maxQueueSize) {
        Logger::instance().error(QString("MQTT: Queue overflow - dropping command '%1'").arg(command));
        emit queueOverflow(command);
        if (callback) {
            callback(command, "", false, -1);
        }
        return;
    }
    
    // Generate unique key for this command instance using sequence number
    QString commandKey = QString("%1_SEQ%2").arg(command).arg(m_commandSequence++);
    
    // Add to queue
    m_commandQueue.enqueue(commandKey);
    
    // Create pending command entry
    PendingCommand pending;
    pending.command = command;  // Store original command without sequence
    pending.callback = callback;
    pending.queuedTime = QDateTime::currentMSecsSinceEpoch();
    pending.state = CommandState::Queued;
    pending.sequenceNumber = m_commandSequence - 1;
    
    m_pendingCommands.insert(commandKey, pending);
    
    Logger::instance().debug(QString("MQTT: Queued command '%1' (queue size: %2)").arg(command).arg(m_commandQueue.size()));
    
    // Start queue processor if not running
    if (!m_queueProcessTimer->isActive()) {
        m_queueProcessTimer->start();
    }
}

int MqttClient::queueSize() const
{
    return m_commandQueue.size();
}

int MqttClient::pendingCommandCount() const
{
    int count = 0;
    for (const auto& cmd : m_pendingCommands) {
        if (cmd.state == CommandState::Sent) {
            count++;
        }
    }
    return count;
}

void MqttClient::clearQueue()
{
    Logger::instance().info(QString("MQTT: Clearing command queue (%1 commands)").arg(m_commandQueue.size()));
    
    // Clear queue
    while (!m_commandQueue.isEmpty()) {
        QString cmdKey = m_commandQueue.dequeue();
        
        // Call callbacks with failure
        if (m_pendingCommands.contains(cmdKey)) {
            PendingCommand pending = m_pendingCommands.take(cmdKey);
            if (pending.callback) {
                pending.callback(pending.command, "", false, -1);
            }
        }
    }
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
    
    // Start queue processor
    if (!m_commandQueue.isEmpty()) {
        m_queueProcessTimer->start();
    }
    
    emit connected();
}

void MqttClient::onDisconnected()
{
    Logger::instance().warning("MQTT: Disconnected");
    
    // Stop queue processor
    m_queueProcessTimer->stop();
    
    // Cancel all pending commands
    for (auto it = m_pendingCommands.begin(); it != m_pendingCommands.end(); ++it) {
        if (it->timeoutTimer) {
            it->timeoutTimer->stop();
            delete it->timeoutTimer;
        }
        if (it->callback) {
            it->callback(it->command, "", false, -1);
        }
    }
    m_pendingCommands.clear();
    m_commandQueue.clear();
    
    // Reset sequence number on disconnect
    m_commandSequence = 0;
    
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

void MqttClient::onQueueProcessTimer()
{
    processQueue();
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

void MqttClient::processQueue()
{
    if (m_commandQueue.isEmpty()) {
        m_queueProcessTimer->stop();
        return;
    }
    
    if (!isConnected()) {
        m_queueProcessTimer->stop();
        return;
    }
    
    // Send next command from queue
    QString commandKey = m_commandQueue.dequeue();
    sendQueuedCommand(commandKey);
}

void MqttClient::sendQueuedCommand(const QString& commandKey)
{
    if (!m_pendingCommands.contains(commandKey)) {
        Logger::instance().warning(QString("MQTT: Command key '%1' not found in pending commands").arg(commandKey));
        return;
    }
    
    PendingCommand& pending = m_pendingCommands[commandKey];
    QString command = pending.command;
    
    // Publish to cmd topic
    QString cmdTopic = m_topicPrefix + "/cmd";
    QByteArray message = command.toUtf8();
    
    Logger::instance().debug(QString("MQTT: Publishing to %1: %2").arg(cmdTopic, command));
    
    qint64 msgId = m_client->publish(cmdTopic, message, 1);  // QoS 1
    
    if (msgId == -1) {
        Logger::instance().error(QString("MQTT: Failed to publish command '%1'").arg(command));
        
        PendingCommand failedPending = m_pendingCommands.take(commandKey);
        if (failedPending.callback) {
            failedPending.callback(command, "", false, -1);
        }
        return;
    }
    
    // Update pending command
    pending.sentTime = QDateTime::currentMSecsSinceEpoch();
    pending.state = CommandState::Sent;
    
    // Setup timeout timer
    pending.timeoutTimer = new QTimer(this);
    pending.timeoutTimer->setSingleShot(true);
    pending.timeoutTimer->setInterval(m_commandTimeout);
    
    connect(pending.timeoutTimer, &QTimer::timeout, this, [this, commandKey]() {
        handleCommandTimeout(commandKey);
    });
    
    pending.timeoutTimer->start();
    
    qint64 queueTime = pending.sentTime - pending.queuedTime;
    Logger::instance().debug(QString("MQTT: Command '%1' sent (queued for %2 ms)").arg(command).arg(queueTime));
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
    int errorCode = extractErrorCode(responseValue);
    
    Logger::instance().debug(QString("MQTT: Parsed command='%1', response='%2', errorCode=%3")
                            .arg(command, responseValue).arg(errorCode));
    
    // Find the oldest pending command matching this command string
    QString matchingKey;
    int lowestSequence = -1;
    
    for (auto it = m_pendingCommands.begin(); it != m_pendingCommands.end(); ++it) {
        if (it->command == command && it->state == CommandState::Sent) {
            if (lowestSequence == -1 || it->sequenceNumber < lowestSequence) {
                matchingKey = it.key();
                lowestSequence = it->sequenceNumber;
            }
        }
    }
    
    if (matchingKey.isEmpty()) {
        Logger::instance().debug(QString("MQTT: Received response for non-pending command: %1").arg(command));
        return;
    }
    
    PendingCommand pending = m_pendingCommands.take(matchingKey);
    
    // Stop timeout timer
    if (pending.timeoutTimer) {
        pending.timeoutTimer->stop();
        delete pending.timeoutTimer;
    }
    
    // Calculate response time
    qint64 responseTime = QDateTime::currentMSecsSinceEpoch() - pending.sentTime;
    Logger::instance().debug(QString("MQTT: Command '%1' completed in %2 ms").arg(command).arg(responseTime));
    
    // Interpret error code if present
    bool success = (errorCode == -1 || errorCode == 0);  // -1 = no error code, 0 = success
    if (errorCode > 0) {
        QString errorMsg = interpretErrorCode(errorCode, command);
        Logger::instance().warning(QString("MQTT: Command '%1' returned error %2: %3")
                                  .arg(command).arg(errorCode).arg(errorMsg));
    }
    
    // Call callback
    if (pending.callback) {
        pending.callback(command, responseValue, success, errorCode);
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

int MqttClient::extractErrorCode(const QString& response)
{
    // Check if response is a single digit (error code)
    if (response.length() == 1 && response[0].isDigit()) {
        return response.toInt();
    }
    
    return -1;  // No error code
}

void MqttClient::handleCommandTimeout(const QString& commandKey)
{
    if (!m_pendingCommands.contains(commandKey)) {
        return;
    }
    
    PendingCommand pending = m_pendingCommands.take(commandKey);
    
    Logger::instance().warning(QString("MQTT: Command '%1' timed out after %2 ms").arg(pending.command).arg(m_commandTimeout));
    
    // Timer will be deleted automatically when command is removed
    if (pending.timeoutTimer) {
        delete pending.timeoutTimer;
    }
    
    // Call callback with failure
    if (pending.callback) {
        pending.callback(pending.command, "", false, -1);
    }
}

QString MqttClient::interpretErrorCode(int errorCode, const QString& command)
{
    // Interpret error codes based on OCS lexicon
    // These are from dome goto command (:DS#) but we'll use as general reference
    
    QStringList errorMessages = {
        "Success",                      // 0
        "Below horizon limit",          // 1
        "Above overhead limit",         // 2
        "Controller in standby",        // 3
        "Dome is parked",              // 4
        "Goto in progress",            // 5
        "Outside limits",              // 6
        "Hardware fault",              // 7
        "Already in motion",           // 8
        "Unspecified error"            // 9
    };
    
    if (errorCode >= 0 && errorCode < errorMessages.size()) {
        return errorMessages[errorCode];
    }
    
    return QString("Unknown error code %1").arg(errorCode);
}

} // namespace ObservatoryMonitor
