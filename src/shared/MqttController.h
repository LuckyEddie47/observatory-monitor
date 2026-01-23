#ifndef MQTTCONTROLLER_H
#define MQTTCONTROLLER_H

#include "AbstractController.h"
#include "MqttClient.h"
#include "ControllerPoller.h"
#include "Config.h"

namespace ObservatoryMonitor {

class MqttController : public AbstractController
{
    Q_OBJECT

public:
    MqttController(const ControllerConfig& config, const BrokerConfig& broker, 
                   double timeout, int reconnectInterval, QObject* parent = nullptr);
    ~MqttController() override;

    ControllerStatus status() const override { return m_status; }

    void connect() override;
    void disconnect() override;

    void sendCommand(const QString& command, ResponseCallback callback) override;

    void startPolling(int fastPollMs, int slowPollMs);
    void stopPolling();

    // Accessors for polling data
    CachedValue getCachedValue(const QString& command) const;
    QHash<QString, CachedValue> getAllCachedValues() const;

private slots:
    void onMqttConnected();
    void onMqttDisconnected();
    void onMqttError(const QString& error);
    void onDataUpdated(const QString& command, const QString& value);

private:
    void updateStatus(ControllerStatus status);

    MqttClient* m_mqttClient;
    ControllerPoller* m_poller;
    ControllerStatus m_status;
};

} // namespace ObservatoryMonitor

#endif // MQTTCONTROLLER_H
