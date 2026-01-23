#include "ControllerProxy.h"
#include <QRegularExpression>

namespace ObservatoryMonitor {

ControllerProxy::ControllerProxy(const QString& name, ControllerManager* manager, QObject* parent)
    : QObject(parent)
    , m_name(name)
    , m_manager(manager)
    , m_azimuth(0.0)
    , m_altitude(0.0)
    , m_ra(0.0)
    , m_dec(0.0)
{
    if (m_manager) {
        connect(m_manager, &ControllerManager::controllerDataUpdated,
                this, &ControllerProxy::onDataUpdated);
        connect(m_manager, &ControllerManager::controllerStatusChanged,
                this, &ControllerProxy::onStatusChanged);
    }
}

QString ControllerProxy::status() const
{
    if (!m_manager) return "Unknown";
    ControllerStatus s = m_manager->getControllerStatus(m_name);
    switch (s) {
        case ControllerStatus::Disconnected: return "Disconnected";
        case ControllerStatus::Connecting:   return "Connecting";
        case ControllerStatus::Connected:    return "Connected";
        case ControllerStatus::Error:        return "Error";
        default:                             return "Unknown";
    }
}

void ControllerProxy::onDataUpdated(const QString& controllerName, const QString& command, const QString& value)
{
    if (controllerName != m_name) return;

    if (command == ":DZ#" || command == ":GZ#") {
        double val = parseDegrees(value);
        if (val != m_azimuth) {
            m_azimuth = val;
            emit azimuthChanged();
        }
    } else if (command == ":GA#") {
        double val = parseDegrees(value);
        if (val != m_altitude) {
            m_altitude = val;
            emit altitudeChanged();
        }
    } else if (command == ":GR#") {
        // RA is usually in HH:MM:SS format
        // For now just store as a double if it's numeric, or we'll need a better parser
        double val = parseDegrees(value);
        if (val != m_ra) {
            m_ra = val;
            emit raChanged();
        }
    } else if (command == ":GD#") {
        double val = parseDegrees(value);
        if (val != m_dec) {
            m_dec = val;
            emit decChanged();
        }
    }
}

void ControllerProxy::onStatusChanged(const QString& name, ControllerStatus status)
{
    if (name == m_name) {
        emit statusChanged();
    }
}

double ControllerProxy::parseDegrees(const QString& value)
{
    // Basic parser for LX200 formats: 
    // sDD*MM'SS# or DD.DDDD# or HH:MM:SS#
    
    // Strip trailing #
    QString clean = value;
    if (clean.endsWith('#')) clean.chop(1);
    
    // Try simple numeric first
    bool ok;
    double d = clean.toDouble(&ok);
    if (ok) return d;
    
    // Try DD*MM'SS format
    QRegularExpression re("([-+]?\\d+)[*°](\\d+)['](\\d+)[\"]?");
    QRegularExpressionMatch match = re.match(clean);
    if (match.hasMatch()) {
        double deg = match.captured(1).toDouble();
        double min = match.captured(2).toDouble();
        double sec = match.captured(3).toDouble();
        double sign = deg < 0 ? -1.0 : 1.0;
        return deg + sign * (min / 60.0 + sec / 3600.0);
    }
    
    // Try HH:MM:SS format
    QRegularExpression re2("(\\d+):(\\d+):(\\d+)");
    QRegularExpressionMatch match2 = re2.match(clean);
    if (match2.hasMatch()) {
        double h = match2.captured(1).toDouble();
        double m = match2.captured(2).toDouble();
        double s = match2.captured(3).toDouble();
        return h + m / 60.0 + s / 3600.0;
    }

    return 0.0;
}

} // namespace ObservatoryMonitor
