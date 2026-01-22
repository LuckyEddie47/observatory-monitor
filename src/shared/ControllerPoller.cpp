#include "ControllerPoller.h"
#include "Logger.h"
#include <QDebug>

namespace ObservatoryMonitor {

ControllerPoller::ControllerPoller(MqttClient* mqttClient, QObject* parent)
    : QObject(parent)
    , m_mqttClient(mqttClient)
    , m_controllerName("Unknown")
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
    // Hardcoded fast poll commands (movement parameters)
    m_fastPollCommands << ":DZ#";  // Dome Azimuth
    // Future: Add telescope altitude, azimuth when OnStepX is implemented
    
    // Hardcoded slow poll commands (status parameters)
    m_slowPollCommands << ":RS#";  // Roof/Shutter Status
    // Future: Add weather data, switch states, etc.
    
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
}

ControllerPoller::~ControllerPoller()
{
    stopPolling();
}

void ControllerPoller::setControllerName(const QString& name)
{
    m_controllerName = name;
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
        Logger::instance().warning(QString("Poller[%1]: Already polling").arg(m_controllerName));
        return;
    }
    
    Logger::instance().info(QString("Poller[%1]: Starting polling (fast: %2ms, slow: %3ms)")
                           .arg(m_controllerName)
                           .arg(m_fastPollInterval)
                           .arg(m_slowPollInterval));
    
    m_isPolling = true;
    
    // Start timers only if connected
    if (m_mqttClient->isConnected()) {
        // Poll immediately on start
        pollFastCommands();
        pollSlowCommands();
        
        m_fastPollTimer->start();
        m_slowPollTimer->start();
        m_staleCheckTimer->start();
    } else {
        Logger::instance().info(QString("Poller[%1]: Waiting for MQTT connection before starting timers").arg(m_controllerName));
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
    return CachedValue();  // Invalid value
}

QHash<QString, CachedValue> ControllerPoller::getAllCachedValues() const
{
    return m_cache;
}

bool ControllerPoller::isDataStale(const QString& command) const
{
    if (!m_cache.contains(command)) {
        return true;  // No data is stale
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
    Logger::instance().info(QString("Poller[%1]: MQTT connected").arg(m_controllerName));
    
    if (m_isPolling) {
        // Poll immediately on connection
        pollFastCommands();
        pollSlowCommands();
        
        // Start timers
        m_fastPollTimer->start();
        m_slowPollTimer->start();
        m_staleCheckTimer->start();
    }
}

void ControllerPoller::onMqttDisconnected()
{
    Logger::instance().warning(QString("Poller[%1]: MQTT disconnected, stopping timers").arg(m_controllerName));
    
    // Stop timers but keep m_isPolling true so we restart when reconnected
    m_fastPollTimer->stop();
    m_slowPollTimer->stop();
    m_staleCheckTimer->stop();
    
    // Mark all cached data as invalid
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        it->valid = false;
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
    QString pollType = isFastPoll ? "fast" : "slow";
    
    Logger::instance().debug(QString("Poller[%1]: Polling %2 command: %3")
                            .arg(m_controllerName, pollType, command));
    
    m_mqttClient->sendCommand(command, [this, command](const QString& cmd, const QString& response, bool success, int errorCode) {
        if (success) {
            // Update cache
            m_cache[command] = CachedValue(response);
            m_successfulPolls++;
            
            Logger::instance().debug(QString("Poller[%1]: %2 = %3")
                                    .arg(m_controllerName, command, response));
            
            emit dataUpdated(command, response);
        } else {
            m_failedPolls++;
            
            if (errorCode > 0) {
                Logger::instance().warning(QString("Poller[%1]: Command %2 failed with error code %3")
                                          .arg(m_controllerName, command).arg(errorCode));
                emit pollError(command, QString("Error code %1").arg(errorCode));
            } else {
                Logger::instance().warning(QString("Poller[%1]: Command %2 failed (timeout or connection issue)")
                                          .arg(m_controllerName, command));
                emit pollError(command, "Timeout or connection issue");
            }
            
            // Mark cached value as invalid on error
            if (m_cache.contains(command)) {
                m_cache[command].valid = false;
            }
        }
    });
}

void ControllerPoller::checkStaleData()
{
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        const QString& command = it.key();
        
        if (isDataStale(command)) {
            Logger::instance().debug(QString("Poller[%1]: Data for %2 is stale")
                                    .arg(m_controllerName, command));
            emit dataStale(command);
        }
    }
}

int ControllerPoller::getStaleThreshold(const QString& command) const
{
    // Determine stale threshold based on whether it's a fast or slow poll command
    int pollInterval = m_slowPollInterval;  // Default to slow
    
    if (m_fastPollCommands.contains(command)) {
        pollInterval = m_fastPollInterval;
    }
    
    return pollInterval * m_staleDataMultiplier;
}

} // namespace ObservatoryMonitor
