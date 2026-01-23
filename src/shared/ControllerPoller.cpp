#include "ControllerPoller.h"
#include "Logger.h"
#include <QDebug>

namespace ObservatoryMonitor {

ControllerPoller::ControllerPoller(const QString& name, const QString& type, MqttClient* mqttClient, QObject* parent)
    : QObject(parent)
    , m_mqttClient(mqttClient)
    , m_controllerName(name)
    , m_controllerType(type)
    , m_fastPollTimer(new QTimer(this))
    , m_slowPollTimer(new QTimer(this))
    , m_staleCheckTimer(new QTimer(this))
    , m_fastPollInterval(1000)      // 1 second default
    , m_slowPollInterval(10000)     // 10 seconds default
    , m_staleDataMultiplier(3)      // Data stale after 3x poll interval
    , m_successfulPolls(0)
    , m_failedPolls(0)
    , m_isPolling(false)
{
    // Setup default commands
    setControllerType(type);
    
    // Setup timers
    m_fastPollTimer->setInterval(m_fastPollInterval);
    connect(m_fastPollTimer, &QTimer::timeout, this, &ControllerPoller::onFastPollTimer);
    
    m_slowPollTimer->setInterval(m_slowPollInterval);
    connect(m_slowPollTimer, &QTimer::timeout, this, &ControllerPoller::onSlowPollTimer);
    
    // Stale data check every 5 seconds
    m_staleCheckTimer->setInterval(5000);
    connect(m_staleCheckTimer, &QTimer::timeout, this, &ControllerPoller::checkStaleData);
    
    // Connect to MQTT client signals
    connect(m_mqttClient, &MqttClient::connected, this, &ControllerPoller::onMqttConnected);
    connect(m_mqttClient, &MqttClient::disconnected, this, &ControllerPoller::onMqttDisconnected);
    connect(m_mqttClient, &MqttClient::responseReceived, this, &ControllerPoller::onResponseReceived);
}

ControllerPoller::~ControllerPoller()
{
    stopPolling();
}

void ControllerPoller::setControllerName(const QString& name)
{
    m_controllerName = name;
}

void ControllerPoller::setControllerType(const QString& type)
{
    m_controllerType = type;
    
    m_fastPollCommands.clear();
    m_slowPollCommands.clear();
    
    if (type.toLower() == "observatory" || type.toLower() == "ocs") {
        m_fastPollCommands << ":DZ#";  // Dome Azimuth
        m_slowPollCommands << ":RS#";  // Roof/Shutter Status
    } else if (type.toLower() == "telescope" || type.toLower() == "onstepx") {
        m_fastPollCommands << ":GR#" << ":GD#";  // RA/Dec
        m_fastPollCommands << ":GZ#" << ":GA#";  // Alt/Az
        m_slowPollCommands << ":GS#";  // Side of pier
    } else {
        // Default minimal set
        m_fastPollCommands << ":DZ#";
        m_slowPollCommands << ":RS#";
    }
}

void ControllerPoller::setFastPollInterval(int intervalMs)
{
    m_fastPollInterval = intervalMs;
    m_fastPollTimer->setInterval(intervalMs);
}

void ControllerPoller::setSlowPollInterval(int intervalMs)
{
    m_slowPollInterval = intervalMs;
    m_slowPollTimer->setInterval(intervalMs);
}

void ControllerPoller::setStaleDataMultiplier(int multiplier)
{
    m_staleDataMultiplier = multiplier;
}

void ControllerPoller::startPolling()
{
    if (m_isPolling) {
        return;
    }
    
    Logger::instance().info(QString("Poller[%1]: Starting polling (fast: %2ms, slow: %3ms)")
                           .arg(m_controllerName)
                           .arg(m_fastPollInterval)
                           .arg(m_slowPollInterval));
    
    m_isPolling = true;
    
    if (m_mqttClient->isConnected()) {
        pollFastCommands();
        pollSlowCommands();
        
        m_fastPollTimer->start();
        m_slowPollTimer->start();
        m_staleCheckTimer->start();
    }
}

void ControllerPoller::stopPolling()
{
    if (!m_isPolling) {
        return;
    }
    
    Logger::instance().info(QString("Poller[%1]: Stopping polling").arg(m_controllerName));
    
    m_fastPollTimer->stop();
    m_slowPollTimer->stop();
    m_staleCheckTimer->stop();
    
    m_isPolling = false;
}

bool ControllerPoller::isPolling() const
{
    return m_isPolling;
}

CachedValue ControllerPoller::getCachedValue(const QString& command) const
{
    if (m_cache.contains(command)) {
        return m_cache[command];
    }
    return CachedValue();
}

QHash<QString, CachedValue> ControllerPoller::getAllCachedValues() const
{
    return m_cache;
}

bool ControllerPoller::isDataStale(const QString& command) const
{
    if (!m_cache.contains(command)) {
        return true;
    }
    
    const CachedValue& cached = m_cache[command];
    if (!cached.valid) {
        return true;
    }
    
    int threshold = getStaleThreshold(command);
    qint64 age = cached.timestamp.msecsTo(QDateTime::currentDateTime());
    
    return age > threshold;
}

void ControllerPoller::onFastPollTimer()
{
    pollFastCommands();
}

void ControllerPoller::onSlowPollTimer()
{
    pollSlowCommands();
}

void ControllerPoller::onMqttConnected()
{
    if (m_isPolling) {
        pollFastCommands();
        pollSlowCommands();
        
        m_fastPollTimer->start();
        m_slowPollTimer->start();
        m_staleCheckTimer->start();
    }
}

void ControllerPoller::onMqttDisconnected()
{
    m_fastPollTimer->stop();
    m_slowPollTimer->stop();
    m_staleCheckTimer->stop();
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        it->valid = false;
    }
}

void ControllerPoller::onResponseReceived(const QString& command, const QString& response, bool isUnsolicited)
{
    if (isUnsolicited) {
        Logger::instance().debug(QString("Poller[%1]: Handling unsolicited update for %2: %3")
                                .arg(m_controllerName, command, response));
        m_cache[command] = CachedValue(response);
        emit dataUpdated(command, response);
    }
}

void ControllerPoller::pollFastCommands()
{
    for (const QString& command : m_fastPollCommands) {
        pollCommand(command, true);
    }
}

void ControllerPoller::pollSlowCommands()
{
    for (const QString& command : m_slowPollCommands) {
        pollCommand(command, false);
    }
}

void ControllerPoller::pollCommand(const QString& command, bool isFastPoll)
{
    m_mqttClient->sendCommand(command, [this, command](const QString& cmd, const QString& response, bool success, int errorCode) {
        if (success) {
            m_cache[command] = CachedValue(response);
            m_successfulPolls++;
            emit dataUpdated(command, response);
        } else {
            m_failedPolls++;
            QString errorStr = errorCode > 0 ? QString("Error %1").arg(errorCode) : "Timeout";
            Logger::instance().debug(QString("Poller[%1]: Poll failed for %2 - %3").arg(m_controllerName, command, errorStr));
            emit pollError(command, errorStr);
            if (m_cache.contains(command)) {
                m_cache[command].valid = false;
            }
        }
    });
}

void ControllerPoller::checkStaleData()
{
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (isDataStale(it.key())) {
            emit dataStale(it.key());
        }
    }
}

int ControllerPoller::getStaleThreshold(const QString& command) const
{
    int pollInterval = m_fastPollCommands.contains(command) ? m_fastPollInterval : m_slowPollInterval;
    return pollInterval * m_staleDataMultiplier;
}

} // namespace ObservatoryMonitor
