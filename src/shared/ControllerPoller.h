#ifndef CONTROLLERPOLLER_H
#define CONTROLLERPOLLER_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QDateTime>
#include <QTimer>
#include "MqttClient.h"

namespace ObservatoryMonitor {

// Structure to hold cached values with metadata
struct CachedValue {
    QString value;
    QDateTime timestamp;
    bool valid;
    
    CachedValue() : valid(false) {}
    CachedValue(const QString& val) : value(val), timestamp(QDateTime::currentDateTime()), valid(true) {}
};

class ControllerPoller : public QObject
{
    Q_OBJECT

public:
    explicit ControllerPoller(MqttClient* mqttClient, QObject* parent = nullptr);
    ~ControllerPoller();
    
    // Configuration
    void setControllerName(const QString& name);
    void setFastPollInterval(int intervalMs);
    void setSlowPollInterval(int intervalMs);
    void setStaleDataMultiplier(int multiplier);  // Data is stale after multiplier * poll_interval
    
    // Polling control
    void startPolling();
    void stopPolling();
    bool isPolling() const;
    
    // Data access
    CachedValue getCachedValue(const QString& command) const;
    QHash<QString, CachedValue> getAllCachedValues() const;
    bool isDataStale(const QString& command) const;
    
    // Statistics
    int successfulPolls() const { return m_successfulPolls; }
    int failedPolls() const { return m_failedPolls; }
    
signals:
    void dataUpdated(const QString& command, const QString& value);
    void dataStale(const QString& command);
    void pollError(const QString& command, const QString& error);
    
private slots:
    void onFastPollTimer();
    void onSlowPollTimer();
    void onMqttConnected();
    void onMqttDisconnected();
    
private:
    void pollFastCommands();
    void pollSlowCommands();
    void pollCommand(const QString& command, bool isFastPoll);
    void checkStaleData();
    int getStaleThreshold(const QString& command) const;
    
    MqttClient* m_mqttClient;
    QString m_controllerName;
    
    QTimer* m_fastPollTimer;
    QTimer* m_slowPollTimer;
    QTimer* m_staleCheckTimer;
    
    int m_fastPollInterval;      // milliseconds
    int m_slowPollInterval;      // milliseconds
    int m_staleDataMultiplier;   // multiplier for stale detection
    
    // Hardcoded command lists
    QStringList m_fastPollCommands;  // Movement parameters
    QStringList m_slowPollCommands;  // Status parameters
    
    // Data cache
    QHash<QString, CachedValue> m_cache;
    
    // Statistics
    int m_successfulPolls;
    int m_failedPolls;
    
    bool m_isPolling;
};

} // namespace ObservatoryMonitor

#endif // CONTROLLERPOLLER_H
