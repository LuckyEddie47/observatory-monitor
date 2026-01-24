#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QQueue>
#include <QHash>
#include <QPointer>
#include <QMqttClient>
#include <QMqttSubscription>
#include <QMqttMessage>
#include <functional>

namespace ObservatoryMonitor {

// Callback type for command responses
// Parameters: command, response, success, errorCode
using ResponseCallback = std::function<void(const QString&, const QString&, bool, int)>;

// Command state enumeration
enum class CommandState {
    Queued,      // Waiting in queue
    Sent,        // Sent to broker, waiting for response
    Responded,   // Response received
    TimedOut,    // No response within timeout
    Error        // Error occurred
};

// Structure to track pending commands
struct PendingCommand {
    QString command;
    ResponseCallback callback;
    QPointer<QTimer> timeoutTimer;
    qint64 queuedTime;
    qint64 sentTime;
    CommandState state;
    int sequenceNumber;
    
    PendingCommand() : timeoutTimer(nullptr), queuedTime(0), sentTime(0), state(CommandState::Queued), sequenceNumber(0) {}
};

class MqttClient : public QObject
{
    Q_OBJECT

public:
    explicit MqttClient(QObject* parent = nullptr);
    ~MqttClient();
    
    // Configuration
    void setHostname(const QString& hostname);
    void setPort(quint16 port);
    void setUsername(const QString& username);
    void setPassword(const QString& password);
    void setTopicPrefix(const QString& prefix);
    void setCommandTimeout(int timeoutMs);
    void setReconnectInterval(int intervalMs);
    void setQueueProcessInterval(int intervalMs);  // Rate limiting
    void setMaxQueueSize(int maxSize);
    
    // Connection management
    void connectToHost();
    void disconnectFromHost();
    bool isConnected() const;
    
    // Send command (adds to queue)
    void sendCommand(const QString& command, ResponseCallback callback);
    
    // Queue status
    int queueSize() const;
    int pendingCommandCount() const;
    void clearQueue();
    
    // Get connection state
    QMqttClient::ClientState state() const;
    
    // Access underlying client (for simulator)
    QMqttClient* client() { return m_client; }
    
signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);
    void stateChanged(QMqttClient::ClientState state);
    void queueOverflow(const QString& command);
    void responseReceived(const QString& command, const QString& response, bool isUnsolicited);
    
private slots:
    void onConnected();
    void onDisconnected();
    void onStateChanged(QMqttClient::ClientState state);
    void onErrorChanged(QMqttClient::ClientError error);
    void onMessageReceived(const QMqttMessage& msg);
    void onReconnectTimer();
    void onQueueProcessTimer();
    
private:
    void setupSubscription();
    void processQueue();
    void sendQueuedCommand(const QString& commandKey);
    void parseResponse(const QString& response);
    QString extractResponseValue(const QString& fullResponse);
    int extractErrorCode(const QString& response);
    void handleCommandTimeout(const QString& commandKey);
    QString interpretErrorCode(int errorCode, const QString& command);
    
    QMqttClient* m_client;
    QString m_topicPrefix;
    int m_commandTimeout;
    int m_reconnectInterval;
    int m_queueProcessInterval;
    int m_maxQueueSize;
    
    QMqttSubscription* m_echoSubscription;
    QTimer* m_reconnectTimer;
    QTimer* m_queueProcessTimer;
    
    // Command queue
    QQueue<QString> m_commandQueue;
    
    // Track pending commands (sent but not responded)
    QHash<QString, PendingCommand> m_pendingCommands;
    
    // Sequence number for unique command identification
    int m_commandSequence;
    
    bool m_autoReconnect;
};

} // namespace ObservatoryMonitor

#endif // MQTTCLIENT_H
