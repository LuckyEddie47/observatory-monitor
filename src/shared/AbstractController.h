#ifndef ABSTRACTCONTROLLER_H
#define ABSTRACTCONTROLLER_H

#include <QObject>
#include <QString>
#include <functional>
#include "Types.h"

namespace ObservatoryMonitor {

// Callback type for command responses
// Parameters: command, response, success, errorCode
using ResponseCallback = std::function<void(const QString&, const QString&, bool, int)>;

// Controller status enumeration
enum class ControllerStatus {
    Disconnected,
    Connecting,
    Connected,
    Error
};

class AbstractController : public QObject
{
    Q_OBJECT

public:
    explicit AbstractController(const QString& name, const QString& type, QObject* parent = nullptr)
        : QObject(parent), m_name(name), m_type(type) {}
    virtual ~AbstractController() = default;

    QString name() const { return m_name; }
    QString type() const { return m_type; }
    virtual ControllerStatus status() const = 0;

    virtual void connect() = 0;
    virtual void disconnect() = 0;

    virtual void sendCommand(const QString& command, ResponseCallback callback) = 0;

signals:
    void statusChanged(ControllerStatus status);
    void dataUpdated(const QString& command, const QString& value);
    void errorOccurred(const QString& error);

protected:
    QString m_name;
    QString m_type;
};

} // namespace ObservatoryMonitor

#endif // ABSTRACTCONTROLLER_H
