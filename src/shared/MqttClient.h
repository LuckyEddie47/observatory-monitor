#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QHash>
#include <QMqttClient>
#include <QMqttSubscription>
#include <QMqttMessage>
#include <functional>

namespace ObservatoryMonitor {

// Callback type for command responses
// Parameters: command, response, success
using ResponseCallback = std::function<void(const QString&, const QString&, bool)>;

// Structure to track pending commands
struct PendingCommand {
    QString command;
    ResponseCallback callback;
    QTimer* timeoutTimer;
    qint64 sentTime;
    
    PendingCommand() : timeoutTimer(nullptr), sentTime(0) {}
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
    
    // Connection management
    void connectToHost();
    void disconnectFromHost();
    bool isConnected() const;
    
    // Send command and receive response
    void sendCommand(const QString& command, ResponseCallback callback);
    
    // Get connection state
    QMqttClient::ClientState state() const;
    
signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);
    void stateChanged(QMqttClient::ClientState state);
    
private slots:
    void onConnected();
    void onDisconnected();
    void onStateChanged(QMqttClient::ClientState state);
    void onErrorChanged(QMqttClient::ClientError error);
    void onMessageReceived(const QMqttMessage& msg);
    void onReconnectTimer();
    
private:
    void setupSubscription();
    void parseResponse(const QString& response);
    QString extractResponseValue(const QString& fullResponse);
    void handleCommandTimeout(const QString& command);
    
    QMqttClient* m_client;
    QString m_topicPrefix;
    int m_commandTimeout;
    int m_reconnectInterval;
    
    QMqttSubscription* m_echoSubscription;
    QTimer* m_reconnectTimer;
    
    // Track pending commands
    QHash<QString, PendingCommand> m_pendingCommands;
    
    bool m_autoReconnect;
};

} // namespace ObservatoryMonitor

#endif // MQTTCLIENT_H
